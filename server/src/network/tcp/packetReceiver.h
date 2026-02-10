#pragma once

#include <functional>

#include "network/tcp/packet.h"
#include "asio.hpp"

namespace server::network::tcp
{
    class PacketsReceiver {
    public:
        PacketsReceiver(
            asio::ip::tcp::socket& socket,
            std::function<void(Packet&&)> onPacket,
            std::function<void()> onDisconnect);

        void startReceiving();

    private:
        void readHeader();
        void readBody();

        asio::ip::tcp::socket& m_socket;
        Packet m_temporary;
        std::function<void(Packet&&)> m_onPacket;
        std::function<void()> m_onDisconnect;
    };
}
