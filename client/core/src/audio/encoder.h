#pragma once

#include <opus.h>
#include <vector>
#include <mutex>
#include <cstdint>

namespace audio 
{
    class Encoder {
    public:
        enum class EncoderMode {
            VOIP = OPUS_APPLICATION_VOIP,
            AUDIO = OPUS_APPLICATION_AUDIO,
            LOW_DELAY = OPUS_APPLICATION_RESTRICTED_LOWDELAY
        };

        struct Config {
            int sampleRate = 48000;
            int channels = 1;
            EncoderMode application = EncoderMode::AUDIO;
            int bitrate = 64000;
            int complexity = 5;
            int frameSize = 960;
        };

        Encoder(const Config& config);
        Encoder();
        ~Encoder();
        bool isInitialized() const { return m_initialized; }
        int encode(const float* pcm, unsigned char* data, int maxDataBytes);

    private:
        bool initialize();

    private:
        Config m_config;
        OpusEncoder* m_encoder;
        bool m_initialized = false;
        mutable std::mutex m_mutex;
    };
}