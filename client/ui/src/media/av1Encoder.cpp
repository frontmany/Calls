#include "av1Encoder.h"
#include "utilities/constant.h"
#include "utilities/logger.h"

#include <QImage>
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <thread>
#include <chrono>

// IVF header for AV1
static const char IVF_HEADER[] = "DKIF";
static const uint32_t IVF_VERSION = 0;
static const uint32_t IVF_HEADER_SIZE = 32;

AV1Encoder::AV1Encoder()
    : m_initialized(false)
    , m_encoderHandle(nullptr)
    , m_inputBuffer(nullptr)
    , m_outputBuffer(nullptr)
    , m_streamHeaderSent(false)
    , m_width(0)
    , m_height(0)
    , m_frameNumber(0)
    , m_yData(nullptr)
    , m_uData(nullptr)
    , m_vData(nullptr)
{
    memset(&m_config, 0, sizeof(m_config));
}

AV1Encoder::~AV1Encoder()
{
    cleanup();
}

bool AV1Encoder::initialize(int width, int height, int preset, int crf, int keyint, double fps)
{
    cleanup();
    
    m_width = width;
    m_height = height;
    m_frameNumber = 0;
    m_streamHeader.clear();
    m_streamHeaderSent = false;
    
    // Initialize SVT-AV1 configuration with defaults
    memset(&m_config, 0, sizeof(m_config));
    
    LOG_DEBUG("AV1Encoder: Initializing with {}x{}, preset: {}, crf: {}", width, height, preset, crf);
    
    // Create encoder handle first - this will reset the config to defaults
    LOG_DEBUG("AV1Encoder: Creating encoder handle...");
    EbErrorType ret = svt_av1_enc_init_handle(&m_encoderHandle, &m_config);
    if (ret != EB_ErrorNone) {
        LOG_ERROR("AV1Encoder: svt_av1_enc_init_handle failed with error: {}", static_cast<int>(ret));
        cleanup();
        return false;
    }
    LOG_DEBUG("AV1Encoder: Encoder handle created successfully");
    
    LOG_DEBUG("AV1Encoder: After init_handle - source: {}x{}, max: {}x{}", 
              m_config.source_width, m_config.source_height,
              m_config.forced_max_frame_width, m_config.forced_max_frame_height);
    
    // NOW set our parameters AFTER init_handle
    m_config.source_width = width;
    m_config.source_height = height;
    m_config.forced_max_frame_width = width;
    m_config.forced_max_frame_height = height;
    m_config.encoder_bit_depth = 8;
    m_config.encoder_color_format = EB_YUV420;
    m_config.profile = MAIN_PROFILE;
    m_config.rate_control_mode = SVT_AV1_RC_MODE_CQP_OR_CRF;
    m_config.qp = crf;
    m_config.intra_period_length = keyint;
    m_config.enc_mode = preset;
    m_config.frame_rate_numerator = static_cast<uint32_t>(fps * 1000);
    m_config.frame_rate_denominator = 1000;
    
    // Speed/quality knobs available in this SVT-AV1 version
    // (keep them conservative to avoid bitstream oddities)
    m_config.film_grain_denoise_apply = 0;
    m_config.film_grain_denoise_strength = 0;
    m_config.cdef_level = -1;
    m_config.enable_restoration_filtering = -1;
    
    LOG_DEBUG("AV1Encoder: Set parameters after init_handle - source: {}x{}, max: {}x{}", 
              m_config.source_width, m_config.source_height,
              m_config.forced_max_frame_width, m_config.forced_max_frame_height);
    
    // Set the parameters to encoder
    LOG_DEBUG("AV1Encoder: Setting encoder parameters...");
    ret = svt_av1_enc_set_parameter(m_encoderHandle, &m_config);
    if (ret != EB_ErrorNone) {
        LOG_ERROR("AV1Encoder: svt_av1_enc_set_parameter failed with error: {}", static_cast<int>(ret));
        cleanup();
        return false;
    }
    LOG_DEBUG("AV1Encoder: Parameters set successfully");
    
    // Initialize encoder
    LOG_DEBUG("AV1Encoder: Initializing encoder...");
    ret = svt_av1_enc_init(m_encoderHandle);
    if (ret != EB_ErrorNone) {
        LOG_ERROR("AV1Encoder: svt_av1_enc_init failed with error: {}", static_cast<int>(ret));
        cleanup();
        return false;
    }
    LOG_DEBUG("AV1Encoder: Encoder initialized successfully");
    
    // Get stream headers (sequence header OBU) once at init time.
    EbBufferHeaderType* streamHeaderPtr = nullptr;
    ret = svt_av1_enc_stream_header(m_encoderHandle, &streamHeaderPtr);
    if (ret == EB_ErrorNone && streamHeaderPtr && streamHeaderPtr->p_buffer && streamHeaderPtr->n_filled_len > 0) {
        m_streamHeader.assign(streamHeaderPtr->p_buffer, streamHeaderPtr->p_buffer + streamHeaderPtr->n_filled_len);
        LOG_DEBUG("AV1Encoder: Got stream header - size: {}", streamHeaderPtr->n_filled_len);
    } else {
        LOG_WARN("AV1Encoder: Failed to get stream header - ret: {}, ptr: {}, buffer: {}, size: {}",
                 static_cast<int>(ret),
                 static_cast<void*>(streamHeaderPtr),
                 streamHeaderPtr ? static_cast<void*>(streamHeaderPtr->p_buffer) : nullptr,
                 streamHeaderPtr ? streamHeaderPtr->n_filled_len : 0);
    }
    if (streamHeaderPtr) {
        svt_av1_enc_stream_header_release(streamHeaderPtr);
    }
    
    // Allocate input buffer
    m_inputBuffer = static_cast<EbBufferHeaderType*>(malloc(sizeof(EbBufferHeaderType)));
    if (!m_inputBuffer) {
        cleanup();
        return false;
    }
    memset(m_inputBuffer, 0, sizeof(EbBufferHeaderType));
    
    // Allocate YUV buffer for input - use EbSvtIOFormat structure like in the sample
    size_t ySize = width * height;
    size_t uvSize = ySize / 4;
    size_t totalSize = sizeof(EbSvtIOFormat) + ySize + 2 * uvSize;
    
    m_inputBuffer->p_buffer = static_cast<uint8_t*>(malloc(totalSize));
    if (!m_inputBuffer->p_buffer) {
        cleanup();
        return false;
    }
    m_inputBuffer->n_alloc_len = totalSize;
    m_inputBuffer->size = sizeof(EbBufferHeaderType);
    
    // Set up EbSvtIOFormat structure
    EbSvtIOFormat* inputFormat = reinterpret_cast<EbSvtIOFormat*>(m_inputBuffer->p_buffer);
    inputFormat->y_stride = width;
    inputFormat->cr_stride = width / 2;
    inputFormat->cb_stride = width / 2;
    
    // Calculate data pointers after the structure
    uint8_t* yData = reinterpret_cast<uint8_t*>(inputFormat + 1);
    uint8_t* uData = yData + ySize;
    uint8_t* vData = uData + uvSize;
    
    // Set the luma/chroma pointers in the structure
    inputFormat->luma = yData;
    inputFormat->cb = uData;
    inputFormat->cr = vData;
    
    // Store pointers for later use in convertRGBToYUV
    m_yData = yData;
    m_uData = uData;
    m_vData = vData;
    
    // Allocate output buffer
    m_outputBuffer = static_cast<EbBufferHeaderType*>(malloc(sizeof(EbBufferHeaderType)));
    if (!m_outputBuffer) {
        cleanup();
        return false;
    }
    memset(m_outputBuffer, 0, sizeof(EbBufferHeaderType));
    m_outputBuffer->size = sizeof(EbBufferHeaderType);
    
    // Complete the encoder initialization
    LOG_DEBUG("AV1Encoder: Completing encoder initialization...");
    ret = svt_av1_enc_init(m_encoderHandle);
    if (ret != EB_ErrorNone) {
        LOG_ERROR("AV1Encoder: svt_av1_enc_init failed with error: {}", static_cast<int>(ret));
        cleanup();
        return false;
    }
    LOG_INFO("AV1Encoder: Encoder initialized successfully for {}x{}", width, height);
    
    m_initialized = true;
    return true;
}

