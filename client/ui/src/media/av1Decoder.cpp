#include "av1Decoder.h"
#include "utilities/logger.h"

#include <QImage>
#include <stdexcept>
#include <cstring>
#include <algorithm>

AV1Decoder::AV1Decoder()
    : m_initialized(false)
    , m_context(nullptr)
    , m_picture(nullptr)
    , m_data(nullptr)
    , m_width(0)
    , m_height(0)
{
}

AV1Decoder::~AV1Decoder()
{
    cleanup();
}

bool AV1Decoder::initialize()
{
    LOG_DEBUG("AV1Decoder: Initializing decoder...");
    cleanup();
    
    // Create dav1d context
    Dav1dSettings settings;
    dav1d_default_settings(&settings);
    
    // Configure settings
    settings.n_threads = 1; // Single thread for simplicity
    settings.max_frame_delay = 1; // Low delay
    
    LOG_DEBUG("AV1Decoder: Opening dav1d context...");
    int result = dav1d_open(&m_context, &settings);
    if (result != 0) {
        LOG_ERROR("AV1Decoder: Failed to open dav1d context, error: {}", result);
        cleanup();
        return false;
    }
    LOG_DEBUG("AV1Decoder: Dav1d context opened successfully");
    
    // Allocate picture
    m_picture = static_cast<Dav1dPicture*>(malloc(sizeof(Dav1dPicture)));
    if (!m_picture) {
        cleanup();
        return false;
    }
    memset(m_picture, 0, sizeof(Dav1dPicture));
    
    // Allocate data
    m_data = static_cast<Dav1dData*>(malloc(sizeof(Dav1dData)));
    if (!m_data) {
        cleanup();
        return false;
    }
    memset(m_data, 0, sizeof(Dav1dData));
    
    LOG_INFO("AV1Decoder: Decoder initialized successfully");
    m_initialized = true;
    return true;
}

bool AV1Decoder::parseIVF(const unsigned char* ivfData, size_t size, Dav1dData* outputData)
{
    if (!ivfData || size < 12 || !outputData) {
        return false;
    }
    
    // Read IVF frame header (12 bytes)
    // First 4 bytes: frame size (little-endian)
    uint32_t frameSize = static_cast<uint32_t>(ivfData[0]) |
                        (static_cast<uint32_t>(ivfData[1]) << 8) |
                        (static_cast<uint32_t>(ivfData[2]) << 16) |
                        (static_cast<uint32_t>(ivfData[3]) << 24);
    
    // Next 8 bytes: PTS (we can ignore for now)
    
    // Check if we have enough data
    if (size < 12 + frameSize) {
        return false;
    }
    
    // Set up dav1d data
    dav1d_data_unref(outputData);
    
    // Allocate buffer for the AV1 data
    uint8_t* buf = static_cast<uint8_t*>(malloc(frameSize));
    if (!buf) {
        return false;
    }
    
    memcpy(buf, ivfData + 12, frameSize);
    
    outputData->data = buf;
    outputData->sz = frameSize;
    outputData->m.timestamp = 0;
    outputData->m.duration = 0;
    outputData->m.offset = 0;
    outputData->m.size = frameSize;
    outputData->m.user_data.data = nullptr;
    outputData->m.user_data.ref = nullptr;
    
    return true;
}

bool AV1Decoder::decodeData(Dav1dData* data)
{
    if (!m_context || !data || !m_picture) {
        LOG_ERROR("AV1Decoder: Invalid context, data, or picture");
        return false;
    }
    
    // Check if data is empty
    if (data->sz == 0 || !data->data) {
        LOG_ERROR("AV1Decoder: Empty data provided to decoder");
        return false;
    }
    
    LOG_DEBUG("AV1Decoder: Decoding data - size: {}, ptr: {}", data->sz, static_cast<const void*>(data->data));
    
    // Send data to decoder
    int result = dav1d_send_data(m_context, data);
    if (result != 0) {
        LOG_ERROR("AV1Decoder: dav1d_send_data failed with result: {}", result);
        return false;
    }
    
    // Get picture from decoder
    result = dav1d_get_picture(m_context, m_picture);
    if (result != 0) {
        LOG_ERROR("AV1Decoder: dav1d_get_picture failed with result: {}", result);
        return false;
    }
    
    LOG_DEBUG("AV1Decoder: Got picture - size: {}x{}, data[0]: {}, data[1]: {}, data[2]: {}", 
              m_picture->p.w, m_picture->p.h,
              static_cast<const void*>(m_picture->data[0]),
              static_cast<const void*>(m_picture->data[1]),
              static_cast<const void*>(m_picture->data[2]));
    
    // Update dimensions if changed
    if (m_width != m_picture->p.w || m_height != m_picture->p.h) {
        m_width = m_picture->p.w;
        m_height = m_picture->p.h;
    }
    
    return true;
}

