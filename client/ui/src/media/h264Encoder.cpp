#include "h264Encoder.h"
#include "hardwareEncoder.h"
#include "utilities/constant.h"
#include "utilities/logger.h"

#include <QImage>
#include <sstream>
#include <iomanip>
#include <thread>
#include <algorithm>

H264Encoder::H264Encoder()
    : m_initialized(false)
    , m_encoder(nullptr)
    , m_hardwareEncoder(nullptr)
    , m_width(0)
    , m_height(0)
    , m_frameNumber(0)
    , m_bitrate(H264_BITRATE_MEDIUM)
    , m_fps(30)
    , m_keyframeInterval(30)
    , m_minQp(H264_QUALITY_MIN_QP_BALANCED)
    , m_maxQp(H264_QUALITY_MAX_QP_BALANCED)
    , m_threads(autoDetectThreadCount())  // Автоматическое определение потоков
    , m_entropyMode(H264_ENTROPY_BEST)
{
    LOG_INFO("H264Encoder: Initialized with auto-detected {} threads", m_threads);
}

H264Encoder::~H264Encoder()
{
    cleanup();
}

bool H264Encoder::initializeWithHardware(int width, int height, int bitrate, int fps, int keyframeInterval)
{
    LOG_INFO("H264Encoder: Trying hardware acceleration first...");
    
    // Try hardware encoders first
    auto availableEncoders = HardwareEncoder::getAvailableEncoders();
    
    if (!availableEncoders.empty()) {
        auto encoderType = availableEncoders[0]; // Use first available
        LOG_INFO("H264Encoder: Using hardware encoder: {}", HardwareEncoder::getEncoderName(encoderType));
        
        m_hardwareEncoder = std::make_unique<HardwareEncoder>();
        if (m_hardwareEncoder->initialize(encoderType, width, height, bitrate, fps)) {
            m_width = width;
            m_height = height;
            m_bitrate = bitrate;
            m_fps = fps;
            m_keyframeInterval = keyframeInterval;
            m_frameNumber = 0;
            m_initialized = true;
            
            LOG_INFO("H264Encoder: Hardware encoder initialized successfully");
            return true;
        } else {
            LOG_WARN("H264Encoder: Hardware encoder failed, falling back to software");
            m_hardwareEncoder.reset();
        }
    } else {
        LOG_INFO("H264Encoder: No hardware encoders available, using software");
    }
    
    // Fallback to software encoder
    return initialize(width, height, bitrate, fps, keyframeInterval);
}

bool H264Encoder::initialize(int width, int height, int bitrate, int fps, int keyframeInterval)
{
    if (m_initialized) {
        LOG_WARN("H264Encoder: Already initialized");
        return true;
    }
    
    cleanup();
    
    m_width = width;
    m_height = height;
    m_bitrate = bitrate;
    m_fps = fps;
    m_keyframeInterval = keyframeInterval;
    m_frameNumber = 0;
    
    LOG_INFO("H264Encoder: Initializing {}x{} @ {}fps, bitrate: {} Mbps", 
             width, height, fps, bitrate / 1000000.0);
    
    // Create encoder
    int result = WelsCreateSVCEncoder(&m_encoder);
    if (result != 0 || !m_encoder) {
        LOG_ERROR("H264Encoder: Failed to create encoder, error: {}", result);
        return false;
    }
    
    LOG_DEBUG("H264Encoder: Encoder created successfully");
    
    // Setup encoder parameters
    SEncParamExt params;
    memset(&params, 0, sizeof(SEncParamExt));
    setupEncoderParams(params);
    
    // Initialize encoder directly
    result = m_encoder->InitializeExt(&params);
    if (result != 0) {
        LOG_ERROR("H264Encoder: Failed to initialize encoder, error: {}", result);
        cleanup();
        return false;
    }
    
    LOG_INFO("H264Encoder: Encoder initialized successfully for {}x{}", width, height);
    
    // Allocate YUV buffer
    size_t yuvSize = width * height * 3 / 2; // YUV420
    m_yuvBuffer.resize(yuvSize);
    
    m_initialized = true;
    return true;
}

