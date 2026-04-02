#include "h264Decoder.h"
#include "utilities/logger.h"

#include <iostream>

namespace core::media
{
    namespace
    {
#if defined(_WIN32)
        constexpr AVHWDeviceType kPreferredHwDevice = AV_HWDEVICE_TYPE_D3D11VA;
#elif defined(__APPLE__)
        constexpr AVHWDeviceType kPreferredHwDevice = AV_HWDEVICE_TYPE_VIDEOTOOLBOX;
#elif defined(__linux__)
        constexpr AVHWDeviceType kPreferredHwDevice = AV_HWDEVICE_TYPE_VAAPI;
#else
        constexpr AVHWDeviceType kPreferredHwDevice = AV_HWDEVICE_TYPE_NONE;
#endif

        extern "C" AVPixelFormat h264_get_hw_format(AVCodecContext* ctx, const AVPixelFormat* pix_fmts)
        {
            auto* self = static_cast<H264Decoder*>(ctx->opaque);
            if (!self)
                return AV_PIX_FMT_NONE;
            const AVPixelFormat wanted = self->hwDecoderPixelFormat();
            for (const AVPixelFormat* p = pix_fmts; *p != AV_PIX_FMT_NONE; p++)
            {
                if (*p == wanted)
                    return *p;
            }
            return AV_PIX_FMT_NONE;
        }

        const char* hwDeviceTypeName(AVHWDeviceType t)
        {
            const char* n = av_hwdevice_get_type_name(t);
            return n ? n : "unknown";
        }
    }

    H264Decoder::H264Decoder()
        : m_codecContext(nullptr)
        , m_frame(nullptr)
        , m_frameRGB(nullptr)
        , m_frameNv12(nullptr)
        , m_packet(nullptr)
        , m_swsContext(nullptr)
        , m_swsNv12(nullptr)
        , m_initialized(false)
        , m_outputWidth(0)
        , m_outputHeight(0)
        , m_outputFormat(AV_PIX_FMT_RGB24)
        , m_lastSrcWidth(0)
        , m_lastSrcHeight(0)
        , m_lastSrcPixFmt(AV_PIX_FMT_NONE)
        , m_surfaceFormat(DecoderSurfaceFormat::Nv12)
        , m_hwDeviceCtx(nullptr)
        , m_swFrame(nullptr)
        , m_hwPixFmt(AV_PIX_FMT_NONE)
        , m_hwDecodeActive(false)
        , m_nv12LastSrcW(0)
        , m_nv12LastSrcH(0)
        , m_nv12LastSrcFmt(AV_PIX_FMT_NONE)
    {
    }

    H264Decoder::~H264Decoder()
    {
        cleanup();
    }

    bool H264Decoder::tryInitHwDecoder(const AVCodec* codec)
    {
        m_hwPixFmt = AV_PIX_FMT_NONE;
        if (kPreferredHwDevice == AV_HWDEVICE_TYPE_NONE)
            return false;

        for (int i = 0;; i++)
        {
            const AVCodecHWConfig* config = avcodec_get_hw_config(codec, i);
            if (!config)
                break;
            if (config->device_type == kPreferredHwDevice
                && (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX))
            {
                m_hwPixFmt = config->pix_fmt;
                break;
            }
        }

        if (m_hwPixFmt == AV_PIX_FMT_NONE)
            return false;

        const int err = av_hwdevice_ctx_create(&m_hwDeviceCtx, kPreferredHwDevice, nullptr, nullptr, 0);
        if (err < 0)
        {
            m_hwPixFmt = AV_PIX_FMT_NONE;
            return false;
        }
        return true;
    }

