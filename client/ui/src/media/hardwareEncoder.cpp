#include "hardwareEncoder.h"
#include "utilities/logger.h"

#ifdef ENABLE_NVENC
#include <cuda_runtime.h>
#include <nvEncodeAPI.h>
#include <d3d11.h>
#endif

#ifdef ENABLE_QSV
#include <mfxvideo++.h>
#include <windows.h>
#endif

#include <sstream>

HardwareEncoder::HardwareEncoder()
    : m_initialized(false)
    , m_type(EncoderType::NONE)
    , m_width(0)
    , m_height(0)
    , m_bitrate(0)
    , m_fps(0)
    , m_frameNumber(0)
#ifdef ENABLE_NVENC
    , m_nvenc(nullptr)
    , m_encoder(nullptr)
    , m_inputBuffer(nullptr)
    , m_outputBuffer(nullptr)
    , m_cudaContext(nullptr)
    , m_cudaDevice(0)
#endif
#ifdef ENABLE_QSV
    , m_session(nullptr)
    , m_surface(nullptr)
#endif
{
}

HardwareEncoder::~HardwareEncoder() {
    cleanup();
}

bool HardwareEncoder::initialize(EncoderType type, int width, int height, int bitrate, int fps) {
    if (m_initialized) {
        LOG_WARN("HardwareEncoder: Already initialized");
        return true;
    }
    
    cleanup();
    
    m_type = type;
    m_width = width;
    m_height = height;
    m_bitrate = bitrate;
    m_fps = fps;
    m_frameNumber = 0;
    
    LOG_INFO("HardwareEncoder: Initializing {} encoder {}x{} @ {}fps, bitrate: {} Mbps", 
             getEncoderName(type), width, height, fps, bitrate / 1000000.0);
    
    switch (type) {
#ifdef ENABLE_NVENC
        case EncoderType::NVENC:
            return initializeNVENC();
#endif
#ifdef ENABLE_QSV
        case EncoderType::QSV:
            return initializeQSV();
#endif
        default:
            LOG_ERROR("HardwareEncoder: Unsupported encoder type");
            return false;
    }
}

std::vector<unsigned char> HardwareEncoder::encode(const QImage& image) {
    if (!m_initialized) {
        LOG_ERROR("HardwareEncoder: Not initialized");
        return {};
    }
    
    switch (m_type) {
#ifdef ENABLE_NVENC
        case EncoderType::NVENC:
            return encodeFrameNVENC(image);
#endif
#ifdef ENABLE_QSV
        case EncoderType::QSV:
            return encodeFrameQSV(image);
#endif
        default:
            LOG_ERROR("HardwareEncoder: Invalid encoder type");
            return {};
    }
}

std::vector<HardwareEncoder::EncoderType> HardwareEncoder::getAvailableEncoders() {
    std::vector<EncoderType> encoders;
    
#ifdef ENABLE_NVENC
    // Check NVENC availability
    int deviceCount = 0;
    if (cudaGetDeviceCount(&deviceCount) == cudaSuccess && deviceCount > 0) {
        encoders.push_back(EncoderType::NVENC);
        LOG_INFO("HardwareEncoder: NVENC available ({} CUDA devices)", deviceCount);
    }
#endif

#ifdef ENABLE_QSV
    // Check QSV availability
    mfxVersion version;
    mfxSession session;
    if (MFXInit(MFX_IMPL_HARDWARE_ANY, &version, &session) == MFX_ERR_NONE) {
        encoders.push_back(EncoderType::QSV);
        LOG_INFO("HardwareEncoder: Intel Quick Sync Video available (API version {}.{}), 
                 version.Major, version.Minor);
        MFXClose(session);
    }
#endif

    if (encoders.empty()) {
        LOG_INFO("HardwareEncoder: No hardware encoders available");
    }
    
    return encoders;
}

std::string HardwareEncoder::getEncoderName(EncoderType type) {
    switch (type) {
        case EncoderType::NVENC: return "NVENC";
        case EncoderType::QSV: return "Intel QSV";
        default: return "None";
    }
}

