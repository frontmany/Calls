#include "opusEncoder.h"
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
    OpusEncoder::OpusEncoder(const Config& config)
        : m_config(config)
    {
        initialize();
    }

    OpusEncoder::OpusEncoder()
    {
        initialize();
    }

    OpusEncoder::~OpusEncoder()
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

    bool OpusEncoder::initialize() {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        // Find the Opus encoder
        m_codec = avcodec_find_encoder(AV_CODEC_ID_OPUS);
        if (!m_codec) {
            std::cerr << "Failed to find Opus encoder" << std::endl;
            return false;
        }
        
        // Allocate codec context
        m_codecContext = avcodec_alloc_context3(m_codec);
        if (!m_codecContext) {
            std::cerr << "Failed to allocate codec context" << std::endl;
            return false;
        }
        
        // Set codec parameters
        m_codecContext->sample_fmt = AV_SAMPLE_FMT_FLT;
        m_codecContext->sample_rate = m_config.sampleRate;
        m_codecContext->ch_layout.nb_channels = m_config.channels;
        av_channel_layout_default(&m_codecContext->ch_layout, m_config.channels);
        m_codecContext->bit_rate = m_config.bitrate;
        m_codecContext->frame_size = m_config.frameSize;
        
        // Set application type
        av_opt_set_int(m_codecContext->priv_data, "application", static_cast<int64_t>(m_config.application), 0);
        
        // Set complexity
        av_opt_set_int(m_codecContext->priv_data, "complexity", static_cast<int64_t>(m_config.complexity), 0);
        
        // Open codec
        int ret = avcodec_open2(m_codecContext, m_codec, nullptr);
        if (ret < 0) {
            char errBuf[AV_ERROR_MAX_STRING_SIZE];
            std::cerr << "Failed to open codec: " << av_make_error_string(errBuf, AV_ERROR_MAX_STRING_SIZE, ret) << std::endl;
            return false;
        }
        
        // Allocate frame and packet
        m_frame = av_frame_alloc();
        m_packet = av_packet_alloc();
        
        if (!m_frame || !m_packet) {
            std::cerr << "Failed to allocate frame or packet" << std::endl;
            return false;
        }
        
        // Configure frame
        m_frame->sample_rate = m_config.sampleRate;
        m_frame->nb_samples = m_config.frameSize;
        m_frame->format = AV_SAMPLE_FMT_FLT;
        av_channel_layout_default(&m_frame->ch_layout, m_config.channels);
        m_frame->ch_layout.nb_channels = m_config.channels;
        
        // Allocate frame buffer
        ret = av_frame_get_buffer(m_frame, 0);
        if (ret < 0) {
            char errBuf[AV_ERROR_MAX_STRING_SIZE];
            std::cerr << "Failed to allocate frame buffer: " << av_make_error_string(errBuf, AV_ERROR_MAX_STRING_SIZE, ret) << std::endl;
            return false;
        }
        
        m_initialized = true;
        return true;
    }

    int OpusEncoder::encode(const float* pcm, unsigned char* data, int maxDataBytes) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_initialized || !m_codecContext || !m_frame || !m_packet) return -1;
        
        // Make sure frame is writable
        int ret = av_frame_make_writable(m_frame);
        if (ret < 0) {
            char errBuf[AV_ERROR_MAX_STRING_SIZE];
            std::cerr << "Failed to make frame writable: " << av_make_error_string(errBuf, AV_ERROR_MAX_STRING_SIZE, ret) << std::endl;
            return -1;
        }
        
        // Copy PCM data to frame
        float* frameData = reinterpret_cast<float*>(m_frame->data[0]);
        memcpy(frameData, pcm, static_cast<size_t>(m_config.frameSize) * static_cast<size_t>(m_config.channels) * sizeof(float));
        
        // Send frame to encoder
        ret = avcodec_send_frame(m_codecContext, m_frame);
        if (ret < 0) {
            char errBuf[AV_ERROR_MAX_STRING_SIZE];
            std::cerr << "Failed to send frame to encoder: " << av_make_error_string(errBuf, AV_ERROR_MAX_STRING_SIZE, ret) << std::endl;
            return -1;
        }
        
        // Receive packet from encoder
        ret = avcodec_receive_packet(m_codecContext, m_packet);
        if (ret < 0) {
            char errBuf[AV_ERROR_MAX_STRING_SIZE];
            std::cerr << "Failed to receive packet from encoder: " << av_make_error_string(errBuf, AV_ERROR_MAX_STRING_SIZE, ret) << std::endl;
            return -1;
        }
        
        // Copy encoded data to output buffer
        int bytesToCopy = (m_packet->size < maxDataBytes) ? m_packet->size : maxDataBytes;
        memcpy(data, m_packet->data, static_cast<size_t>(bytesToCopy));
        
        // Unref packet for next use
        av_packet_unref(m_packet);
        
        return bytesToCopy;
    }
}