#include "opusDecoder.h"
#include <iostream>
#include <cstring>

namespace core
{
    namespace media 
    {
    OpusDecoder::OpusDecoder(const Config& config)
        : m_config(config)
    {
        initialize();
    }

    OpusDecoder::OpusDecoder()
    {
        initialize();
    }

    OpusDecoder::~OpusDecoder()
    {
        if (m_initialized) {
            opus_decoder_destroy(m_decoder);
        }
    }

    bool OpusDecoder::initialize() {
        std::lock_guard<std::mutex> lock(m_mutex);

        int error;
        m_decoder = opus_decoder_create(m_config.sampleRate, m_config.channels, &error);

        if (error != OPUS_OK || !m_decoder) {
            std::cerr << "Failed to create Opus decoder: " << opus_strerror(error) << std::endl;
            return false;
        }

        m_initialized = true;
        return true;
    }

    int OpusDecoder::decode(const unsigned char* data, int dataLength, float* pcm, int frameSize, int decodeFec) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_initialized || !m_decoder) return OPUS_INVALID_STATE;

        return opus_decode_float(m_decoder, data, dataLength, pcm, frameSize, decodeFec);
    }
    }
}