#pragma once

#include <functional>
#include "network/tcp/packet.h"
#include "utilities/safeQueue.h"
#include "asio.hpp"

namespace core::network::tcp {

class PacketsSender {
public:
    PacketsSender(
        asio::ip::tcp::socket& socket,
        core::utilities::SafeQueue<Packet>& queue,
        std::function<void()> onError);

    void send();

private:
    void writeHeader();
    void writeBody(const Packet* packet);
    void resolveSending();

    asio::ip::tcp::socket& m_socket;
    core::utilities::SafeQueue<Packet>& m_queue;
    std::function<void()> m_onError;
};

}