#pragma once

#include <cstdint>
#include <vector>

namespace core
{
    enum class VideoPixelFormat : std::uint8_t
    {
        Rgb24 = 0,
        Nv12 = 1,
    };

    /**
     * Decoded or captured frame for display.
     * Rgb24: tightly packed width*height*3, strideY = width*3, strideUV/uvOffset unused.
     * Nv12: Y plane then interleaved UV; strideY/strideUV are row bytes (FFmpeg-style).
     */
    struct VideoFrameBuffer
    {
        std::vector<std::uint8_t> data;
        int width = 0;
        int height = 0;
        VideoPixelFormat format = VideoPixelFormat::Rgb24;
        int strideY = 0;
        int strideUV = 0;
        /** If non-zero, UV plane starts at this byte offset; else strideY * height. */
        int uvOffset = 0;

        bool isEmpty() const
        {
            return data.empty() || width <= 0 || height <= 0;
        }

        std::size_t uvByteOffset() const
        {
            if (format != VideoPixelFormat::Nv12)
                return 0;
            if (uvOffset > 0)
                return static_cast<std::size_t>(uvOffset);
            return static_cast<std::size_t>(strideY) * static_cast<std::size_t>(height);
        }
    };
}