#ifdef ENABLE_NVENC
bool HardwareEncoder::initializeNVENC() {
    LOG_INFO("HardwareEncoder: Initializing NVENC...");
    
    // Initialize CUDA
    if (cudaGetDevice(&m_cudaDevice) != cudaSuccess) {
        LOG_ERROR("HardwareEncoder: Failed to get CUDA device");
        return false;
    }
    
    if (cudaSetDevice(m_cudaDevice) != cudaSuccess) {
        LOG_ERROR("HardwareEncoder: Failed to set CUDA device");
        return false;
    }
    
    if (cudaCtxCreate(&m_cudaContext, 0, m_cudaDevice) != cudaSuccess) {
        LOG_ERROR("HardwareEncoder: Failed to create CUDA context");
        return false;
    }
    
    // Load NVENC library
    HMODULE nvencDll = LoadLibrary(L"nvEncodeAPI64.dll");
    if (!nvencDll) {
        LOG_ERROR("HardwareEncoder: Failed to load NVENC library");
        return false;
    }
    
    auto createInstance = (NV_ENCODE_API_FUNCTION_LIST* (*)())GetProcAddress(nvencDll, "NvEncodeAPICreateInstance");
    if (!createInstance) {
        LOG_ERROR("HardwareEncoder: Failed to get NVENC create function");
        FreeLibrary(nvencDll);
        return false;
    }
    
    m_nvenc = createInstance();
    if (!m_nvenc) {
        LOG_ERROR("HardwareEncoder: Failed to create NVENC instance");
        FreeLibrary(nvencDll);
        return false;
    }
    
    // Create encoder
    NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS sessionParams = {};
    sessionParams.version = NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER;
    sessionParams.deviceType = NV_ENC_DEVICE_TYPE_CUDA;
    sessionParams.device = m_cudaContext;
    sessionParams.apiVersion = NVENCAPI_VERSION;
    
    if (m_nvenc->NvEncOpenEncodeSessionEx(&sessionParams, &m_encoder) != NV_ENC_SUCCESS) {
        LOG_ERROR("HardwareEncoder: Failed to open NVENC session");
        return false;
    }
    
    // Configure encoder
    NV_ENC_INITIALIZE_PARAMS initParams = {};
    initParams.version = NV_ENC_INITIALIZE_PARAMS_VER;
    initParams.encodeGUID = NV_ENC_CODEC_H264_GUID;
    initParams.presetGUID = NV_ENC_PRESET_LOW_LATENCY_HQ_GUID;
    initParams.encodeWidth = m_width;
    initParams.encodeHeight = m_height;
    initParams.darWidth = m_width;
    initParams.darHeight = m_height;
    initParams.frameRateNum = m_fps;
    initParams.frameRateDen = 1;
    initParams.enableEncodeAsync = 0;
    initParams.enablePTD = 1;
    
    NV_ENC_CONFIG config = {};
    config.version = NV_ENC_CONFIG_VER;
    config.profileGUID = NV_ENC_H264_PROFILE_HIGH_GUID;
    config.rcParams.version = NV_ENC_RC_PARAMS_VER;
    config.rcParams.averageBitRate = m_bitrate;
    config.rcParams.maxBitRate = m_bitrate * 2;
    config.rcParams.rateControlMode = NV_ENC_PARAMS_RC_CBR;
    config.rcParams.vbvBufferSize = m_bitrate / m_fps;
    config.rcParams.vbvInitialDelay = config.rcParams.vbvBufferSize / 2;
    
    initParams.encodeConfig = &config;
    
    if (m_nvenc->NvEncInitializeEncoder(m_encoder, &initParams) != NV_ENC_SUCCESS) {
        LOG_ERROR("HardwareEncoder: Failed to initialize NVENC encoder");
        return false;
    }
    
    // Create input/output buffers
    NV_ENC_CREATE_INPUT_BUFFER inputBufferParams = {};
    inputBufferParams.version = NV_ENC_CREATE_INPUT_BUFFER_VER;
    inputBufferParams.width = m_width;
    inputBufferParams.height = m_height;
    inputBufferParams.bufferFmt = NV_ENC_BUFFER_FORMAT_NV12;
    
    if (m_nvenc->NvEncCreateInputBuffer(m_encoder, &inputBufferParams) != NV_ENC_SUCCESS) {
        LOG_ERROR("HardwareEncoder: Failed to create NVENC input buffer");
        return false;
    }
    m_inputBuffer = inputBufferParams.inputBuffer;
    
    NV_ENC_CREATE_BITSTREAM_BUFFER outputBufferParams = {};
    outputBufferParams.version = NV_ENC_CREATE_BITSTREAM_BUFFER_VER;
    outputBufferParams.size = m_width * m_height * 2; // Estimated size
    
    if (m_nvenc->NvEncCreateBitstreamBuffer(m_encoder, &outputBufferParams) != NV_ENC_SUCCESS) {
        LOG_ERROR("HardwareEncoder: Failed to create NVENC output buffer");
        return false;
    }
    m_outputBuffer = outputBufferParams.bitstreamBuffer;
    
    m_initialized = true;
    LOG_INFO("HardwareEncoder: NVENC initialized successfully");
    return true;
}

