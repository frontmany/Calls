#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libavutil/frame.h>
}
#include <vector>
#include <memory>
#include <mutex>
#include <cstdint>

namespace core::media
{
    class OpusDecoder {
    public:
        struct Config {
            int sampleRate = 48000;
            int channels = 1;
        };

        OpusDecoder(const Config& config);
        OpusDecoder();
        ~OpusDecoder();
        bool isInitialized() const { return m_initialized; }
        int decode(const unsigned char* data, int dataLength, float* pcm, int frameSize, int decodeFec);

    private:
        bool initialize();

    private:
        Config m_config;
        AVCodecContext* m_codecContext = nullptr;
        const AVCodec* m_codec = nullptr;
        AVFrame* m_frame = nullptr;
        AVPacket* m_packet = nullptr;
        bool m_initialized = false;
        mutable std::mutex m_mutex;
    };
}