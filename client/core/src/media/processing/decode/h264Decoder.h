#pragma once

#include <functional>
#include <memory>
#include <cstdint>
#include <vector>
#include "../../frame.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/hwcontext.h>
#include <libswscale/swscale.h>
}

namespace core::media
{
    enum class DecoderSurfaceFormat
    {
        Rgb24,
        Nv12,
    };

    class H264Decoder
    {
    public:
        using DecodedFrameCallback = std::function<void(const Frame& frame)>;

        H264Decoder();
        ~H264Decoder();

        bool initialize();
        void cleanup();
        bool decodePacket(const uint8_t* data, size_t size, int64_t pts);
        void setDecodedFrameCallback(DecodedFrameCallback callback);
        bool isInitialized() const;
        void setOutputFormat(int width, int height, int format = AV_PIX_FMT_RGB24);
        void setSurfaceFormat(DecoderSurfaceFormat format);

        AVPixelFormat hwDecoderPixelFormat() const { return m_hwPixFmt; }
        bool usesHardwareDecoder() const { return m_hwDecodeActive; }
        DecoderSurfaceFormat surfaceFormat() const { return m_surfaceFormat; }

    private:
        AVCodecContext* m_codecContext;
        AVFrame* m_frame;
        AVFrame* m_frameRGB;
        AVFrame* m_frameNv12;
        AVPacket* m_packet;
        SwsContext* m_swsContext;
        SwsContext* m_swsNv12;

        DecodedFrameCallback m_decodedCallback;
        bool m_initialized;
        int m_outputWidth;
        int m_outputHeight;
        int m_outputFormat;
        int m_lastSrcWidth;
        int m_lastSrcHeight;
        AVPixelFormat m_lastSrcPixFmt;
        DecoderSurfaceFormat m_surfaceFormat;

        std::vector<uint8_t> m_nv12PackBuffer;

        AVBufferRef* m_hwDeviceCtx;
        AVFrame* m_swFrame;
        AVPixelFormat m_hwPixFmt;
        bool m_hwDecodeActive;

        bool tryInitHwDecoder(const AVCodec* codec);
        bool convertFrameToRGB(AVFrame* srcFrame);
        bool convertFrameToNv12(AVFrame* srcFrame);

        int m_nv12LastSrcW;
        int m_nv12LastSrcH;
        AVPixelFormat m_nv12LastSrcFmt;
    };
}
