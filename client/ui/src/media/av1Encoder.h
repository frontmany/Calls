#pragma once

#include <vector>
#include <memory>
#include <QImage>

extern "C" {
#include <EbSvtAv1.h>
#include <EbSvtAv1Enc.h>
}

class AV1Encoder {
public:
    AV1Encoder();
    ~AV1Encoder();

    // Initialize encoder with target resolution
    bool initialize(int width, int height, int preset = 10, int crf = 32, int keyint = 30, double fps = 14.0);
    
    // Encode RGB image data to AV1
    // Input: RGB data (width * height * 3 bytes), width, height
    // Output: encoded AV1 data in IVF format
    std::vector<unsigned char> encode(const unsigned char* rgbData, int width, int height);
    
    // Encode QImage to AV1
    std::vector<unsigned char> encode(const QImage& image);
    
    // Check if encoder is initialized
    bool isInitialized() const { return m_initialized; }
    
    // Get current frame number (for keyframe detection)
    int getFrameNumber() const { return m_frameNumber; }
    
    // Cleanup resources
    void cleanup();

private:
    bool m_initialized;
    EbComponentType* m_encoderHandle;
    EbSvtAv1EncConfiguration m_config;
    EbBufferHeaderType* m_inputBuffer;
    EbBufferHeaderType* m_outputBuffer;
    std::vector<unsigned char> m_streamHeader;
    bool m_streamHeaderSent;
    
    int m_width;
    int m_height;
    int m_frameNumber;
    
    // YUV data pointers for EbSvtIOFormat
    uint8_t* m_yData;
    uint8_t* m_uData;
    uint8_t* m_vData;
    
    // Frame buffering for real-time encoding
    std::vector<int> m_bufferedFrames;
    static const int MIN_BUFFERED_FRAMES = 3; // Need at least 3 frames before getting packets
    
    // Convert RGB to YUV420p
    bool convertRGBToYUV(const unsigned char* rgbData, int width, int height);
    
    // Create IVF header
    std::vector<unsigned char> createIVFHeader() const;
    
    // Serialize packet to IVF format
    std::vector<unsigned char> packetToIVF(const EbBufferHeaderType* packet) const;
};
