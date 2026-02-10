#pragma once

#include <functional>

#include "network/tcp/packet.h"
#include "utilities/safeQueue.h"
#include "asio.hpp"

namespace server::network::tcp
{
    class PacketSender {
    public:
        PacketSender(
            asio::ip::tcp::socket& socket,
            utilities::SafeQueue<Packet>& queue,
            std::function<void()> onError);

        void send();

    private:
        void writeHeader();
        void writeBody(const Packet* packet);
        void resolveSending();

        asio::ip::tcp::socket& m_socket;
        utilities::SafeQueue<Packet>& m_queue;
        std::function<void()> m_onError;
    };
}
