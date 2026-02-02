#include "h264Decoder.h"
#include "utilities/logger.h"

#include <QImage>
#include <stdexcept>
#include <cstring>
#include <algorithm>

extern "C" {
#include "codec_api.h"
#include "codec_app_def.h"
#include "codec_def.h"
}

H264Decoder::H264Decoder()
    : m_initialized(false)
    , m_decoder(nullptr)
{
    m_yuvData[0] = nullptr;
    m_yuvData[1] = nullptr;
    m_yuvData[2] = nullptr;
    memset(&m_bufferInfo, 0, sizeof(SBufferInfo));
}

H264Decoder::~H264Decoder()
{
    cleanup();
}

bool H264Decoder::initialize()
{
    LOG_DEBUG("H264Decoder: Initializing decoder...");
    cleanup();
    
    // Create decoder
    int result = WelsCreateDecoder(&m_decoder);
    if (result != 0 || !m_decoder) {
        LOG_ERROR("H264Decoder: Failed to create decoder, error: {}", result);
        return false;
    }
    
    LOG_DEBUG("H264Decoder: Decoder created successfully");
    
    // Set decoder parameters
    SDecodingParam decParam;
    memset(&decParam, 0, sizeof(SDecodingParam));
    decParam.uiTargetDqLayer = UINT_MAX;
    decParam.eEcActiveIdc = ERROR_CON_SLICE_MV_COPY_CROSS_IDR_FREEZE_RES_CHANGE;
    decParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;
    // eOutputColorFormat не существует в этой версии - убрали
    
    // Initialize decoder
    result = m_decoder->Initialize(&decParam);
    if (result != 0) {
        LOG_ERROR("H264Decoder: Failed to initialize decoder, error: {}", result);
        cleanup();
        return false;
    }
    
    LOG_INFO("H264Decoder: Decoder initialized successfully");
    
    m_initialized = true;
    return true;
}

QImage H264Decoder::decode(const std::vector<unsigned char>& h264Data)
{
    if (!m_initialized || !m_decoder || h264Data.empty()) {
        LOG_ERROR("H264Decoder: Not initialized or empty data - decoder: {}, initialized: {}, data size: {}", 
                  static_cast<void*>(m_decoder), m_initialized, h264Data.size());
        return QImage();
    }
    
    // Убираем спам логов для каждого кадра, оставляем только при проблемах
    // LOG_DEBUG("H264Decoder: Decoding {} bytes", h264Data.size());
    
    // Log first few bytes for debugging (только если есть проблемы)
    // if (h264Data.size() >= 8) {
    //     LOG_DEBUG("H264Decoder: First 8 bytes: {:02x} {:02x} {:02x} {:02x} {:02x} {:02x} {:02x} {:02x}", 
    //               h264Data[0], h264Data[1], h264Data[2], h264Data[3],
    //               h264Data[4], h264Data[5], h264Data[6], h264Data[7]);
    // }
    
    // Reset buffer info
    memset(&m_bufferInfo, 0, sizeof(SBufferInfo));
    m_yuvData[0] = nullptr;
    m_yuvData[1] = nullptr;
    m_yuvData[2] = nullptr;
    
    // Decode frame
    unsigned char* inputData[3] = { const_cast<unsigned char*>(h264Data.data()), nullptr, nullptr };
    int result = m_decoder->DecodeFrame2(inputData[0], static_cast<int>(h264Data.size()), m_yuvData, &m_bufferInfo);
    
    if (result != 0) {
        // Error 16 means no start code found - might need SPS/PPS first
        if (result == 16) {
            LOG_DEBUG("H264Decoder: No start code found (error 16), might be waiting for SPS/PPS");
        } else {
            LOG_ERROR("H264Decoder: Failed to decode frame, error: {}", result);
        }
        return QImage();
    }
    
    // Check if we have a valid output frame
    if (m_bufferInfo.iBufferStatus != 1) {
        // Убираем спам логов для каждого кадра
        // LOG_DEBUG("H264Decoder: No output frame available (buffer status: {})", m_bufferInfo.iBufferStatus);
        return QImage();
    }
    
    // Get frame dimensions from buffer info
    int width = m_bufferInfo.UsrData.sSystemBuffer.iWidth;
    int height = m_bufferInfo.UsrData.sSystemBuffer.iHeight;
    
    // Alternative: get from stride info if system buffer is zero
    if (width <= 0 || height <= 0) {
        width = m_bufferInfo.UsrData.sSystemBuffer.iWidth;
        height = m_bufferInfo.UsrData.sSystemBuffer.iHeight;
        if (width <= 0 || height <= 0) {
            // Use default dimensions if still zero
            width = 1920;
            height = 1080;
            LOG_WARN("H264Decoder: Using default dimensions {}x{}", width, height);
        }
    }
    
    if (!m_yuvData[0] || !m_yuvData[1] || !m_yuvData[2]) {
        LOG_ERROR("H264Decoder: Invalid YUV data pointers - yData: {}, uData: {}, vData: {}", 
                  static_cast<void*>(m_yuvData[0]), 
                  static_cast<void*>(m_yuvData[1]), 
                  static_cast<void*>(m_yuvData[2]));
        return QImage();
    }
    
    // Убираем спам логов для каждого кадра
    // LOG_DEBUG("H264Decoder: Decoded frame - size: {}x{}", width, height);
    
    // Convert YUV to RGB QImage
    QImage rgbImage = convertYUVToRGB(m_yuvData[0], m_yuvData[1], m_yuvData[2], width, height);
    
    if (rgbImage.isNull()) {
        LOG_ERROR("H264Decoder: Failed to convert YUV to RGB");
        return QImage();
    }
    
    // Убираем спам логов для каждого кадра
    // LOG_DEBUG("H264Decoder: YUV to RGB conversion successful");
    return rgbImage;
}

