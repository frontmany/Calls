#pragma once

#include <cstdint>
#include <functional>
#include <memory>

#include "network/tcp/packet.h"
#include "network/tcp/packetReceiver.h"
#include "network/tcp/packetSender.h"
#include "utilities/safeQueue.h"
#include "asio.hpp"

namespace server::network::tcp
{
    class Connection : public std::enable_shared_from_this<Connection> {
    public:
        Connection(
            asio::io_context& ctx,
            asio::ip::tcp::socket&& socket,
            std::function<void(OwnedPacket&&)> onPacket,
            std::function<void(Connection)> onDisconnected);

        void send(const Packet& packet);
        void close();
        asio::ip::tcp::endpoint remoteEndpoint() const;

    private:
        void readHandshake();
        void writeHandshake();

        asio::io_context& m_ctx;
        asio::ip::tcp::socket m_socket;
        utilities::SafeQueue<Packet> m_outQueue;
        uint64_t m_handshakeOut = 0;
        uint64_t m_handshakeIn = 0;

        PacketsReceiver m_receiver;
        PacketSender m_sender;

        std::function<void(OwnedPacket&&)> m_onPacket;
        std::function<void(Connection)> m_onDisconnected;
    };
}