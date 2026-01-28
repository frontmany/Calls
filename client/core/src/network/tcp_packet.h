#pragma once

#include <cstdint>
#include <vector>

namespace core
{
    namespace network
    {
#pragma pack(push, 1)
        struct TcpPacketHeader {
            uint32_t type = 0;
            uint32_t bodySize = 0;
        };
#pragma pack(pop)

        struct TcpPacket {
            uint32_t type = 0;
            uint32_t bodySize = 0;
            std::vector<uint8_t> body;

            static constexpr size_t headerSize() { return sizeof(TcpPacketHeader); }
            const TcpPacketHeader* headerPtr() const {
                return reinterpret_cast<const TcpPacketHeader*>(&type);
            }
            TcpPacketHeader* headerPtrMut() {
                return reinterpret_cast<TcpPacketHeader*>(&type);
            }
        };
    }
}
