#pragma once
#include <asio.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "packetType.h"
#include "utilities/safeQueue.h"

namespace core
{
    namespace network 
    {
    class PacketReceiver {
    private:
        struct PendingPacket {
            uint64_t packetId = 0;
            uint16_t totalChunks = 0;
            std::size_t receivedChunks = 0;
            std::vector<std::vector<unsigned char>> chunks;
            uint32_t type = 0;
            std::chrono::steady_clock::time_point lastUpdated{};
        };

        struct ReceivedPacket {
            std::vector<unsigned char> data;
            uint32_t type;
        };

        struct AssemblyJob {
            std::vector<std::vector<unsigned char>> chunks;
            uint32_t type = 0;
        };

    public:
        PacketReceiver();
        ~PacketReceiver();

        bool init(asio::ip::udp::socket& socket,
            std::function<void(const unsigned char*, int, uint32_t)> onPacketReceived,
            std::function<void()> onErrorCallback,
            std::function<void(uint32_t)> onPingReceived,
            const asio::ip::udp::endpoint& serverEndpoint);

        void start();
        void stop();
        bool isRunning() const;
        void setConnectionDown(bool isDown);

    private:
        using PendingPacketMap = std::unordered_map<uint64_t, PendingPacket>;

        void doReceive();
        void processDatagram(std::size_t bytesTransferred);
        void processReceivedPackets();
        void initPendingPacket(PendingPacket& packet, uint64_t packetId, uint16_t totalChunks, uint32_t packetType,
            std::chrono::steady_clock::time_point now);
        void pruneExpiredPackets(PendingPacketMap& packets, std::chrono::steady_clock::time_point now);
        void evictOldestPacket(PendingPacketMap& packets);
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
        PendingPacketMap m_pendingPackets;
        core::utilities::SafeQueue<ReceivedPacket> m_receivedPacketsQueue;
        core::utilities::SafeQueue<AssemblyJob> m_assemblyQueue;
        std::thread m_processingThread;
        const std::size_t m_headerSize = 18;
        const std::size_t m_maxPendingPackets = 8;
        const std::chrono::milliseconds m_pendingPacketTimeout{3000};
        std::function<void(const unsigned char*, int, uint32_t)> m_onPacketReceived;
        std::function<void()> m_onErrorCallback;
        std::function<void(uint32_t)> m_onPingReceived;
        std::atomic_bool m_connectionDown{false};
        asio::ip::udp::endpoint m_serverEndpoint;
    };
    }
}
