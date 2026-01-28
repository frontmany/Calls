#pragma once

#include <cstdint>
#include <functional>
#include <memory>

#include "tcp_packet.h"
#include "tcp_packets_receiver.h"
#include "tcp_packets_sender.h"
#include "utilities/safeQueue.h"
#include "asio.hpp"

namespace server
{
    namespace network
    {
        class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
        public:
            TcpConnection(
                asio::io_context& ctx,
                asio::ip::tcp::socket&& socket,
                std::function<void(OwnedTcpPacket&&)> onPacket,
                std::function<void(TcpConnectionPtr)> onDisconnected);

            void sendPacket(const TcpPacket& packet);
            void close();
            asio::ip::tcp::endpoint remoteEndpoint() const;

        private:
            void readHandshake();
            void writeHandshake();

            asio::io_context& m_ctx;
            asio::ip::tcp::socket m_socket;
            utilities::SafeQueue<TcpPacket> m_outQueue;
            uint64_t m_handshakeOut = 0;
            uint64_t m_handshakeIn = 0;

            TcpPacketsReceiver m_receiver;
            TcpPacketsSender m_sender;

            std::function<void(OwnedTcpPacket&&)> m_onPacket;
            std::function<void(TcpConnectionPtr)> m_onDisconnected;
        };
    }
}