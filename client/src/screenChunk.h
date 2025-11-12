#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "packetTypes.h"

namespace calls 
{
    struct ScreenChunkHeader {
        uint32_t id = 0;
        uint16_t number = 0;
        uint16_t chunksCount = 0;
        uint32_t payloadSize = 0;
    };

    struct ScreenChunk {
        ScreenChunkHeader header;
        std::vector<unsigned char> payload;
    };

}

