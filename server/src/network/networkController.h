#pragma once
#include <array>
#include <functional>
#include <string>
#include <vector>

#include "asio.hpp"
#include "network/tcp/server.h"
#include "network/udp/server.h"

namespace server
{
    namespace network
    {
        class NetworkController {
        public:
            NetworkController(uint16_t tcpPort,
                const std::string& udpPort,
                tcp::Server::OnPacket onTcpPacket,
                tcp::Server::OnDisconnect onTcpDisconnect,
                std::function<void(const unsigned char*, int, uint32_t, const asio::ip::udp::endpoint&, const std::array<unsigned char, 32>&)> onUdpReceive);

            ~NetworkController();

            void start();
            void stop();

            bool sendUdp(const std::vector<unsigned char>& data, uint32_t type, const asio::ip::udp::endpoint& endpoint);
            bool sendUdp(std::vector<unsigned char>&& data, uint32_t type, const asio::ip::udp::endpoint& endpoint);
            bool sendUdp(const unsigned char* data, int size, uint32_t type, const asio::ip::udp::endpoint& endpoint);

        private:
            tcp::Server m_tcpServer;
            udp::Server m_udpServer;
        };
    }
}
