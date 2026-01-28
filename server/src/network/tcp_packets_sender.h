#pragma once

#include <functional>

#include "tcp_packet.h"
#include "utilities/safeQueue.h"
#include "asio.hpp"

namespace server
{
    namespace network
    {
        class TcpPacketsSender {
        public:
            TcpPacketsSender(
                asio::ip::tcp::socket& socket,
                utilities::SafeQueue<TcpPacket>& queue,
                std::function<void()> onError);

            void send();

        private:
            void writeHeader();
            void writeBody(const TcpPacket* packet);
            void resolveSending();

            asio::ip::tcp::socket& m_socket;
            utilities::SafeQueue<TcpPacket>& m_queue;
            std::function<void()> m_onError;
        };
    }
}
