#include "opusDecoder.h"
#include <iostream>
#include <cstring>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libavutil/frame.h>
}

namespace core::media
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
            if (m_packet) {
                av_packet_free(&m_packet);
            }
            if (m_frame) {
                av_frame_free(&m_frame);
            }
            if (m_codecContext) {
                avcodec_free_context(&m_codecContext);
            }
        }
    }

    bool OpusDecoder::initialize() {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        m_codec = avcodec_find_decoder(AV_CODEC_ID_OPUS);
        if (!m_codec) {
            std::cerr << "Failed to find Opus decoder" << std::endl;
            return false;
        }
        
        m_codecContext = avcodec_alloc_context3(m_codec);
        if (!m_codecContext) {
            std::cerr << "Failed to allocate codec context" << std::endl;
            return false;
        }

        m_codecContext->sample_fmt = AV_SAMPLE_FMT_FLT;
        m_codecContext->sample_rate = m_config.sampleRate;
        m_codecContext->ch_layout.nb_channels = m_config.channels;
        av_channel_layout_default(&m_codecContext->ch_layout, m_config.channels);

        int ret = avcodec_open2(m_codecContext, m_codec, nullptr);
        if (ret < 0) {
            char errBuf[AV_ERROR_MAX_STRING_SIZE];
            std::cerr << "Failed to open codec: " << av_make_error_string(errBuf, AV_ERROR_MAX_STRING_SIZE, ret) << std::endl;
            return false;
        }

        m_frame = av_frame_alloc();
        m_packet = av_packet_alloc();
        
        if (!m_frame || !m_packet) {
            std::cerr << "Failed to allocate frame or packet" << std::endl;
            return false;
        }
        
        m_initialized = true;
        return true;
    }

    int OpusDecoder::decode(const unsigned char* data, int dataLength, float* pcm, int frameSize, int decodeFec) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_initialized || !m_codecContext || !m_frame || !m_packet) return -1;

        m_packet->data = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(data));
        m_packet->size = dataLength;

        int ret = avcodec_send_packet(m_codecContext, m_packet);
        if (ret < 0) {
            char errBuf[AV_ERROR_MAX_STRING_SIZE];
            std::cerr << "Failed to send packet to decoder: " << av_make_error_string(errBuf, AV_ERROR_MAX_STRING_SIZE, ret) << std::endl;
            return -1;
        }

        ret = avcodec_receive_frame(m_codecContext, m_frame);
        if (ret < 0) {
            char errBuf[AV_ERROR_MAX_STRING_SIZE];
            std::cerr << "Failed to receive frame from decoder: " << av_make_error_string(errBuf, AV_ERROR_MAX_STRING_SIZE, ret) << std::endl;
            return -1;
        }

        int samplesToCopy = (m_frame->nb_samples < frameSize) ? m_frame->nb_samples : frameSize;
        int channelsToCopy = (m_frame->ch_layout.nb_channels < m_config.channels) ? m_frame->ch_layout.nb_channels : m_config.channels;

        float* frameData = reinterpret_cast<float*>(m_frame->data[0]);
        for (int sample = 0; sample < samplesToCopy; ++sample) {
            for (int channel = 0; channel < channelsToCopy; ++channel) {
                pcm[sample * m_config.channels + channel] = frameData[sample * m_frame->ch_layout.nb_channels + channel];
            }
        }

        av_frame_unref(m_frame);
        
        return samplesToCopy;
    }
}