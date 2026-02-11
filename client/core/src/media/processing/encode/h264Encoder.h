#pragma once

#include <functional>
#include <memory>
#include <cstdint>

#include "media/frame.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

namespace core::media
{
    class H264Encoder
    {
    public:
        using EncodedDataCallback = std::function<void(const uint8_t* data, size_t size, int64_t pts)>;

        H264Encoder();
        ~H264Encoder();

        bool initialize(int width, int height, int fps = 30, int bitrate = 2000000);
        void cleanup();
        bool encodeFrame(const Frame& frame);
        void setEncodedDataCallback(EncodedDataCallback callback);
        bool isInitialized() const;

    private:
        AVCodecContext* m_codecContext;
        AVFrame* m_frame;
        AVPacket* m_packet;
        SwsContext* m_swsContext;
            
        EncodedDataCallback m_encodedCallback;
        bool m_initialized;
        int m_width;
        int m_height;
        int m_fps;
            
        bool convertFrame(const Frame& inputFrame);
    };
}