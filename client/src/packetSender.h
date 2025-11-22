#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <optional>
#include <thread>
#include <vector>

#include "packetQueue.h"

#include <asio.hpp>

namespace calls {
    class PacketSender
    {
    public:
        PacketSender();
        ~PacketSender();

        void init(PacketQueue& queue, asio::ip::udp::socket& socket, asio::ip::udp::endpoint remoteEndpoint, std::function<void()> onErrorCallback);
        void start();
        void stop();
        bool isRunning() const;

    private:
        void run();
        void sendPacket(const Packet& packet);
        std::vector<std::vector<unsigned char>> splitPacket(const Packet& packet);
        void writeUint16(std::vector<unsigned char>& buffer, uint16_t value);
        void writeUint32(std::vector<unsigned char>& buffer, uint32_t value);
        void writeUint64(std::vector<unsigned char>& buffer, uint64_t value);

    private:
        std::optional<std::reference_wrapper<PacketQueue>> m_queue;
        std::atomic<bool> m_running;
        std::thread m_thread;
        asio::ip::udp::endpoint m_serverEndpoint;
        std::optional<std::reference_wrapper<asio::ip::udp::socket>> m_socket;
        std::function<void()> m_onErrorCallback;

        const std::size_t m_maxPayloadSize = 1300;
        const std::size_t m_headerSize = 18;
    };
}