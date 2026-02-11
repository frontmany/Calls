#include "h264Decoder.h"
#include <iostream>


namespace core::media
{
    H264Decoder::H264Decoder()
        : m_codecContext(nullptr)
        , m_frame(nullptr)
        , m_frameRGB(nullptr)
        , m_packet(nullptr)
        , m_swsContext(nullptr)
        , m_initialized(false)
        , m_outputWidth(0)
        , m_outputHeight(0)
        , m_outputFormat(AV_PIX_FMT_RGB24)
    {
    }

    H264Decoder::~H264Decoder()
    {
        cleanup();
    }

    bool H264Decoder::initialize()
    {
        if (m_initialized) {
            cleanup();
        }

        // Find H.264 decoder
        const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
        if (!codec) {
            std::cerr << "H.264 decoder not found" << std::endl;
            return false;
        }

        // Allocate codec context
        m_codecContext = avcodec_alloc_context3(codec);
        if (!m_codecContext) {
            std::cerr << "Failed to allocate decoder context" << std::endl;
            return false;
        }

        // Open codec
        int ret = avcodec_open2(m_codecContext, codec, nullptr);
        if (ret < 0) {
            std::cerr << "Failed to open decoder: error code " << ret << std::endl;
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

        // Allocate RGB frame
        m_frameRGB = av_frame_alloc();
        if (!m_frameRGB) {
            std::cerr << "Failed to allocate RGB frame" << std::endl;
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

    void H264Decoder::cleanup()
    {
        if (m_frame) {
            av_frame_free(&m_frame);
        }

        if (m_frameRGB) {
            av_frame_free(&m_frameRGB);
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

    bool H264Decoder::decodePacket(const uint8_t* data, size_t size, int64_t pts)
    {
        if (!m_initialized) {
            std::cerr << "Decoder not initialized" << std::endl;
            return false;
        }

        // Set packet data
        m_packet->data = const_cast<uint8_t*>(data);
        m_packet->size = size;
        m_packet->pts = pts;

        // Send packet to decoder
        int ret = avcodec_send_packet(m_codecContext, m_packet);
        if (ret < 0) {
            std::cerr << "Error sending packet to decoder: error code " << ret << std::endl;
            return false;
        }

        // Receive frames from decoder
        while (ret >= 0) {
            ret = avcodec_receive_frame(m_codecContext, m_frame);
            if (ret == static_cast<int>(AVERROR(EAGAIN)) || ret == AVERROR_EOF) {
                break;
            }
            else if (ret < 0) {
                std::cerr << "Error receiving frame from decoder: error code " << ret << std::endl;
                return false;
            }

            // Update output dimensions if not set
            if (m_outputWidth == 0 || m_outputHeight == 0) {
                m_outputWidth = m_frame->width;
                m_outputHeight = m_frame->height;
            }

            // Convert frame to RGB and call callback
            if (convertFrameToRGB() && m_decodedCallback) {
                Frame outputFrame;
                outputFrame.data = m_frameRGB->data[0];
                outputFrame.width = m_outputWidth;
                outputFrame.height = m_outputHeight;
                outputFrame.format = m_outputFormat;
                outputFrame.pts = m_frame->pts;
                outputFrame.size = static_cast<int>(av_image_get_buffer_size((AVPixelFormat)m_outputFormat,
                    m_outputWidth, m_outputHeight, 1));

                m_decodedCallback(outputFrame);
            }
        }

        return true;
    }

    bool H264Decoder::convertFrameToRGB()
    {
        // Check if we need to create or update sws context
        if (!m_swsContext ||
            m_frameRGB->width != m_outputWidth ||
            m_frameRGB->height != m_outputHeight) {

            if (m_swsContext) {
                sws_freeContext(m_swsContext);
            }

            // Setup RGB frame
            m_frameRGB->format = m_outputFormat;
            m_frameRGB->width = m_outputWidth;
            m_frameRGB->height = m_outputHeight;

            // Allocate buffer for RGB frame
            int ret = av_frame_get_buffer(m_frameRGB, 0);
            if (ret < 0) {
                std::cerr << "Failed to allocate RGB frame buffer: error code " << ret << std::endl;
                return false;
            }

            // Create sws context
            m_swsContext = sws_getContext(
                m_frame->width, m_frame->height, (AVPixelFormat)m_frame->format,
                m_outputWidth, m_outputHeight, (AVPixelFormat)m_outputFormat,
                SWS_BILINEAR, nullptr, nullptr, nullptr
            );

            if (!m_swsContext) {
                std::cerr << "Failed to create sws context" << std::endl;
                return false;
            }
        }

        // Make sure RGB frame is writable
        int ret = av_frame_make_writable(m_frameRGB);
        if (ret < 0) {
            std::cerr << "Failed to make RGB frame writable: error code " << ret << std::endl;
            return false;
        }

        // Convert frame
        ret = sws_scale(
            m_swsContext,
            m_frame->data, m_frame->linesize,
            0, m_frame->height,
            m_frameRGB->data, m_frameRGB->linesize
        );

        if (ret != m_frame->height) {
            std::cerr << "Failed to convert frame to RGB: expected height " << m_frame->height
                << ", got " << ret << std::endl;
            return false;
        }

        return true;
    }

    void H264Decoder::setDecodedFrameCallback(DecodedFrameCallback callback)
    {
        m_decodedCallback = callback;
    }

    bool H264Decoder::isInitialized() const
    {
        return m_initialized;
    }

    void H264Decoder::setOutputFormat(int width, int height, int format)
    {
        m_outputWidth = width;
        m_outputHeight = height;
        m_outputFormat = format;

        // Force recreation of sws context on next decode
        if (m_swsContext) {
            sws_freeContext(m_swsContext);
            m_swsContext = nullptr;
        }
    }
}