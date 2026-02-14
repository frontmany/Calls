#pragma once

#include <cstdint>
#include <vector>

namespace core::media
{
    struct Frame {
        const uint8_t* data = nullptr;
        size_t size = 0;
        int width = 0;
        int height = 0;
        int linesize = 0;  // row stride in bytes (0 = packed, assume width * bytesPerPixel)
        int format = 0;    // AVPixelFormat
        int64_t pts = 0;   // Presentation timestamp

        bool isValid() const {
            return data != nullptr && size > 0 && width > 0 && height > 0;
        }
    };
}
