#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libavutil/frame.h>
}
#include <vector>
#include <mutex>
#include <cstdint>

namespace core::media
{
    class OpusEncoder {
    public:
        enum class EncoderMode {
            VOIP = 2048,
            AUDIO = 2049,
            LOW_DELAY = 2051
        };

        struct Config {
            int sampleRate = 48000;
            int channels = 1;
            EncoderMode application = EncoderMode::AUDIO;
            int bitrate = 64000;
            int complexity = 5;
            int frameSize = 960;
        };

        OpusEncoder(const Config& config);
        OpusEncoder();
        ~OpusEncoder();
        bool isInitialized() const { return m_initialized; }
        int encode(const float* pcm, unsigned char* data, int maxDataBytes);

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