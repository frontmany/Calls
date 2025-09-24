#include "encoder.h"
#include <iostream>
#include <cstring>

using namespace calls;

Encoder::Encoder(const Config& config) 
    : m_config(config)
{
    initialize();
}

Encoder::Encoder()
{
    initialize();
}

Encoder::~Encoder()
{
    if (m_initialized) {
        opus_encoder_destroy(m_encoder);
    }
}

bool Encoder::initialize() {
    std::lock_guard<std::mutex> lock(m_mutex);
    int error;
    m_encoder = opus_encoder_create(m_config.sampleRate, m_config.channels,
        static_cast<int>(m_config.application), &error);

    if (error != OPUS_OK || !m_encoder) {
        std::cerr << "Failed to create Opus encoder: " << opus_strerror(error) << std::endl;
        return false;
    }

    if (opus_encoder_ctl(m_encoder, OPUS_SET_BITRATE(m_config.bitrate)) != OPUS_OK) {
        std::cerr << "Failed to set encoder bitrate" << std::endl;
        return false;
    }

    if (opus_encoder_ctl(m_encoder, OPUS_SET_COMPLEXITY(m_config.complexity)) != OPUS_OK) {
        std::cerr << "Failed to set encoder complexity" << std::endl;
        return false;
    }

    m_initialized = true;
    return true;
}

int Encoder::encode(const float* pcm, unsigned char* data, int maxDataBytes) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized || !m_encoder) return OPUS_INVALID_STATE;

    return opus_encode_float(m_encoder, pcm, m_config.frameSize, data, maxDataBytes);
}