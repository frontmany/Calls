#pragma once

#include <cstdint>

struct ChunkHeader
{
    uint32_t id = 0;
    uint16_t number = 0;
    uint16_t chunksCount = 0;
    uint32_t payloadSize = 0;
};

