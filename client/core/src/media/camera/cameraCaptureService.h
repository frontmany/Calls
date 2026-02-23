#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <cstdint>
#include <thread>
#include "../frame.h"

extern "C" {
struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct SwsContext;
}

namespace core::media
{
    class CameraCaptureService
    {
    public:
        using FrameCallback = std::function<void(const Frame&)>;
        using ErrorCallback = std::function<void()>;

        CameraCaptureService();
        ~CameraCaptureService();

        bool start(const char* deviceName = nullptr);
        void stop();
        bool isRunning() const;
        void setFrameCallback(FrameCallback callback);
        void setErrorCallback(ErrorCallback callback);
        static bool getAvailableDevices(char devices[][256], int maxDevices, int& deviceCount);

    private:
        void cleanup();
        bool initializeCamera(const char* deviceName);
        void captureFrame();

        AVFormatContext* m_formatContext;
        AVCodecContext* m_codecContext;
        AVFrame* m_frame;
        AVFrame* m_frameRGB;
        SwsContext* m_swsContext;
        uint8_t* m_buffer;
            
        FrameCallback m_frameCallback;
        ErrorCallback m_errorCallback;
        std::atomic_bool m_isRunning;
        std::atomic_bool m_shouldStop;
        std::atomic_bool m_threadSelfDeleted;
        int m_frameWidth;
        int m_frameHeight;
            
        std::thread* m_captureThread;
    };
}