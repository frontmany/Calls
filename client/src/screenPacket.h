#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "packetTypes.h"

namespace calls {

struct ScreenChunkHeader final {
    uint32_t frameId = 0;
    uint16_t chunkIndex = 0;
    uint16_t chunksCount = 0;
    uint32_t payloadSize = 0;

    static constexpr std::size_t serializedSize() {
        return sizeof(ScreenChunkHeader);
    }
};

static_assert(sizeof(ScreenChunkHeader) == 12, "ScreenChunkHeader has unexpected size");

struct ScreenDatagram final {
    ScreenChunkHeader header{};
    PacketType type = PacketType::SCREEN;
    std::vector<unsigned char> payload;
};

}

