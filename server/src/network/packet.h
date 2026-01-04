#pragma once

#include <cstdint>
#include <vector>
#include "asio.hpp"

namespace server
{
    namespace network 
    {
        struct Packet {
            uint64_t id;
            uint32_t type;
            std::vector<unsigned char> data;
            asio::ip::udp::endpoint endpoint;
        };
    }
}