void H264Encoder::setupEncoderParams(SEncParamExt& params) {
    // Basic settings
    params.iUsageType = CAMERA_VIDEO_REAL_TIME;
    params.iPicWidth = m_width;
    params.iPicHeight = m_height;
    params.iTargetBitrate = m_bitrate;
    params.iRCMode = RC_BITRATE_MODE;
    params.fMaxFrameRate = m_fps;
    
    // GOP structure
    params.uiIntraPeriod = m_keyframeInterval;
    
    // Quality settings - основные параметры качества
    params.iMaxQp = m_maxQp;           // Максимальный QP (качество) - 51=худшее, 18=хорошее
    params.iMinQp = m_minQp;           // Минимальный QP (качество) - слишком низкий может быть избыточным
    
    // Rate control settings
    params.iRCMode = RC_BITRATE_MODE;  // Режим контроля битрейта
    params.iMaxBitrate = m_bitrate * 2; // Максимальный битрейт (пики)
    
    // Multi-threading - оптимизация для производительности
    params.iMultipleThreadIdc = 4;      // Фиксированные 4 потока для лучшей производительности
    
    // Деблокинг фильтр - отключаем для скорости
    params.iLoopFilterDisableIdc = 1;   // 1=выключен для производительности
    params.iLoopFilterAlphaC0Offset = 0; // Смещение альфа/бета (-6..+6)
    params.iLoopFilterBetaOffset = 0;
    
    // Предварительная обработка - оптимизация для скорости
    params.bEnableDenoise = false;       // Отключено для скорости
    params.bEnableBackgroundDetection = false; // Отключено для скорости
    params.bEnableAdaptiveQuant = false;  // Отключено для скорости
    params.bEnableSceneChangeDetect = false; // Отключено для скорости
    
    // Энтропийное кодирование - используем быстрый режим
    params.iEntropyCodingModeFlag = 0;  // 0=CAVLC (быстрее), 1=CABAC (лучше сжатие)
    
    // Spatial layer settings - оптимизация
    params.iSpatialLayerNum = 1;
    params.sSpatialLayers[0].iVideoWidth = m_width;
    params.sSpatialLayers[0].iVideoHeight = m_height;
    params.sSpatialLayers[0].fFrameRate = m_fps;
    params.sSpatialLayers[0].iSpatialBitrate = m_bitrate;
    params.sSpatialLayers[0].iMaxSpatialBitrate = m_bitrate * 2;
    params.sSpatialLayers[0].sSliceArgument.uiSliceMode = SM_SINGLE_SLICE;
}

bool H264Encoder::convertRGBToYUV(const unsigned char* rgbData, int width, int height)
{
    if (!rgbData || m_yuvBuffer.empty()) {
        return false;
    }
    
    const unsigned char* rgb = rgbData;
    unsigned char* yPlane = m_yuvBuffer.data();
    unsigned char* uPlane = yPlane + width * height;
    unsigned char* vPlane = uPlane + (width * height) / 4;
    
    // Convert RGB to YUV420
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
            
            // Y plane
            yPlane[j * width + i] = std::clamp(yVal, 0, 255);
            
            // Subsample UV
            if ((j % 2 == 0) && (i % 2 == 0)) {
                int uvIndex = (j / 2) * (width / 2) + (i / 2);
                uPlane[uvIndex] = std::clamp(uVal, 0, 255);
                vPlane[uvIndex] = std::clamp(vVal, 0, 255);
            }
        }
    }
    
    return true;
}

