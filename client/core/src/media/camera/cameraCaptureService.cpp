#include "cameraCaptureService.h"
#include <iostream>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#include <dshow.h>
#elif defined(__APPLE__)
#include <AVFoundation/AVFoundation.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#endif

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
}

static bool g_devicesRegistered = false;
static void registerDevices() {
    if (!g_devicesRegistered) {
#ifdef _WIN32
        // Initialize COM for DirectShow
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#endif
        avdevice_register_all();
        g_devicesRegistered = true;
        std::cout << "AV devices registered successfully" << std::endl;
    }
}

namespace core::media
{
    CameraCaptureService::CameraCaptureService()
        : m_formatContext(nullptr)
        , m_codecContext(nullptr)
        , m_frame(nullptr)
        , m_frameRGB(nullptr)
        , m_swsContext(nullptr)
        , m_buffer(nullptr)
        , m_isRunning(false)
        , m_shouldStop(false)
        , m_frameWidth(0)
        , m_frameHeight(0)
        , m_captureThread(nullptr)
    {
        registerDevices();
    }

    CameraCaptureService::~CameraCaptureService()
    {
        stop();
    }

    bool CameraCaptureService::start(const char* deviceName)
    {
        if (m_isRunning) {
            std::cerr << "Camera capture is already running" << std::endl;
            return false;
        }

        if (!initializeCamera(deviceName)) {
            return false;
        }

        m_shouldStop = false;
        m_isRunning = true;

        m_captureThread = new std::thread([this]() {
            while (!m_shouldStop && m_isRunning) {
                captureFrame();
                std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30 FPS
            }
            });

        std::cout << "Camera capture started successfully" << std::endl;
        return true;
    }

    void CameraCaptureService::stop()
    {
        if (!m_isRunning) {
            return;
        }

        m_shouldStop = true;
        m_isRunning = false;

        if (m_captureThread) {
            static_cast<std::thread*>(m_captureThread)->join();
            delete static_cast<std::thread*>(m_captureThread);
            m_captureThread = nullptr;
        }

        cleanup();
        std::cout << "Camera capture stopped" << std::endl;
    }

    bool CameraCaptureService::isRunning() const
    {
        return m_isRunning;
    }

    void CameraCaptureService::setFrameCallback(FrameCallback callback)
    {
        m_frameCallback = callback;
    }

    bool CameraCaptureService::getAvailableDevices(char devices[][256], int maxDevices, int& deviceCount)
    {
        deviceCount = 0;

#ifdef _WIN32
        ICreateDevEnum* pDevEnum = nullptr;
        IEnumMoniker* pEnum = nullptr;
        IMoniker* pMoniker = nullptr;

        HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER,
            IID_ICreateDevEnum, (void**)&pDevEnum);
        if (FAILED(hr)) return false;

        hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);
        if (FAILED(hr)) {
            pDevEnum->Release();
            return false;
        }

        while (pEnum->Next(1, &pMoniker, nullptr) == S_OK && deviceCount < maxDevices) {
            IPropertyBag* pPropBag;
            hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropBag);
            if (SUCCEEDED(hr)) {
                VARIANT var;
                VariantInit(&var);
                hr = pPropBag->Read(L"FriendlyName", &var, 0);
                if (SUCCEEDED(hr)) {
                    WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1,
                        devices[deviceCount], 256, nullptr, nullptr);

                    char dshowName[300];
                    snprintf(dshowName, sizeof(dshowName), "video=%s", devices[deviceCount]);
                    strncpy(devices[deviceCount], dshowName, 255);
                    devices[deviceCount][255] = '\0';

                    deviceCount++;
                    VariantClear(&var);
                }
                pPropBag->Release();
            }
            pMoniker->Release();
        }

        pEnum->Release();
        pDevEnum->Release();

#elif defined(__APPLE__)
        snprintf(devices[deviceCount], 256, "0");
        deviceCount++;
        if (deviceCount < maxDevices) {
            snprintf(devices[deviceCount], 256, "1");
            deviceCount++;
        }
#else
        for (int i = 0; i < maxDevices; ++i) {
            char device[32];
            snprintf(device, sizeof(device), "/dev/video%d", i);

            int fd = open(device, O_RDWR);
            if (fd >= 0) {
                struct v4l2_capability cap;
                if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == 0) {
                    snprintf(devices[deviceCount], 256, "%s", device);
                    deviceCount++;
                }
                close(fd);
            }
            else {
                break;
            }
        }
