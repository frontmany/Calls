#pragma once

#include <vector>
#include <memory>
#include <QImage>

extern "C" {
#include "codec_api.h"
#include "codec_app_def.h"
#include "codec_def.h"
}

class H264Decoder {
public:
    H264Decoder();
    ~H264Decoder();

    // Initialize decoder
    bool initialize();
    
    // Decode H.264 data to QImage
    // Input: H.264 encoded data
    // Output: decoded QImage
    QImage decode(const std::vector<unsigned char>& h264Data);
    
    // Check if decoder is initialized
    bool isInitialized() const { return m_initialized; }
    
    // Cleanup resources
    void cleanup();

private:
    bool m_initialized;
    ISVCDecoder* m_decoder;
    
    // Output buffer info
    unsigned char* m_yuvData[3];
    SBufferInfo m_bufferInfo;
    
    // Convert YUV to RGB
    QImage convertYUVToRGB(const unsigned char* yData, const unsigned char* uData, const unsigned char* vData, 
                          int width, int height);
    
    // Allocate output buffers
    bool allocateOutputBuffers(int width, int height);
    
    // Free output buffers
    void freeOutputBuffers();
};