std::vector<unsigned char> H264Encoder::encode(const unsigned char* rgbData, int width, int height)
{
    if (!m_initialized || !m_encoder || !rgbData) {
        LOG_ERROR("H264Encoder: Not initialized for RGB encoding - initialized: {}, data: {}, encoder: {}", 
                  m_initialized, static_cast<const void*>(rgbData), 
                  static_cast<void*>(m_encoder));
        return {};
    }
    
    // Убираем спам логов для каждого кадра
    // LOG_DEBUG("H264Encoder: RGB encoding - size: {}x{}", width, height);
    
    // Convert RGB to YUV420
    if (!convertRGBToYUV(rgbData, width, height)) {
        LOG_ERROR("H264Encoder: RGB to YUV conversion failed");
        return {};
    }
    
    // Убираем спам логов для каждого кадра
    // LOG_DEBUG("H264Encoder: RGB to YUV conversion successful");
    
    // Prepare input frame
    SSourcePicture pic;
    memset(&pic, 0, sizeof(SSourcePicture));
    pic.iPicWidth = width;
    pic.iPicHeight = height;
    pic.iColorFormat = videoFormatI420;
    pic.iStride[0] = width;
    pic.iStride[1] = width / 2;
    pic.iStride[2] = width / 2;
    pic.pData[0] = m_yuvBuffer.data();
    pic.pData[1] = m_yuvBuffer.data() + width * height;
    pic.pData[2] = m_yuvBuffer.data() + width * height + (width * height) / 4;
    
    // Encode frame
    SFrameBSInfo info;
    memset(&info, 0, sizeof(SFrameBSInfo));
    
    int result = m_encoder->EncodeFrame(&pic, &info);
    if (result != 0) {
        LOG_ERROR("H264Encoder: Failed to encode frame, error: {}", result);
        return {};
    }
    
    m_frameNumber++;
    
    if (info.iFrameSizeInBytes <= 0) {
        // Убираем спам логов для каждого кадра
        // LOG_DEBUG("H264Encoder: No output frame (delayed encoding)");
        return {};
    }
    
    // Collect NAL units - try without adding start codes first
    std::vector<unsigned char> h264Data;
    
    // Use the frame buffer directly if available
    if (info.sLayerInfo[0].pBsBuf && info.iFrameSizeInBytes > 0) {
        // Direct copy - OpenH264 might already include start codes
        h264Data.assign(info.sLayerInfo[0].pBsBuf, 
                       info.sLayerInfo[0].pBsBuf + info.iFrameSizeInBytes);
    } else {
        // Fallback: collect from individual layers
        for (int layer = 0; layer < info.iLayerNum; ++layer) {
            SLayerBSInfo* layerInfo = &info.sLayerInfo[layer];
            if (layerInfo->pBsBuf && layerInfo->iNalCount > 0) {
                unsigned char* currentPos = layerInfo->pBsBuf;
                for (int nal = 0; nal < layerInfo->iNalCount; ++nal) {
                    int nalSize = layerInfo->pNalLengthInByte[nal];
                    if (nalSize > 0) {
                        h264Data.insert(h264Data.end(), currentPos, currentPos + nalSize);
                        currentPos += nalSize;
                    }
                }
            }
        }
    }
    
    // Убираем спам логов для каждого кадра - логируем только при проблемах
    // LOG_DEBUG("H264Encoder: Encoding completed, result size: {}", h264Data.size());
    
    // Убираем спам логов для каждого кадра - логируем только при проблемах
    // if (h264Data.size() >= 8) {
    //     bool hasStartCode = (h264Data[0] == 0x00 && h264Data[1] == 0x00 && 
    //                         h264Data[2] == 0x00 && h264Data[3] == 0x01);
    //     LOG_DEBUG("H264Encoder: First 8 bytes: {:02x} {:02x} {:02x} {:02x} {:02x} {:02x} {:02x} {:02x} (has start code: {})", 
    //               h264Data[0], h264Data[1], h264Data[2], h264Data[3],
    //               h264Data[4], h264Data[5], h264Data[6], h264Data[7], hasStartCode);
    // }
    
    // Убираем спам логов о размере каждого кадра
    // std::stringstream ss;
    // ss << "H.264 encoded: " << h264Data.size() << " bytes | "
    //     << std::fixed << std::setprecision(2)
    //     << (h264Data.size() / 1024.0) << " KB | "
    //     << (h264Data.size() / (1024.0 * 1024.0)) << " MB";
    // LOG_INFO("{}", ss.str());
    
    return h264Data;
}

void H264Encoder::setQualityPreset(const std::string& quality) {
    if (quality == "high") {
        m_minQp = H264_QUALITY_MIN_QP_BEST;
        m_maxQp = H264_QUALITY_MAX_QP_BEST;
        m_bitrate = H264_BITRATE_HIGH;
    } else if (quality == "medium") {
        m_minQp = H264_QUALITY_MIN_QP_BALANCED;
        m_maxQp = H264_QUALITY_MAX_QP_BALANCED;
        m_bitrate = H264_BITRATE_MEDIUM;
    } else if (quality == "low") {
        m_minQp = H264_QUALITY_MIN_QP_FAST;
        m_maxQp = H264_QUALITY_MAX_QP_FAST;
        m_bitrate = H264_BITRATE_LOW;
    } else {
        LOG_WARN("H264Encoder: Unknown quality preset '{}', using medium", quality);
        setQualityPreset("medium");
        return;
    }
    
    LOG_INFO("H264Encoder: Set quality preset '{}' - QP: {}-{}, Bitrate: {} Mbps", 
             quality, m_minQp, m_maxQp, m_bitrate / 1000000.0);
}