#endif

        return deviceCount > 0;
    }

    bool CameraCaptureService::initializeCamera(const char* deviceName)
    {
        const char* device = deviceName;
        if (!device) {
            char devices[10][256];
            int deviceCount = 0;

            if (getAvailableDevices(devices, 10, deviceCount) && deviceCount > 0) {
                device = devices[0];
                std::cout << "Auto-detected camera: " << device << std::endl;
            }
            else {

#ifdef _WIN32
                device = "default";
#elif defined(__APPLE__)
                device = "0";
#else
                device = "/dev/video0";
#endif
            }
        }

        m_formatContext = avformat_alloc_context();
        if (!m_formatContext) {
            std::cerr << "Failed to allocate format context" << std::endl;
            return false;
        }

        const AVInputFormat* inputFormat = nullptr;
#ifdef _WIN32
        inputFormat = av_find_input_format("dshow");
#elif defined(__APPLE__)
        inputFormat = av_find_input_format("avfoundation");
#else
        inputFormat = av_find_input_format("v4l2");
#endif

        if (!inputFormat) {
            std::cerr << "Failed to find camera input format" << std::endl;
            cleanup();
            return false;
        }

        AVDictionary* options = nullptr;
        av_dict_set(&options, "framerate", "30", 0);
        av_dict_set(&options, "video_size", "640x480", 0);
#ifdef __APPLE__
        av_dict_set(&options, "pixel_format", "bgr0", 0); // macOS uses BGRA
#endif

        std::cout << "Attempting to open device: " << device << std::endl;
        if (avformat_open_input(&m_formatContext, device, inputFormat, &options) != 0) {
            std::cerr << "Failed to open camera device: " << device << std::endl;
            cleanup();
            return false;
        }

        if (avformat_find_stream_info(m_formatContext, nullptr) < 0) {
            std::cerr << "Failed to find stream information" << std::endl;
            cleanup();
            return false;
        }

        int videoStreamIndex = av_find_best_stream(m_formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        if (videoStreamIndex < 0) {
            std::cerr << "Failed to find video stream" << std::endl;
            cleanup();
            return false;
        }

        AVCodecParameters* codecParams = m_formatContext->streams[videoStreamIndex]->codecpar;

        const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
        if (!codec) {
            std::cerr << "Failed to find codec" << std::endl;
            cleanup();
            return false;
        }

        m_codecContext = avcodec_alloc_context3(codec);
        if (!m_codecContext) {
            std::cerr << "Failed to allocate codec context" << std::endl;
            cleanup();
            return false;
        }

        if (avcodec_parameters_to_context(m_codecContext, codecParams) < 0) {
            std::cerr << "Failed to copy codec parameters" << std::endl;
            cleanup();
            return false;
        }

        if (avcodec_open2(m_codecContext, codec, nullptr) < 0) {
            std::cerr << "Failed to open codec" << std::endl;
            cleanup();
            return false;
        }


        m_frame = av_frame_alloc();
        m_frameRGB = av_frame_alloc();
        if (!m_frame || !m_frameRGB) {
            std::cerr << "Failed to allocate frames" << std::endl;
            cleanup();
            return false;
        }

        int width = m_codecContext->width;
        int height = m_codecContext->height;

        std::cout << "Codec context dimensions: " << width << "x" << height << std::endl;

        if (width <= 0 || height <= 0) {
            AVStream* videoStream = m_formatContext->streams[videoStreamIndex];
            if (videoStream && videoStream->codecpar) {
                width = videoStream->codecpar->width;
                height = videoStream->codecpar->height;
                std::cout << "Stream dimensions: " << width << "x" << height << std::endl;
            }
        }

        if (width <= 0 || height <= 0) {
            width = 640;
            height = 480;
            std::cout << "Using default dimensions: " << width << "x" << height << std::endl;
        }

        m_frameWidth = width;
        m_frameHeight = height;

        int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width, height, 1);
        m_buffer = (uint8_t*)av_malloc(bufferSize * sizeof(uint8_t));

        if (!m_buffer) {
            std::cerr << "Failed to allocate buffer" << std::endl;
            cleanup();
            return false;
        }

        av_image_fill_arrays(m_frameRGB->data, m_frameRGB->linesize, m_buffer,
            AV_PIX_FMT_RGB24, width, height, 1);

        m_swsContext = sws_getContext(width, height, m_codecContext->pix_fmt,
            width, height, AV_PIX_FMT_RGB24,
            SWS_BILINEAR, nullptr, nullptr, nullptr);

        if (!m_swsContext) {
            std::cerr << "Failed to initialize scale context" << std::endl;
            cleanup();
            return false;
        }

        return true;
    }

    void CameraCaptureService::captureFrame()
    {
        if (!m_formatContext || !m_codecContext || !m_frame) {
            return;
        }

        AVPacket* packet = av_packet_alloc();
        if (!packet) return;

        int ret = av_read_frame(m_formatContext, packet);
        if (ret < 0) {
            av_packet_free(&packet);
            return;
        }

        ret = avcodec_send_packet(m_codecContext, packet);
        av_packet_free(&packet);

        if (ret < 0) {
            return;
        }

        ret = avcodec_receive_frame(m_codecContext, m_frame);
        if (ret < 0) {
            return;
        }

        sws_scale(m_swsContext, m_frame->data, m_frame->linesize, 0, m_frame->height,
            m_frameRGB->data, m_frameRGB->linesize);

        Frame frameData;
        frameData.data = m_frameRGB->data[0];
        frameData.width = m_frameWidth;
        frameData.height = m_frameHeight;
        frameData.format = AV_PIX_FMT_RGB24;
        frameData.pts = m_frame->pts;
        frameData.size = av_image_get_buffer_size(AV_PIX_FMT_RGB24, m_frameWidth, m_frameHeight, 1);

        if (m_frameCallback) {
            m_frameCallback(frameData);
        }
    }

    void CameraCaptureService::cleanup()
    {
        if (m_swsContext) {
            sws_freeContext(m_swsContext);
            m_swsContext = nullptr;
        }

        if (m_buffer) {
            av_free(m_buffer);
            m_buffer = nullptr;
        }

        if (m_frameRGB) {
            av_frame_free(&m_frameRGB);
        }

        if (m_frame) {
            av_frame_free(&m_frame);
        }

        if (m_codecContext) {
            avcodec_free_context(&m_codecContext);
        }

        if (m_formatContext) {
            avformat_close_input(&m_formatContext);
            avformat_free_context(m_formatContext);
            m_formatContext = nullptr;
        }
    }
}