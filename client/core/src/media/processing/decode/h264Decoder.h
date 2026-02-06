#pragma once

#include <functional>
#include <memory>
#include <cstdint>
#include "../../frame.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace core
{
    namespace media
    {
        class H264Decoder
        {
        public:
            using DecodedFrameCallback = std::function<void(const Frame& frame)>;

            H264Decoder();
            ~H264Decoder();

            bool initialize();
            void cleanup();
            bool decodePacket(const uint8_t* data, size_t size, int64_t pts);
            void setDecodedFrameCallback(DecodedFrameCallback callback);
            bool isInitialized() const;
            void setOutputFormat(int width, int height, int format = AV_PIX_FMT_RGB24);

        private:
            AVCodecContext* m_codecContext;
            AVFrame* m_frame;
            AVFrame* m_frameRGB;
            AVPacket* m_packet;
            SwsContext* m_swsContext;
            
            DecodedFrameCallback m_decodedCallback;
            bool m_initialized;
            int m_outputWidth;
            int m_outputHeight;
            int m_outputFormat;
            
            bool convertFrameToRGB();
        };
    }
}