    bool H264Decoder::initialize()
    {
        if (m_initialized)
            cleanup();

        const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
        if (!codec)
        {
            std::cerr << "H.264 decoder not found" << std::endl;
            return false;
        }

        m_codecContext = avcodec_alloc_context3(codec);
        if (!m_codecContext)
        {
            std::cerr << "Failed to allocate decoder context" << std::endl;
            return false;
        }

        m_codecContext->opaque = this;

        if (tryInitHwDecoder(codec))
        {
            m_codecContext->get_format = h264_get_hw_format;
            m_codecContext->hw_device_ctx = av_buffer_ref(m_hwDeviceCtx);
            m_hwDecodeActive = true;
        }

        int ret = avcodec_open2(m_codecContext, codec, nullptr);
        if (ret < 0 && m_hwDecodeActive)
        {
            avcodec_free_context(&m_codecContext);
            m_codecContext = nullptr;
            m_hwDecodeActive = false;
            m_hwPixFmt = AV_PIX_FMT_NONE;
            if (m_hwDeviceCtx)
            {
                av_buffer_unref(&m_hwDeviceCtx);
                m_hwDeviceCtx = nullptr;
            }

            m_codecContext = avcodec_alloc_context3(codec);
            if (!m_codecContext)
            {
                std::cerr << "Failed to allocate decoder context after HW fallback" << std::endl;
                return false;
            }
            m_codecContext->opaque = this;
            ret = avcodec_open2(m_codecContext, codec, nullptr);
        }

        if (ret < 0)
        {
            std::cerr << "Failed to open decoder: error code " << ret << std::endl;
            cleanup();
            return false;
        }

        m_frame = av_frame_alloc();
        if (!m_frame)
        {
            std::cerr << "Failed to allocate frame" << std::endl;
            cleanup();
            return false;
        }

        if (m_hwDecodeActive)
        {
            m_swFrame = av_frame_alloc();
            if (!m_swFrame)
            {
                std::cerr << "Failed to allocate SW frame for HW decode" << std::endl;
                cleanup();
                return false;
            }
            LOG_INFO("H264Decoder: hardware decode active ({})", hwDeviceTypeName(kPreferredHwDevice));
        }

        m_frameRGB = av_frame_alloc();
        if (!m_frameRGB)
        {
            std::cerr << "Failed to allocate RGB frame" << std::endl;
            cleanup();
            return false;
        }

        m_packet = av_packet_alloc();
        if (!m_packet)
        {
            std::cerr << "Failed to allocate packet" << std::endl;
            cleanup();
            return false;
        }

        m_initialized = true;
        return true;
    }

    void H264Decoder::cleanup()
    {
        m_nv12PackBuffer.clear();

        if (m_swFrame)
        {
            av_frame_free(&m_swFrame);
        }

        if (m_frame)
        {
            av_frame_free(&m_frame);
        }

        if (m_frameRGB)
        {
            av_frame_free(&m_frameRGB);
        }

        if (m_frameNv12)
        {
            av_frame_free(&m_frameNv12);
        }

        if (m_packet)
        {
            av_packet_free(&m_packet);
        }

        if (m_codecContext)
        {
            avcodec_free_context(&m_codecContext);
        }

        if (m_hwDeviceCtx)
        {
            av_buffer_unref(&m_hwDeviceCtx);
            m_hwDeviceCtx = nullptr;
        }

        if (m_swsContext)
        {
            sws_freeContext(m_swsContext);
            m_swsContext = nullptr;
        }

        if (m_swsNv12)
        {
            sws_freeContext(m_swsNv12);
            m_swsNv12 = nullptr;
        }

        m_hwDecodeActive = false;
        m_hwPixFmt = AV_PIX_FMT_NONE;
        m_lastSrcPixFmt = AV_PIX_FMT_NONE;
        m_nv12LastSrcW = 0;
        m_nv12LastSrcH = 0;
        m_nv12LastSrcFmt = AV_PIX_FMT_NONE;
        m_initialized = false;
    }

