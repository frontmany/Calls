#pragma once

#include <cstdint>
#include <vector>

#include "packetType.h"

namespace core::network::udp {

struct Packet {
    uint64_t id;
    uint32_t type;
    std::vector<unsigned char> data;
};

}