std::vector<unsigned char> HardwareEncoder::encodeFrameNVENC(const QImage& image) {
    if (!m_initialized || !m_nvenc || !m_encoder) {
        return {};
    }
    
    // Convert QImage to NV12 format and upload to CUDA
    // This is a simplified version - in production you'd need proper NV12 conversion
    NV_ENC_LOCK_INPUT_BUFFER lockParams = {};
    lockParams.version = NV_ENC_LOCK_INPUT_BUFFER_VER;
    lockParams.inputBuffer = m_inputBuffer;
    
    if (m_nvenc->NvEncLockInputBuffer(m_encoder, &lockParams) != NV_ENC_SUCCESS) {
        LOG_ERROR("HardwareEncoder: Failed to lock NVENC input buffer");
        return {};
    }
    
    // Convert RGB to NV12 (simplified)
    QImage rgbImage = image.convertToFormat(QImage::Format_RGB888);
    // TODO: Proper RGB to NV12 conversion here
    
    m_nvenc->NvEncUnlockInputBuffer(m_encoder, m_inputBuffer);
    
    // Encode frame
    NV_ENC_PIC_PARAMS picParams = {};
    picParams.version = NV_ENC_PIC_PARAMS_VER;
    picParams.inputBuffer = m_inputBuffer;
    picParams.outputBitstream = m_outputBuffer;
    picParams.inputWidth = m_width;
    picParams.inputHeight = m_height;
    picParams.inputPitch = m_width;
    picParams.frameIdx = m_frameNumber++;
    picParams.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
    
    if (m_nvenc->NvEncEncodePicture(m_encoder, &picParams) != NV_ENC_SUCCESS) {
        LOG_ERROR("HardwareEncoder: Failed to encode NVENC frame");
        return {};
    }
    
    // Get output
    NV_ENC_LOCK_BITSTREAM lockOutputParams = {};
    lockOutputParams.version = NV_ENC_LOCK_BITSTREAM_VER;
    lockOutputParams.outputBitstream = m_outputBuffer;
    
    if (m_nvenc->NvEncLockBitstream(m_encoder, &lockOutputParams) != NV_ENC_SUCCESS) {
        LOG_ERROR("HardwareEncoder: Failed to lock NVENC output buffer");
        return {};
    }
    
    std::vector<unsigned char> output;
    output.resize(lockOutputParams.bitstreamSizeInBytes);
    memcpy(output.data(), lockOutputParams.bitstreamBufferPtr, lockOutputParams.bitstreamSizeInBytes);
    
    m_nvenc->NvEncUnlockBitstream(m_encoder, m_outputBuffer);
    
    return output;
}

void HardwareEncoder::cleanupNVENC() {
    if (m_nvenc && m_encoder) {
        if (m_inputBuffer) {
            m_nvenc->NvEncDestroyInputBuffer(m_encoder, m_inputBuffer);
            m_inputBuffer = nullptr;
        }
        
        if (m_outputBuffer) {
            m_nvenc->NvEncDestroyBitstreamBuffer(m_encoder, m_outputBuffer);
            m_outputBuffer = nullptr;
        }
        
        m_nvenc->NvEncDestroyEncoder(m_encoder);
        m_encoder = nullptr;
    }
    
    if (m_cudaContext) {
        cudaCtxDestroy(m_cudaContext);
        m_cudaContext = nullptr;
    }
}
#endif