void H264Encoder::setPerformanceMode(const std::string& mode) {
    if (mode == "auto") {
        // Автоматическое определение на основе количества ядер
        m_threads = autoDetectThreadCount();
        // Для автоматического режима используем CABAC для лучшего качества
        m_entropyMode = H264_ENTROPY_BEST;
    } else if (mode == "best") {
        m_threads = H264_THREADS_BEST;      // 1 поток
        m_entropyMode = H264_ENTROPY_BEST; // CABAC
    } else if (mode == "balanced") {
        m_threads = H264_THREADS_BALANCED; // 2 потока
        m_entropyMode = H264_ENTROPY_BEST; // CABAC
    } else if (mode == "fast") {
        m_threads = H264_THREADS_FAST;     // 4+ потока
        m_entropyMode = H264_ENTROPY_FAST; // CAVLC
    } else {
        LOG_WARN("H264Encoder: Unknown performance mode '{}', using auto", mode);
        setPerformanceMode("auto");
        return;
    }
    
    LOG_INFO("H264Encoder: Set performance mode '{}' - threads: {}, entropy: {}", 
             mode, m_threads, m_entropyMode == 1 ? "CABAC" : "CAVLC");
}

int H264Encoder::autoDetectThreadCount() {
    // Получаем количество логических ядер процессора
    unsigned int numCores = std::thread::hardware_concurrency();
    
    if (numCores == 0) {
        LOG_WARN("H264Encoder: Could not detect CPU core count, using default 2 threads");
        return 2; // Значение по умолчанию
    }
    
    // Оптимальное количество потоков для H.264 кодирования
    int optimalThreads;
    
    if (numCores <= 2) {
        // Для слабых систем - 1 поток для максимального качества
        optimalThreads = 1;
    } else if (numCores <= 4) {
        // Для систем с 2-4 ядрами - 2 потока
        optimalThreads = 2;
    } else if (numCores <= 8) {
        // Для систем с 4-8 ядрами - 4 потока
        optimalThreads = 4;
    } else {
        // Для мощных систем - 6 потоков (больше не всегда лучше)
        optimalThreads = 6;
    }
    
    LOG_INFO("H264Encoder: Auto-detected {} CPU cores, using {} threads for H.264 encoding", 
             numCores, optimalThreads);
    
    return optimalThreads;
}

void H264Encoder::setQualityParameters(int minQp, int maxQp, int bitrate) {
    m_minQp = std::clamp(minQp, 0, 51);
    m_maxQp = std::clamp(maxQp, m_minQp, 51);
    m_bitrate = std::max(bitrate, 100000); // Minimum 100 kbps
    
    LOG_INFO("H264Encoder: Set custom quality - QP: {}-{}, Bitrate: {} Mbps", 
             m_minQp, m_maxQp, m_bitrate / 1000000.0);
}

std::vector<unsigned char> H264Encoder::encode(const QImage& image)
{
    if (!m_initialized) {
        LOG_ERROR("H264Encoder: Not initialized");
        return {};
    }
    
    // Use hardware encoder if available
    if (m_hardwareEncoder) {
        return m_hardwareEncoder->encode(image);
    }
    
    // Убираем спам логов для каждого кадра
    // LOG_DEBUG("H264Encoder: Encoding frame - input size: {}x{}", image.width(), image.height());
    
    // Convert QImage to RGB24 format
    QImage rgbImage = image.convertToFormat(QImage::Format_RGB888);
    if (rgbImage.isNull()) {
        LOG_ERROR("H264Encoder: Failed to convert image to RGB888");
        return {};
    }
    
    // Encode RGB data (без спам логов для каждого кадра)
    return encode(rgbImage.bits(), rgbImage.width(), rgbImage.height());
}

void H264Encoder::cleanup()
{
    if (!m_initialized) {
        return;
    }
    
    // Cleanup hardware encoder first
    if (m_hardwareEncoder) {
        m_hardwareEncoder.reset();
    }
    
    // Cleanup software encoder
    if (m_encoder) {
        m_encoder->Uninitialize();
        WelsDestroySVCEncoder(m_encoder);
        m_encoder = nullptr;
    }
    
    m_yuvBuffer.clear();
    m_initialized = false;
    m_width = 0;
    m_height = 0;
    m_frameNumber = 0;
    
    LOG_INFO("H264Encoder: Cleanup completed");
}