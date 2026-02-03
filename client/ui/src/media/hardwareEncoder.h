#pragma once

#include <vector>
#include <memory>
#include <QImage>

#ifdef ENABLE_NVENC
#include <cuda_runtime.h>
#include <nvEncodeAPI.h>
#endif

#ifdef ENABLE_QSV
#include <mfxvideo++.h>
#endif

class HardwareEncoder {
public:
    enum class EncoderType {
        NONE,
        NVENC,
        QSV
    };

    HardwareEncoder();
    ~HardwareEncoder();

    // Initialize hardware encoder
    bool initialize(EncoderType type, int width, int height, int bitrate = 2000000, int fps = 30);
    
    // Encode QImage to H.264
    std::vector<unsigned char> encode(const QImage& image);
    
    // Check if encoder is initialized
    bool isInitialized() const { return m_initialized; }
    
    // Get available hardware encoders
    static std::vector<EncoderType> getAvailableEncoders();
    
    // Get encoder name
    static std::string getEncoderName(EncoderType type);

private:
    bool m_initialized;
    EncoderType m_type;
    int m_width;
    int m_height;
    int m_bitrate;
    int m_fps;
    int m_frameNumber;

#ifdef ENABLE_NVENC
    // NVENC specific members
    NV_ENCODE_API_FUNCTION_LIST* m_nvenc;
    void* m_encoder;
    void* m_inputBuffer;
    void* m_outputBuffer;
    CUcontext m_cudaContext;
    CUdevice m_cudaDevice;
    
    bool initializeNVENC();
    bool encodeFrameNVENC(const QImage& image);
    void cleanupNVENC();
#endif

#ifdef ENABLE_QSV
    // QSV specific members
    mfxSession m_session;
    mfxVideoParam m_params;
    mfxFrameAllocator m_allocator;
    mfxFrameSurface1* m_surface;
    mfxBitstream m_bitstream;
    
    bool initializeQSV();
    bool encodeFrameQSV(const QImage& image);
    void cleanupQSV();
#endif

    void cleanup();
};