bool AV1Encoder::convertRGBToYUV(const unsigned char* rgbData, int width, int height)
{
    if (!rgbData || !m_yData || !m_uData || !m_vData) {
        return false;
    }
    
    const unsigned char* rgb = rgbData;
    
    // Convert to YUV420
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            int r = rgb[0];
            int g = rgb[1];
            int b = rgb[2];
            rgb += 3;
            
            // YUV conversion
            int yVal = ((66 * r + 129 * g + 25 * b + 128) >> 8) + 16;
            int uVal = ((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128;
            int vVal = ((112 * r - 94 * g - 18 * b + 128) >> 8) + 128;
            
            m_yData[j * width + i] = std::clamp(yVal, 0, 255);
            
            // Subsample UV
            if ((j % 2 == 0) && (i % 2 == 0)) {
                int uvIndex = (j / 2) * (width / 2) + (i / 2);
                m_uData[uvIndex] = std::clamp(uVal, 0, 255);
                m_vData[uvIndex] = std::clamp(vVal, 0, 255);
            }
        }
    }
    
    return true;
}

std::vector<unsigned char> AV1Encoder::createIVFHeader() const
{
    std::vector<unsigned char> header(IVF_HEADER_SIZE, 0);
    
    // Signature
    memcpy(header.data(), IVF_HEADER, 4);
    
    // Version
    header[4] = IVF_VERSION & 0xFF;
    header[5] = (IVF_VERSION >> 8) & 0xFF;
    
    // Header length
    header[6] = IVF_HEADER_SIZE & 0xFF;
    header[7] = (IVF_HEADER_SIZE >> 8) & 0xFF;
    
    // FourCC (AV01)
    header[8] = 'A';
    header[9] = 'V';
    header[10] = '0';
    header[11] = '1';
    
    // Width
    header[12] = m_width & 0xFF;
    header[13] = (m_width >> 8) & 0xFF;
    header[14] = (m_width >> 16) & 0xFF;
    header[15] = (m_width >> 24) & 0xFF;
    
    // Height
    header[16] = m_height & 0xFF;
    header[17] = (m_height >> 8) & 0xFF;
    header[18] = (m_height >> 16) & 0xFF;
    header[19] = (m_height >> 24) & 0xFF;
    
    // Time base (denominator)
    header[20] = 30 & 0xFF;
    header[21] = (30 >> 8) & 0xFF;
    header[22] = (30 >> 16) & 0xFF;
    header[23] = (30 >> 24) & 0xFF;
    
    // Time base (numerator)
    header[24] = 1 & 0xFF;
    header[25] = (1 >> 8) & 0xFF;
    header[26] = (1 >> 16) & 0xFF;
    header[27] = (1 >> 24) & 0xFF;
    
    // Frame count (will be updated later)
    // header[28-31] = frame count
    
    return header;
}

