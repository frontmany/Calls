#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace server::network::tcp
{
    class Connection;

#pragma pack(push, 1)
    struct PacketHeader {
        uint32_t type = 0;
        uint32_t bodySize = 0;
    };
#pragma pack(pop)

    struct Packet {
        uint32_t type = 0;
        uint32_t bodySize = 0;
        std::vector<uint8_t> body;

        static constexpr size_t headerSize() { return sizeof(PacketHeader); }
        const PacketHeader* headerPtr() const {
            return reinterpret_cast<const PacketHeader*>(&type);
        }
        PacketHeader* headerPtrMut() {
            return reinterpret_cast<PacketHeader*>(&type);
        }
    };

    using ConnectionPtr = std::shared_ptr<Connection>;

    struct OwnedPacket {
        ConnectionPtr connection;
        Packet packet;
    };
}
