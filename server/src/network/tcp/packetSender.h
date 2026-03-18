#pragma once

#include <functional>
#include <optional>

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
        void startNextIfNeeded();
        void writeHeader();
        void writeBody();
        void resolveSending();

        asio::ip::tcp::socket& m_socket;
        utilities::SafeQueue<Packet>& m_queue;
        std::function<void()> m_onError;

        // Keep the currently-sending packet alive across async callbacks.
        std::optional<Packet> m_current;
    };
}