std::vector<unsigned char> AV1Encoder::packetToIVF(const EbBufferHeaderType* packet) const
{
    if (!packet || packet->n_filled_len == 0) {
        return {};
    }
    
    std::vector<unsigned char> ivfData;
    ivfData.reserve(12 + packet->n_filled_len);
    
    // IVF frame header (12 bytes)
    uint32_t frameSize = static_cast<uint32_t>(packet->n_filled_len);
    uint64_t pts = m_frameNumber;
    
    // Write frame size (little-endian)
    ivfData.push_back(static_cast<unsigned char>(frameSize & 0xFF));
    ivfData.push_back(static_cast<unsigned char>((frameSize >> 8) & 0xFF));
    ivfData.push_back(static_cast<unsigned char>((frameSize >> 16) & 0xFF));
    ivfData.push_back(static_cast<unsigned char>((frameSize >> 24) & 0xFF));
    
    // Write PTS (little-endian, 64-bit)
    for (int i = 0; i < 8; ++i) {
        ivfData.push_back(static_cast<unsigned char>((pts >> (i * 8)) & 0xFF));
    }
    
    // Write packet data
    ivfData.insert(ivfData.end(), packet->p_buffer, packet->p_buffer + packet->n_filled_len);
    
    return ivfData;
}

std::vector<unsigned char> AV1Encoder::encode(const unsigned char* rgbData, int width, int height)
{
    if (!m_initialized || !rgbData || !m_encoderHandle || !m_inputBuffer) {
        LOG_ERROR("AV1Encoder: Not initialized for RGB encoding - initialized: {}, data: {}, handle: {}, buffer: {}", 
                  m_initialized, static_cast<const void*>(rgbData), 
                  static_cast<void*>(m_encoderHandle), static_cast<void*>(m_inputBuffer));
        return {};
    }
    
    LOG_DEBUG("AV1Encoder: RGB encoding - size: {}x{}", width, height);
    
    // Convert RGB to YUV420p and copy to input buffer
    if (!convertRGBToYUV(rgbData, width, height)) {
        LOG_ERROR("AV1Encoder: RGB to YUV conversion failed");
        return {};
    }
    
    LOG_DEBUG("AV1Encoder: RGB to YUV conversion successful");
    
    // Set up input buffer properly
    m_inputBuffer->n_filled_len = m_inputBuffer->n_alloc_len;
    m_inputBuffer->pts = m_frameNumber;
    m_inputBuffer->dts = m_frameNumber;
    m_inputBuffer->pic_type = EB_AV1_INVALID_PICTURE;
    m_inputBuffer->flags = 0;
    m_inputBuffer->metadata = nullptr;
    
    // Increment frame number after setting up buffer
    m_frameNumber++;
    
    // Send frame to encoder
    EbErrorType ret = svt_av1_enc_send_picture(m_encoderHandle, m_inputBuffer);
    if (ret != EB_ErrorNone) {
        LOG_ERROR("AV1Encoder: Failed to send picture to encoder, error: {}", static_cast<int>(ret));
        return {};
    }
    
    // Track buffered frames
    m_bufferedFrames.push_back(m_frameNumber);
    
    // Only try to get packets after buffering enough frames
    if (m_bufferedFrames.size() < MIN_BUFFERED_FRAMES) {
        LOG_DEBUG("AV1Encoder: Buffering frame {}/{}", m_bufferedFrames.size(), MIN_BUFFERED_FRAMES);
        return {};
    }
    
    // Get encoded data - now we should have packets available
    EbBufferHeaderType* outputBufferPtr = nullptr;
    
    // Try to get packet - non-blocking call
    ret = svt_av1_enc_get_packet(m_encoderHandle, &outputBufferPtr, 0);
    
    if (ret == EB_NoErrorEmptyQueue) {
        LOG_DEBUG("AV1Encoder: No packet available yet");
        return {};
    } else if (ret != EB_ErrorNone || !outputBufferPtr || !outputBufferPtr->p_buffer) {
        LOG_ERROR("AV1Encoder: Failed to get packet - ret: {}, ptr: {}, buffer: {}", 
                  static_cast<int>(ret), static_cast<void*>(outputBufferPtr),
                  outputBufferPtr ? static_cast<void*>(outputBufferPtr->p_buffer) : nullptr);
        return {};
    }
     
    LOG_DEBUG("AV1Encoder: Got packet - size: {}", outputBufferPtr->n_filled_len);

    if (outputBufferPtr->n_filled_len < 16) {
        LOG_DEBUG("AV1Encoder: Dropping too-small packet ({} bytes)", outputBufferPtr->n_filled_len);
        svt_av1_enc_release_out_buffer(&outputBufferPtr);
        return {};
    }
    
    // Convert to raw AV1 data; prepend stream header once so dav1d can parse OBUs reliably.
    const bool shouldPrependHeader = (!m_streamHeaderSent && !m_streamHeader.empty());
    std::vector<unsigned char> result;
    result.reserve((shouldPrependHeader ? m_streamHeader.size() : 0U) + outputBufferPtr->n_filled_len);
    if (shouldPrependHeader) {
        result.insert(result.end(), m_streamHeader.begin(), m_streamHeader.end());
        m_streamHeaderSent = true;
        LOG_DEBUG("AV1Encoder: Prepended stream header ({} bytes) to first packet", m_streamHeader.size());
    }
    result.insert(result.end(), outputBufferPtr->p_buffer, outputBufferPtr->p_buffer + outputBufferPtr->n_filled_len);
    
    // Release the output buffer
    svt_av1_enc_release_out_buffer(&outputBufferPtr);
    
    return result;
}

