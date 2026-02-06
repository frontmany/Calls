#pragma once

#include <functional>
#include <memory>
#include <cstdint>
#include "../frame.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
}

namespace core
{
    namespace media
    {
        class CameraCaptureService
        {
        public:
            using FrameCallback = std::function<void(const Frame&)>;

            CameraCaptureService();
            ~CameraCaptureService();

            bool start(const char* deviceName = nullptr);
            void stop();
            bool isRunning() const;
            void setFrameCallback(FrameCallback callback);
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
            bool m_isRunning;
            bool m_shouldStop;
            int m_frameWidth;
            int m_frameHeight;
            
            // Platform-specific handle for capture thread
            void* m_captureThread;
        };
    }
}