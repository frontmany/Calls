#pragma once

#include <vector>
#include <memory>
#include <QImage>

extern "C" {
#include "codec_api.h"
#include "codec_app_def.h"
#include "codec_def.h"
}

class H264Encoder {
public:
    H264Encoder();
    ~H264Encoder();

    // Initialize encoder with target resolution
    bool initialize(int width, int height, int bitrate = 2000000, int fps = 30, int keyframeInterval = 30);
    
    // Encode RGB image data to H.264
    // Input: RGB data (width * height * 3 bytes), width, height
    // Output: encoded H.264 data
    std::vector<unsigned char> encode(const unsigned char* rgbData, int width, int height);
    
    // Encode QImage to H.264
    std::vector<unsigned char> encode(const QImage& image);
    
    // Check if encoder is initialized
    bool isInitialized() const { return m_initialized; }
    
    // Get current encoder dimensions
    QSize getCurrentSize() const { return QSize(m_width, m_height); }
    
    // Get current frame number
    int getFrameNumber() const { return m_frameNumber; }
    
    // Set quality preset (high/medium/low)
    void setQualityPreset(const std::string& quality);
    
    // Set performance mode (best/balanced/fast)
    void setPerformanceMode(const std::string& mode);
    
    // Set custom quality parameters
    void setQualityParameters(int minQp, int maxQp, int bitrate);
    
    // Auto-detect optimal thread count based on CPU cores
    int autoDetectThreadCount();
    
    // Cleanup resources
    void cleanup();

private:
    bool m_initialized;
    ISVCEncoder* m_encoder;
    
    int m_width;
    int m_height;
    int m_frameNumber;
    int m_bitrate;
    int m_fps;
    int m_keyframeInterval;
    int m_minQp;    // Минимальный QP (качество)
    int m_maxQp;    // Максимальный QP (качество)
    int m_threads;  // Количество потоков
    int m_entropyMode; // Режим энтропии (0=CAVLC, 1=CABAC)
    
    // YUV data buffers
    std::vector<unsigned char> m_yuvBuffer;
    
    // Convert RGB to YUV420
    bool convertRGBToYUV(const unsigned char* rgbData, int width, int height);
    
    // Setup encoder parameters
    void setupEncoderParams(SEncParamExt& params);
};
