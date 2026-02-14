#include "h264Encoder.h"
#include <iostream>

namespace core::media
{
    H264Encoder::H264Encoder()
        : m_codecContext(nullptr)
        , m_frame(nullptr)
        , m_packet(nullptr)
        , m_swsContext(nullptr)
        , m_initialized(false)
        , m_width(0)
        , m_height(0)
        , m_fps(30)
        , m_bitrate(2000000)
    {
    }

    H264Encoder::~H264Encoder()
    {
        cleanup();
    }

    bool H264Encoder::initialize(int width, int height, int fps, int bitrate)
    {
        if (m_initialized) {
            cleanup();
        }

        m_width = width;
        m_height = height;
        m_fps = fps;
        m_bitrate = bitrate;

        // Find H.264 encoder
        const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
        if (!codec) {
            std::cerr << "H.264 codec not found" << std::endl;
            return false;
        }

        // Allocate codec context
        m_codecContext = avcodec_alloc_context3(codec);
        if (!m_codecContext) {
            std::cerr << "Failed to allocate codec context" << std::endl;
            return false;
        }

        // Set codec parameters
        m_codecContext->codec_id = AV_CODEC_ID_H264;
        m_codecContext->width = width;
        m_codecContext->height = height;
        m_codecContext->time_base.num = 1;
        m_codecContext->time_base.den = fps;
        m_codecContext->framerate.num = fps;
        m_codecContext->framerate.den = 1;
        m_codecContext->bit_rate = bitrate;
        m_codecContext->gop_size = 10;
        m_codecContext->max_b_frames = 0;
        m_codecContext->pix_fmt = AV_PIX_FMT_YUV420P;

        // H.264 specific options
        if (codec->id == AV_CODEC_ID_H264) {
            av_opt_set(m_codecContext->priv_data, "preset", "fast", 0);
            av_opt_set(m_codecContext->priv_data, "tune", "zerolatency", 0);
        }

        // Open codec
        int ret = avcodec_open2(m_codecContext, codec, nullptr);
        if (ret < 0) {
            std::cerr << "Failed to open codec: error code " << ret << std::endl;
            cleanup();
            return false;
        }

        // Allocate frame
        m_frame = av_frame_alloc();
        if (!m_frame) {
            std::cerr << "Failed to allocate frame" << std::endl;
            cleanup();
            return false;
        }

        m_frame->format = m_codecContext->pix_fmt;
        m_frame->width = width;
        m_frame->height = height;

        ret = av_frame_get_buffer(m_frame, 0);
        if (ret < 0) {
            std::cerr << "Failed to allocate frame buffer: error code " << ret << std::endl;
            cleanup();
            return false;
        }

        // Allocate packet
        m_packet = av_packet_alloc();
        if (!m_packet) {
            std::cerr << "Failed to allocate packet" << std::endl;
            cleanup();
            return false;
        }

        m_initialized = true;
        return true;
    }

    void H264Encoder::cleanup()
    {
        if (m_frame) {
            av_frame_free(&m_frame);
        }

        if (m_packet) {
            av_packet_free(&m_packet);
        }

        if (m_codecContext) {
            avcodec_free_context(&m_codecContext);
        }

        if (m_swsContext) {
            sws_freeContext(m_swsContext);
            m_swsContext = nullptr;
        }

        m_initialized = false;
    }

    bool H264Encoder::encodeFrame(const Frame& frame)
    {
        if (!m_initialized) {
            std::cerr << "Encoder not initialized" << std::endl;
            return false;
        }

        // Reinitialize encoder if input resolution changed
        if (frame.width != m_width || frame.height != m_height) {
            std::cout << "Encoder resolution changed from " << m_width << "x" << m_height
                      << " to " << frame.width << "x" << frame.height << ", reinitializing" << std::endl;
            auto callback = m_encodedCallback;
            if (!initialize(frame.width, frame.height, m_fps, m_bitrate)) {
                std::cerr << "Failed to reinitialize encoder for new resolution" << std::endl;
                return false;
            }
            m_encodedCallback = callback;
        }

        // Convert input frame to YUV420P if needed
        if (!convertFrame(frame)) {
            return false;
        }

        // Set frame timestamp
        m_frame->pts = frame.pts;

        // Send frame to encoder
        int ret = avcodec_send_frame(m_codecContext, m_frame);
        if (ret < 0) {
            std::cerr << "Error sending frame to encoder: error code " << ret << std::endl;
            return false;
        }

        // Receive packets from encoder
        while (ret >= 0) {
            ret = avcodec_receive_packet(m_codecContext, m_packet);
            if (ret == static_cast<int>(AVERROR(EAGAIN)) || ret == AVERROR_EOF) {
                break;
            }
            else if (ret < 0) {
                std::cerr << "Error receiving packet from encoder: error code " << ret << std::endl;
                return false;
            }

            // Call callback with encoded data
            if (m_encodedCallback) {
                m_encodedCallback(m_packet->data, m_packet->size, m_packet->pts);
            }

            av_packet_unref(m_packet);
        }

        return true;
    }

    bool H264Encoder::convertFrame(const Frame& inputFrame)
    {
        // Validate input - avoid "bad src image pointers"
        if (!inputFrame.data || inputFrame.width <= 0 || inputFrame.height <= 0 ||
            inputFrame.format == AV_PIX_FMT_NONE || inputFrame.format < 0) {
            return false;
        }

        // Use RGB24 if format not explicitly set
        AVPixelFormat srcFormat = (inputFrame.format != 0) ?
            static_cast<AVPixelFormat>(inputFrame.format) : AV_PIX_FMT_RGB24;

        // Create sws context for pixel format conversion (1:1, no scaling)
        if (!m_swsContext) {
            m_swsContext = sws_getContext(
                m_width, m_height, srcFormat,
                m_width, m_height, AV_PIX_FMT_YUV420P,
                SWS_BILINEAR, nullptr, nullptr, nullptr
            );

            if (!m_swsContext) {
                return false;
            }
        }

        int ret = av_frame_make_writable(m_frame);
        if (ret < 0) {
            return false;
        }

        const uint8_t* srcData[4] = { inputFrame.data, nullptr, nullptr, nullptr };
        int srcLinesize[4] = { inputFrame.width * 3, 0, 0, 0 };

        ret = sws_scale(
            m_swsContext,
            srcData, srcLinesize,
            0, m_height,
            m_frame->data, m_frame->linesize
        );

        if (ret <= 0 || ret != m_height) {
            return false;
        }

        return true;
    }

    void H264Encoder::setEncodedDataCallback(EncodedDataCallback callback)
    {
        m_encodedCallback = callback;
    }

    bool H264Encoder::isInitialized() const
    {
        return m_initialized;
    }
}