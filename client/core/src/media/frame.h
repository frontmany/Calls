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
        int format = 0;  // AVPixelFormat
        int64_t pts = 0; // Presentation timestamp
            
        Frame() = default;
        Frame(const uint8_t* data, size_t size, int width, int height, int format = 0, int64_t pts = 0)
            : data(data), size(size), width(width), height(height), format(format), pts(pts) {}
            
        bool isValid() const {
            return data != nullptr && size > 0 && width > 0 && height > 0;
        }
    };
}