#ifdef ENABLE_QSV
bool HardwareEncoder::initializeQSV() {
    LOG_INFO("HardwareEncoder: Initializing Intel Quick Sync Video...");
    
    // Initialize Media SDK session
    mfxVersion version;
    if (MFXInit(MFX_IMPL_HARDWARE_ANY, &version, &m_session) != MFX_ERR_NONE) {
        LOG_ERROR("HardwareEncoder: Failed to initialize Media SDK session");
        return false;
    }
    
    // Query implementation capabilities
    mfxIMPL impl;
    if (MFXQueryIMPL(m_session, &impl) == MFX_ERR_NONE) {
        LOG_INFO("HardwareEncoder: Media SDK implementation: {}", impl);
    }
    
    // Configure video parameters
    memset(&m_params, 0, sizeof(m_params));
    m_params.mfx.CodecId = MFX_CODEC_AVC;
    m_params.mfx.TargetUsage = MFX_TARGETUSAGE_BEST_SPEED;
    m_params.mfx.TargetKbps = m_bitrate / 1000;
    m_params.mfx.FrameInfo.FrameRateExtN = m_fps;
    m_params.mfx.FrameInfo.FrameRateExtD = 1;
    m_params.mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
    m_params.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
    m_params.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    m_params.mfx.FrameInfo.Width = m_width;
    m_params.mfx.FrameInfo.Height = m_height;
    m_params.mfx.FrameInfo.CropX = 0;
    m_params.mfx.CropW = m_width;
    m_params.mfx.FrameInfo.CropY = 0;
    m_params.mfx.CropH = m_height;
    
    // Configure encoding parameters
    m_params.IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY;
    
    // Initialize encoder
    if (MFXVideoENCODE enc(m_session); enc.Init(&m_params) != MFX_ERR_NONE) {
        LOG_ERROR("HardwareEncoder: Failed to initialize QSV encoder");
        return false;
    }
    
    // Allocate surface
    m_surface = new mfxFrameSurface1();
    memset(m_surface, 0, sizeof(mfxFrameSurface1));
    m_surface->Info = m_params.mfx.FrameInfo;
    
    // Allocate bitstream
    m_bitstream.BufferSize = m_width * m_height * 2;
    m_bitstream.Data = new mfxU8[m_bitstream.BufferSize];
    memset(&m_bitstream, 0, sizeof(m_bitstream));
    
    m_initialized = true;
    LOG_INFO("HardwareEncoder: Intel QSV initialized successfully");
    return true;
}

std::vector<unsigned char> HardwareEncoder::encodeFrameQSV(const QImage& image) {
    if (!m_initialized || !m_surface) {
        return {};
    }
    
    // Convert QImage to NV12 and copy to surface
    // This is simplified - proper implementation needed
    QImage rgbImage = image.convertToFormat(QImage::Format_RGB888);
    // TODO: Convert RGB to NV12 and copy to m_surface->Data
    
    mfxSyncPoint syncp;
    MFXVideoENCODE enc(m_session);
    
    if (enc.EncodeFrameAsync(nullptr, m_surface, &m_bitstream, &syncp) != MFX_ERR_NONE) {
        LOG_ERROR("HardwareEncoder: Failed to encode QSV frame");
        return {};
    }
    
    // Wait for completion
    MFXVideoCORE core(m_session);
    core.SyncOperation(syncp, 1000); // 1 second timeout
    
    std::vector<unsigned char> output;
    output.resize(m_bitstream.DataLength);
    memcpy(output.data(), m_bitstream.Data, m_bitstream.DataLength);
    
    return output;
}

void HardwareEncoder::cleanupQSV() {
    if (m_surface) {
        delete m_surface;
        m_surface = nullptr;
    }
    
    if (m_bitstream.Data) {
        delete[] m_bitstream.Data;
        m_bitstream.Data = nullptr;
    }
    
    if (m_session) {
        MFXClose(m_session);
        m_session = nullptr;
    }
}
#endif

void HardwareEncoder::cleanup() {
    if (!m_initialized) {
        return;
    }
    
    switch (m_type) {
#ifdef ENABLE_NVENC
        case EncoderType::NVENC:
            cleanupNVENC();
            break;
#endif
#ifdef ENABLE_QSV
        case EncoderType::QSV:
            cleanupQSV();
            break;
#endif
        default:
            break;
    }
    
    m_initialized = false;
    m_type = EncoderType::NONE;
}