QImage AV1Decoder::convertYUVToRGB(const Dav1dPicture* picture)
{
    if (!picture || !picture->data[0]) {
        LOG_ERROR("AV1Decoder: Invalid picture or null data[0]");
        return QImage();
    }
    
    int width = picture->p.w;
    int height = picture->p.h;
    
    LOG_DEBUG("AV1Decoder: Converting YUV to RGB - size: {}x{}", width, height);
    
    // Get YUV data
    const uint8_t* y = static_cast<const uint8_t*>(picture->data[0]);
    const uint8_t* u = static_cast<const uint8_t*>(picture->data[1]);
    const uint8_t* v = static_cast<const uint8_t*>(picture->data[2]);
    
    int yStride = picture->stride[0];
    int uStride = picture->stride[1];
    int vStride = picture->stride[2];
    
    // Fix V stride if it's wrong (should be same as U stride for YUV420)
    if (vStride != uStride) {
        LOG_WARN("AV1Decoder: V stride {} differs from U stride {}, using U stride for V", vStride, uStride);
        vStride = uStride;
    }
    
    LOG_DEBUG("AV1Decoder: Strides - Y: {}, U: {}, V: {}", yStride, uStride, vStride);
    LOG_DEBUG("AV1Decoder: Data pointers - Y: {}, U: {}, V: {}", 
              static_cast<const void*>(y), static_cast<const void*>(u), static_cast<const void*>(v));
    
    // Validate pointers and strides
    if (!y || !u || !v || yStride <= 0 || uStride <= 0 || vStride <= 0) {
        LOG_ERROR("AV1Decoder: Invalid YUV data or strides - Y: {}, U: {}, V: {}, yStride: {}, uStride: {}, vStride: {}", 
                  static_cast<const void*>(y), static_cast<const void*>(u), static_cast<const void*>(v),
                  yStride, uStride, vStride);
        return QImage();
    }
    
    // Check if pointers look valid (not obviously corrupted)
    uintptr_t yPtr = reinterpret_cast<uintptr_t>(y);
    uintptr_t uPtr = reinterpret_cast<uintptr_t>(u);
    uintptr_t vPtr = reinterpret_cast<uintptr_t>(v);
    
    // Check for obviously invalid pointers (too small)
    if (yPtr < 0x1000 || uPtr < 0x1000 || vPtr < 0x1000) {
        LOG_ERROR("AV1Decoder: Suspicious pointer values - Y: 0x{:x}, U: 0x{:x}, V: 0x{:x}", yPtr, uPtr, vPtr);
        return QImage();
    }
    
    // Also check if they're not obviously corrupted (like all bits set)
    if (yPtr == 0xFFFFFFFFFFFFFFFF || uPtr == 0xFFFFFFFFFFFFFFFF || vPtr == 0xFFFFFFFFFFFFFFFF) {
        LOG_ERROR("AV1Decoder: Corrupted pointer values - Y: 0x{:x}, U: 0x{:x}, V: 0x{:x}", yPtr, uPtr, vPtr);
        return QImage();
    }
    
    // Create RGB image
    QImage rgbImage(width, height, QImage::Format_RGB888);
    if (rgbImage.isNull()) {
        LOG_ERROR("AV1Decoder: Failed to create RGB image");
        return QImage();
    }
    
    uint8_t* rgb = rgbImage.bits();
    int rgbStride = width * 3;
    
    // Convert YUV420 to RGB888
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            // Get YUV values with bounds checking
            int yIndex = j * yStride + i;
            if (yIndex >= yStride * height) {
                LOG_ERROR("AV1Decoder: Y index out of bounds - index: {}, max: {}", yIndex, yStride * height);
                return QImage();
            }
            int yVal = y[yIndex];
            
            // Calculate UV indices safely
            int uvJ = j / 2;
            int uvI = i / 2;
            
            // Check UV bounds before accessing
            if (uvJ >= (height / 2) || uvI >= (width / 2)) {
                LOG_ERROR("AV1Decoder: UV bounds check failed - uvJ: {}, uvI: {}, maxJ: {}, maxI: {}", 
                          uvJ, uvI, height / 2, width / 2);
                return QImage();
            }
            
            int uIndex = uvJ * uStride + uvI;
            int vIndex = uvJ * vStride + uvI;
            
            if (uIndex >= uStride * (height / 2) || vIndex >= vStride * (height / 2)) {
                LOG_ERROR("AV1Decoder: UV index out of bounds - uIndex: {}, vIndex: {}, uMax: {}, vMax: {}", 
                          uIndex, vIndex, uStride * (height / 2), vStride * (height / 2));
                return QImage();
            }
            
            int uVal = u[uIndex];
            int vVal = v[vIndex];
            
            // Convert YUV420 to RGB (BT.601)
            int c = yVal - 16;
            int d = uVal - 128;
            int e = vVal - 128;
            int r = (298 * c + 409 * e + 128) >> 8;
            int g = (298 * c - 100 * d - 208 * e + 128) >> 8;
            int b = (298 * c + 516 * d + 128) >> 8;
            
            // Clamp values
            r = std::clamp(r, 0, 255);
            g = std::clamp(g, 0, 255);
            b = std::clamp(b, 0, 255);
            
            // Set RGB values
            int rgbIndex = j * rgbStride + i * 3;
            rgb[rgbIndex] = static_cast<uint8_t>(r);
            rgb[rgbIndex + 1] = static_cast<uint8_t>(g);
            rgb[rgbIndex + 2] = static_cast<uint8_t>(b);
        }
    }
    
    LOG_DEBUG("AV1Decoder: YUV to RGB conversion completed successfully");
    return rgbImage;
}

