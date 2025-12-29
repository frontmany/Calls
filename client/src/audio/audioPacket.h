#pragma once

#include <vector>

namespace audio 
{
    struct AudioPacket {
        std::vector<float> audioData;
        int samples;
    };
}