#pragma once

#include <vector>

namespace core::media
{
    struct AudioPacket {
        std::vector<float> audioData;
        int samples;
    };
    
}