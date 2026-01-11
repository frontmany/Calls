#pragma once
#include <asio.hpp>

#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
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
        };

        struct ReceivedPacket {
            std::vector<unsigned char> data;
            uint32_t type;
        };

    public:
        PacketReceiver();
        ~PacketReceiver();

        bool init(asio::ip::udp::socket& socket,
            std::function<void(const unsigned char*, int, uint32_t)> onPacketReceived,
            std::function<void()> onErrorCallback,
            std::function<void(uint32_t)> onPingReceived);

        void start();
        void stop();
        bool isRunning() const;
        void setConnectionDown(bool isDown);

    private:
        void doReceive();
        void processDatagram(std::size_t bytesTransferred);
        void processReceivedPackets();
        void resetPendingPacket(uint64_t packetId, uint16_t totalChunks, uint32_t packetType);
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
        PendingPacket m_pendingPacket;
        core::utilities::SafeQueue<ReceivedPacket> m_receivedPacketsQueue;
        std::thread m_processingThread;
        const std::size_t m_headerSize = 18;
        std::function<void(const unsigned char*, int, uint32_t)> m_onPacketReceived;
        std::function<void()> m_onErrorCallback;
        std::function<void(uint32_t)> m_onPingReceived;
        std::atomic_bool m_connectionDown{false};
    };
    }
}