    bool H264Decoder::convertFrameToNv12(AVFrame* src)
    {
        const int w = src->width;
        const int h = src->height;
        if (!src->data[0] || w <= 0 || h <= 0 || src->format == AV_PIX_FMT_NONE)
            return false;

        const int bufSize = av_image_get_buffer_size(AV_PIX_FMT_NV12, w, h, 1);
        if (bufSize < 0)
            return false;
        m_nv12PackBuffer.resize(static_cast<size_t>(bufSize));

        if (static_cast<AVPixelFormat>(src->format) == AV_PIX_FMT_NV12)
        {
            if (av_image_copy_to_buffer(
                    m_nv12PackBuffer.data(),
                    bufSize,
                    src->data,
                    src->linesize,
                    AV_PIX_FMT_NV12,
                    w,
                    h,
                    1)
                < 0)
            {
                return false;
            }
        }
        else
        {
            if (!m_frameNv12)
            {
                m_frameNv12 = av_frame_alloc();
                if (!m_frameNv12)
                    return false;
            }

            const AVPixelFormat srcFmt = static_cast<AVPixelFormat>(src->format);
            const bool needSws = !m_swsNv12 || m_nv12LastSrcW != w || m_nv12LastSrcH != h || m_nv12LastSrcFmt != srcFmt;

            if (needSws)
            {
                if (m_swsNv12)
                {
                    sws_freeContext(m_swsNv12);
                    m_swsNv12 = nullptr;
                }
                av_frame_unref(m_frameNv12);
                m_frameNv12->format = AV_PIX_FMT_NV12;
                m_frameNv12->width = w;
                m_frameNv12->height = h;
                if (av_frame_get_buffer(m_frameNv12, 32) < 0)
                    return false;

                m_swsNv12 = sws_getContext(
                    w,
                    h,
                    srcFmt,
                    w,
                    h,
                    AV_PIX_FMT_NV12,
                    SWS_BILINEAR,
                    nullptr,
                    nullptr,
                    nullptr);
                if (!m_swsNv12)
                    return false;

                m_nv12LastSrcW = w;
                m_nv12LastSrcH = h;
                m_nv12LastSrcFmt = srcFmt;
            }

            if (av_frame_make_writable(m_frameNv12) < 0)
                return false;

            const int sc = sws_scale(
                m_swsNv12,
                src->data,
                src->linesize,
                0,
                h,
                m_frameNv12->data,
                m_frameNv12->linesize);
            if (sc < 0 || sc != h)
                return false;

            if (av_image_copy_to_buffer(
                    m_nv12PackBuffer.data(),
                    bufSize,
                    m_frameNv12->data,
                    m_frameNv12->linesize,
                    AV_PIX_FMT_NV12,
                    w,
                    h,
                    1)
                < 0)
            {
                return false;
            }
        }

        return true;
    }

    bool H264Decoder::decodePacket(const uint8_t* data, size_t size, int64_t pts)
    {
        if (!m_initialized)
        {
            std::cerr << "Decoder not initialized" << std::endl;
            return false;
        }

        m_packet->data = const_cast<uint8_t*>(data);
        m_packet->size = static_cast<int>(size);
        m_packet->pts = pts;

        int ret = avcodec_send_packet(m_codecContext, m_packet);
        if (ret < 0)
        {
            std::cerr << "Error sending packet to decoder: error code " << ret << std::endl;
            return false;
        }

        while (ret >= 0)
        {
            ret = avcodec_receive_frame(m_codecContext, m_frame);
            if (ret == static_cast<int>(AVERROR(EAGAIN)) || ret == AVERROR_EOF)
            {
                break;
            }
            if (ret < 0)
            {
                std::cerr << "Error receiving frame from decoder: error code " << ret << std::endl;
                return false;
            }

            AVFrame* src = m_frame;

            if (m_hwDecodeActive && m_hwPixFmt != AV_PIX_FMT_NONE && m_frame->format == m_hwPixFmt)
            {
                av_frame_unref(m_swFrame);
                const int tr = av_hwframe_transfer_data(m_swFrame, m_frame, 0);
                if (tr < 0)
                {
                    std::cerr << "Error transferring frame from HW: " << tr << std::endl;
                    continue;
                }
                av_frame_copy_props(m_swFrame, m_frame);
                src = m_swFrame;
            }

            if (src->width > 0 && src->height > 0
                && (m_outputWidth != src->width || m_outputHeight != src->height))
            {
                m_outputWidth = src->width;
                m_outputHeight = src->height;
            }
            if (m_outputWidth == 0 || m_outputHeight == 0)
            {
                m_outputWidth = src->width;
                m_outputHeight = src->height;
            }

            if (m_surfaceFormat == DecoderSurfaceFormat::Nv12)
            {
                if (convertFrameToNv12(src) && m_decodedCallback)
                {
                    Frame outputFrame;
                    outputFrame.data = m_nv12PackBuffer.data();
                    outputFrame.size = static_cast<int>(m_nv12PackBuffer.size());
                    outputFrame.width = m_outputWidth;
                    outputFrame.height = m_outputHeight;
                    outputFrame.linesize = m_outputWidth;
                    outputFrame.linesizeUV = m_outputWidth;
                    outputFrame.format = AV_PIX_FMT_NV12;
                    outputFrame.pts = src->pts;
                    m_decodedCallback(outputFrame);
                }
            }
            else
            {
                if (convertFrameToRGB(src) && m_decodedCallback)
                {
                    Frame outputFrame;
                    outputFrame.data = m_frameRGB->data[0];
                    outputFrame.width = m_outputWidth;
                    outputFrame.height = m_outputHeight;
                    outputFrame.linesize = m_frameRGB->linesize[0];
                    outputFrame.linesizeUV = 0;
                    outputFrame.format = m_outputFormat;
                    outputFrame.pts = src->pts;
                    outputFrame.size = static_cast<int>(av_image_get_buffer_size(
                        static_cast<AVPixelFormat>(m_outputFormat), m_outputWidth, m_outputHeight, 1));

                    m_decodedCallback(outputFrame);
                }
            }
        }

        return true;
    }

