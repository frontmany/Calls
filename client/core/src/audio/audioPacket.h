#pragma once

#include <vector>

namespace core
{
    namespace audio 
    {
        struct AudioPacket {
            std::vector<float> audioData;
            int samples;
        };
    }
}