#pragma once

#include <functional>
#include <memory>
#include <cstdint>
#include <thread>
#include <string>
#include "media/frame.h"

extern "C" {
struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct SwsContext;
}

namespace core::media
{
    struct ScreenCaptureTarget
    {
        int index = -1;
        std::string osId;
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        double dpr = 1.0;
    };

    class ScreenCaptureService
    {
    public:
        using FrameCallback = std::function<void(const Frame&)>;

        ScreenCaptureService();
        ~ScreenCaptureService();

        bool start(const ScreenCaptureTarget& target);
        void stop();
        bool isRunning() const;
        void setFrameCallback(FrameCallback callback);

    private:
        void cleanup();
        bool initializeScreenCapture(const ScreenCaptureTarget& target);
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
        ScreenCaptureTarget m_target;
        int m_frameWidth;
        int m_frameHeight;
            
        std::thread* m_captureThread;
    };
}