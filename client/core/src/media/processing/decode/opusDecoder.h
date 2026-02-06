#pragma once

#include <opus.h>
#include <vector>
#include <memory>
#include <mutex>
#include <cstdint>

namespace core
{
    namespace media
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
        OpusDecoder* m_decoder = nullptr;
        bool m_initialized = false;
        mutable std::mutex m_mutex;
    };
    }
}