std::vector<unsigned char> AV1Encoder::encode(const QImage& image)
{
    if (!m_initialized || !m_encoderHandle || !m_inputBuffer) {
        LOG_ERROR("AV1Encoder: Not initialized - handle: {}, buffer: {}, initialized: {}", 
                  static_cast<void*>(m_encoderHandle), static_cast<void*>(m_inputBuffer), m_initialized);
        return {};
    }
    
    if (image.isNull()) {
        LOG_ERROR("AV1Encoder: Input image is null");
        return {};
    }
    
    LOG_DEBUG("AV1Encoder: Encoding frame - input size: {}x{}", image.width(), image.height());
    
    // Convert QImage to RGB24 format
    QImage rgbImage = image.convertToFormat(QImage::Format_RGB888);
    if (rgbImage.isNull()) {
        LOG_ERROR("AV1Encoder: Failed to convert image to RGB888");
        return {};
    }
    
    // Encode RGB data
    return encode(rgbImage.bits(), rgbImage.width(), rgbImage.height());
}

void AV1Encoder::cleanup()
{
    if (m_outputBuffer) {
        free(m_outputBuffer);
        m_outputBuffer = nullptr;
    }
    
    if (m_inputBuffer) {
        if (m_inputBuffer->p_buffer) {
            free(m_inputBuffer->p_buffer);
        }
        free(m_inputBuffer);
        m_inputBuffer = nullptr;
    }
    
    if (m_encoderHandle) {
        svt_av1_enc_deinit_handle(m_encoderHandle);
        m_encoderHandle = nullptr;
    }
    
    m_initialized = false;
    m_width = 0;
    m_height = 0;
    m_frameNumber = 0;
    m_yData = nullptr;
    m_uData = nullptr;
    m_vData = nullptr;
    m_streamHeader.clear();
    m_streamHeaderSent = false;
    m_bufferedFrames.clear();
}