QImage AV1Decoder::decode(const std::vector<unsigned char>& av1Data)
{
    if (!m_initialized || av1Data.empty()) {
        LOG_ERROR("AV1Decoder: Not initialized or empty data");
        return QImage();
    }
    
    LOG_DEBUG("AV1Decoder: Decoding raw AV1 data - size: {}", av1Data.size());
    
    return decode(av1Data.data(), av1Data.size());
}

QImage AV1Decoder::decode(const unsigned char* data, size_t size)
{
    if (!m_initialized || !data || size == 0) {
        LOG_ERROR("AV1Decoder: Not initialized or invalid data");
        return QImage();
    }
    
    LOG_DEBUG("AV1Decoder: Starting decode - size: {}", size);
    
    // Set up dav1d data directly (no IVF parsing)
    dav1d_data_unref(m_data);
    
    // Create a copy of the data
    uint8_t* buf = static_cast<uint8_t*>(malloc(size));
    if (!buf) {
        LOG_ERROR("AV1Decoder: Failed to allocate memory for AV1 data");
        return QImage();
    }
    
    memcpy(buf, data, size);
    
    m_data->data = buf;
    m_data->sz = size;
    m_data->m.timestamp = 0;
    m_data->m.duration = 0;
    m_data->m.offset = 0;
    m_data->m.size = size;
    m_data->m.user_data.data = nullptr;
    m_data->m.user_data.ref = nullptr;
    
    // Decode data to picture
    if (!decodeData(m_data)) {
        LOG_ERROR("AV1Decoder: Failed to decode data");
        free(buf);
        return QImage();
    }
    
    // Convert YUV to RGB
    QImage result = convertYUVToRGB(m_picture);
    
    // Unreference the picture and data
    dav1d_picture_unref(m_picture);
    dav1d_data_unref(m_data);
    
    return result;
}

void AV1Decoder::cleanup()
{
    if (m_data) {
        dav1d_data_unref(m_data);
        free(m_data);
        m_data = nullptr;
    }
    
    if (m_picture) {
        dav1d_picture_unref(m_picture);
        free(m_picture);
        m_picture = nullptr;
    }
    
    if (m_context) {
        dav1d_close(&m_context);
        m_context = nullptr;
    }
    
    m_initialized = false;
    m_width = 0;
    m_height = 0;
}
