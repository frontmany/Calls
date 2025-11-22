#pragma once
#include <asio.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "packetTypes.h"

namespace calls {
    class PacketReceiver {
    private:
        struct PendingPacket {
            uint16_t totalChunks = 0;
            std::vector<std::vector<unsigned char>> chunks;
            std::size_t receivedChunks = 0;
            PacketType type = PacketType::PING;
            bool typeSet = false;
            std::chrono::steady_clock::time_point lastUpdate;
        };

    public:
        PacketReceiver();
        ~PacketReceiver();

        bool init(asio::ip::udp::socket& socket,
            std::function<void(const unsigned char*, int, PacketType)> onPacketReceived,
            std::function<void()> onErrorCallback);

        void start();
        void stop();
        bool isRunning() const;

    private:
        void doReceive();
        void processDatagram(std::size_t bytesTransferred);
        void handleChunk(uint64_t packetId, uint16_t chunkIndex, uint16_t chunkCount, PacketType packetType,
            const unsigned char* payload, std::size_t payloadSize);
        void cleanupExpired();
        uint16_t readUint16(const unsigned char* data);
        uint32_t readUint32(const unsigned char* data);
        uint64_t readUint64(const unsigned char* data);
        void notifyError(const std::error_code& ec);

    private:
        std::optional<std::reference_wrapper<asio::ip::udp::socket>> m_socket;
        asio::ip::udp::endpoint m_remoteEndpoint;
        std::array<unsigned char, 1500> m_buffer{};
        std::atomic<bool> m_running;
        std::mutex m_stateMutex;
        std::unordered_map<uint64_t, PendingPacket> m_pendingPackets;
        std::function<void(const unsigned char*, int, PacketType)> m_onPacketReceived;
        std::function<void()> m_onErrorCallback;

        const std::size_t m_headerSize = 18;
        const std::chrono::seconds m_packetTimeout = std::chrono::seconds(2);
    };
}
