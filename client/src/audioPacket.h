#pragma once

#include <vector>

namespace calls {

struct AudioPacket {
    std::vector<float> audioData;
    int samples;
};

}