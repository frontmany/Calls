#pragma once

#include <functional>
#include "tcp_packet.h"
#include "asio.hpp"

namespace core
{
    namespace network
    {
        class TcpPacketsReceiver {
        public:
            TcpPacketsReceiver(
                asio::ip::tcp::socket& socket,
                std::function<void(TcpPacket&&)> onPacket,
                std::function<void()> onDisconnect);

            void startReceiving();

        private:
            void readHeader();
            void readBody();

            asio::ip::tcp::socket& m_socket;
            TcpPacket m_temporary;
            std::function<void(TcpPacket&&)> m_onPacket;
            std::function<void()> m_onDisconnect;
        };
    }
}