    bool H264Decoder::convertFrameToRGB(AVFrame* srcFrame)
    {
        if (!srcFrame->data[0] || srcFrame->width <= 0 || srcFrame->height <= 0
            || srcFrame->format == AV_PIX_FMT_NONE || srcFrame->format < 0)
        {
            return false;
        }

        const int dstWidth = (m_outputWidth > 0) ? m_outputWidth : srcFrame->width;
        const int dstHeight = (m_outputHeight > 0) ? m_outputHeight : srcFrame->height;

        const bool needNewSws = !m_swsContext || m_lastSrcWidth != srcFrame->width
            || m_lastSrcHeight != srcFrame->height || m_frameRGB->width != dstWidth || m_frameRGB->height != dstHeight
            || m_lastSrcPixFmt != static_cast<AVPixelFormat>(srcFrame->format);

        if (needNewSws)
        {
            if (m_swsContext)
            {
                sws_freeContext(m_swsContext);
                m_swsContext = nullptr;
            }

            m_lastSrcWidth = srcFrame->width;
            m_lastSrcHeight = srcFrame->height;
            m_lastSrcPixFmt = static_cast<AVPixelFormat>(srcFrame->format);

            m_frameRGB->format = m_outputFormat;
            m_frameRGB->width = dstWidth;
            m_frameRGB->height = dstHeight;

            const int bufRet = av_frame_get_buffer(m_frameRGB, 0);
            if (bufRet < 0)
            {
                return false;
            }

            m_swsContext = sws_getContext(
                srcFrame->width,
                srcFrame->height,
                static_cast<AVPixelFormat>(srcFrame->format),
                dstWidth,
                dstHeight,
                static_cast<AVPixelFormat>(m_outputFormat),
                SWS_BILINEAR,
                nullptr,
                nullptr,
                nullptr);

            if (!m_swsContext)
            {
                return false;
            }
        }

        int mk = av_frame_make_writable(m_frameRGB);
        if (mk < 0)
        {
            return false;
        }

        const int sc = sws_scale(
            m_swsContext,
            srcFrame->data,
            srcFrame->linesize,
            0,
            srcFrame->height,
            m_frameRGB->data,
            m_frameRGB->linesize);

        if (sc < 0 || sc != dstHeight)
        {
            return false;
        }

        return true;
    }

    void H264Decoder::setDecodedFrameCallback(DecodedFrameCallback callback)
    {
        m_decodedCallback = std::move(callback);
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
        m_lastSrcWidth = 0;
        m_lastSrcHeight = 0;
        m_lastSrcPixFmt = AV_PIX_FMT_NONE;

        if (m_swsContext)
        {
            sws_freeContext(m_swsContext);
            m_swsContext = nullptr;
        }
    }

    void H264Decoder::setSurfaceFormat(DecoderSurfaceFormat format)
    {
        m_surfaceFormat = format;
        m_lastSrcWidth = 0;
        m_lastSrcHeight = 0;
        m_lastSrcPixFmt = AV_PIX_FMT_NONE;
        m_nv12LastSrcW = 0;
        m_nv12LastSrcH = 0;
        m_nv12LastSrcFmt = AV_PIX_FMT_NONE;
        if (m_swsContext)
        {
            sws_freeContext(m_swsContext);
            m_swsContext = nullptr;
        }
        if (m_swsNv12)
        {
            sws_freeContext(m_swsNv12);
            m_swsNv12 = nullptr;
        }
    }

} // namespace core::media
