#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <vector>

#include "packetTypes.h"

namespace network 
{
    struct Packet {
        uint64_t id;
        uint32_t type;
        std::vector<unsigned char> data;
    };
}