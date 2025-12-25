#pragma once
#include <asio.hpp>

#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "packetTypes.h"
#include "safeQueue.h"

namespace calls {
    class PacketReceiver {
    private:
        struct PendingPacket {
            uint16_t totalChunks = 0;
            std::vector<std::vector<unsigned char>> chunks;
            std::size_t receivedChunks = 0;
            PacketType type = PacketType::PING;
            bool typeSet = false;
        };

        struct ReceivedPacket {
            std::vector<unsigned char> data;
            PacketType type;
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
        void processReceivedPackets();
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
        std::optional<PendingPacket> m_pendingPacket;
        std::function<void(const unsigned char*, int, PacketType)> m_onPacketReceived;
        std::function<void()> m_onErrorCallback;
        SafeQueue<ReceivedPacket> m_receivedPacketsQueue;
        std::thread m_processingThread;

        const std::size_t m_headerSize = 18;
    };
}
