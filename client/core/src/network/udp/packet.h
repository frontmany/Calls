#pragma once

#include <cstdint>
#include <vector>

namespace core::network::udp {

struct Packet {
    uint64_t id;
    uint32_t type;
    std::vector<unsigned char> data;
};

}
