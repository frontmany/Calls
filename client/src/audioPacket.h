#pragma once

#include <vector>

struct AudioPacket {
    std::vector<float> audioData;
    int samples;
};