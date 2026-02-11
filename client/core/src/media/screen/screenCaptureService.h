#pragma once

#include <functional>
#include <memory>
#include <cstdint>
#include "media/frame.h"

extern "C" {
struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct SwsContext;
}

namespace core::media
{
    class ScreenCaptureService
    {
    public:
        using FrameCallback = std::function<void(const Frame&)>;

        ScreenCaptureService();
        ~ScreenCaptureService();

        bool start(int screenIndex = 0); 
        void stop();
        bool isRunning() const;
        void setFrameCallback(FrameCallback callback);
        static int getScreenCount();

    private:
        void cleanup();
        bool initializeScreenCapture(int screenIndex);
        void captureFrame();

        AVFormatContext* m_formatContext;
        AVCodecContext* m_codecContext;
        AVFrame* m_frame;
        AVFrame* m_frameRGB;
        SwsContext* m_swsContext;
        uint8_t* m_buffer;
            
        FrameCallback m_frameCallback;
        bool m_isRunning;
        bool m_shouldStop;
        int m_screenIndex;
        int m_frameWidth;
        int m_frameHeight;
            
        // Platform-specific handle for capture thread
        void* m_captureThread;
    };
}