#pragma once

#include <functional>
#include <optional>
#include <atomic>

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
            std::function<void()> onError,
            std::function<ConnectionPtr()> lockConnection);

        void send();

    private:
        void startNextIfNeeded();
        void writeHeader();
        void writeBody();
        void resolveSending();

        asio::ip::tcp::socket& m_socket;
        utilities::SafeQueue<Packet>& m_queue;
        std::function<void()> m_onError;
        std::function<ConnectionPtr()> m_lockConnection;

        // Keep the currently-sending packet alive across async callbacks.
        std::optional<Packet> m_current;
        PacketHeader m_currentHeader{};
        std::atomic_bool m_sending{false};
    };
}
