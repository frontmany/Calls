#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

#include "packet.h"
#include "utilities/safeQueue.h"

#include <asio.hpp>

namespace server
{
    namespace network 
    {
    class PacketSender
    {
    public:
        PacketSender();
        ~PacketSender();

        void init(asio::ip::udp::socket& socket, std::function<void()> onErrorCallback);
        void send(const Packet& packet);
        void stop();

    private:
        void startSendingIfIdle();
        void sendNextDatagram();
        void processNextPacketFromQueue();
        std::vector<std::vector<unsigned char>> splitPacket(const Packet& packetData);
        void writeUint16(std::vector<unsigned char>& buffer, uint16_t value);
        void writeUint32(std::vector<unsigned char>& buffer, uint32_t value);
        void writeUint64(std::vector<unsigned char>& buffer, uint64_t value);

    private:
        server::utilities::SafeQueue<Packet> m_packetQueue;
        std::atomic<bool> m_isSending;

        std::optional<std::reference_wrapper<asio::ip::udp::socket>> m_socket;
        std::function<void()> m_onErrorCallback;

        std::vector<std::vector<unsigned char>> m_currentDatagrams;
        std::size_t m_currentDatagramIndex;
        asio::ip::udp::endpoint m_currentEndpoint;

        const std::size_t m_maxPayloadSize = 1300;
        const std::size_t m_headerSize = 18;
    };
    }
}

