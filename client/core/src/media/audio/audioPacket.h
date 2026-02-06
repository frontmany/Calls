#pragma once

#include <vector>

namespace core
{
    namespace media 
    {
        struct AudioPacket {
            std::vector<float> audioData;
            int samples;
        };
    }
}