QImage H264Decoder::convertYUVToRGB(const unsigned char* yData, const unsigned char* uData, const unsigned char* vData, 
                                   int width, int height)
{
    if (!yData || !uData || !vData || width <= 0 || height <= 0) {
        return QImage();
    }
    
    // Create RGB image
    QImage rgbImage(width, height, QImage::Format_RGB888);
    if (rgbImage.isNull()) {
        return QImage();
    }
    
    const unsigned char* y = yData;
    const unsigned char* u = uData;
    const unsigned char* v = vData;
    
    // Use default strides since buffer info may not have correct stride data
    int yStride = width;
    int uvStride = width / 2;
    
    // Try to get actual strides from buffer info, but fallback to defaults
    if (m_bufferInfo.UsrData.sSystemBuffer.iStride[0] > 0) {
        yStride = m_bufferInfo.UsrData.sSystemBuffer.iStride[0];
    }
    if (m_bufferInfo.UsrData.sSystemBuffer.iStride[1] > 0) {
        uvStride = m_bufferInfo.UsrData.sSystemBuffer.iStride[1];
    }
    
    // Убираем спам логов для каждого кадра
    // LOG_DEBUG("H264Decoder: Using strides - Y: {}, UV: {}, dimensions: {}x{}", 
    //           yStride, uvStride, width, height);
    
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            // Get YUV values with proper stride
            int yVal = y[j * yStride + i];
            int uVal = u[(j / 2) * uvStride + (i / 2)];
            int vVal = v[(j / 2) * uvStride + (i / 2)];
            
            // Clamp values to valid range
            yVal = std::clamp(yVal, 0, 255);
            uVal = std::clamp(uVal, 0, 255);
            vVal = std::clamp(vVal, 0, 255);
            
            // YUV to RGB conversion (BT.601 standard)
            int c = yVal - 16;
            int d = uVal - 128;
            int e = vVal - 128;
            
            int r = std::clamp((298 * c + 409 * e + 128) >> 8, 0, 255);
            int g = std::clamp((298 * c - 100 * d - 208 * e + 128) >> 8, 0, 255);
            int b = std::clamp((298 * c + 516 * d + 128) >> 8, 0, 255);
            
            // Set pixel in QImage
            rgbImage.setPixel(i, j, qRgb(r, g, b));
        }
    }
    
    return rgbImage;
}

bool H264Decoder::allocateOutputBuffers(int width, int height)
{
    freeOutputBuffers();
    
    // Allocate YUV buffers
    size_t ySize = width * height;
    size_t uvSize = ySize / 4;
    
    m_yuvData[0] = new unsigned char[ySize];
    m_yuvData[1] = new unsigned char[uvSize];
    m_yuvData[2] = new unsigned char[uvSize];
    
    if (!m_yuvData[0] || !m_yuvData[1] || !m_yuvData[2]) {
        freeOutputBuffers();
        return false;
    }
    
    return true;
}

void H264Decoder::freeOutputBuffers()
{
    delete[] m_yuvData[0];
    delete[] m_yuvData[1];
    delete[] m_yuvData[2];
    
    m_yuvData[0] = nullptr;
    m_yuvData[1] = nullptr;
    m_yuvData[2] = nullptr;
}

void H264Decoder::cleanup()
{
    if (m_decoder) {
        m_decoder->Uninitialize();
        WelsDestroyDecoder(m_decoder);
        m_decoder = nullptr;
    }
    
    freeOutputBuffers();
    m_initialized = false;
}
