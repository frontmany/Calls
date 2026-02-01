#pragma once

#include <vector>
#include <memory>
#include <QImage>

// Use libdav1d for decoding since SVT-AV1 only provides encoding
extern "C" {
#include <dav1d/dav1d.h>
}

class AV1Decoder {
public:
    AV1Decoder();
    ~AV1Decoder();

    // Initialize decoder
    bool initialize();
    
    // Decode AV1 data (IVF format) to QImage
    // Input: IVF formatted AV1 data
    // Output: decoded QImage (RGB888 format)
    QImage decode(const std::vector<unsigned char>& ivfData);
    QImage decode(const unsigned char* data, size_t size);
    
    // Check if decoder is initialized
    bool isInitialized() const { return m_initialized; }
    
    // Cleanup resources
    void cleanup();

private:
    bool m_initialized;
    Dav1dContext* m_context;
    Dav1dPicture* m_picture;
    Dav1dData* m_data;
    
    int m_width;
    int m_height;
    
    // Parse IVF frame header and extract AV1 data
    bool parseIVF(const unsigned char* ivfData, size_t size, Dav1dData* outputData);
    
    // Convert YUV420p to RGB
    QImage convertYUVToRGB(const Dav1dPicture* picture);
    
    // Decode packet to frame
    bool decodeData(Dav1dData* data);
};
