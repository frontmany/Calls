#pragma once

#include <vector>
#include <cstdint>
#include "IPacketSender.h"
#include "network/networkController.h"
#include "asio.hpp"

namespace server
{
    namespace services
    {
        // Адаптер NetworkController к интерфейсу IPacketSender
        class NetworkPacketSender : public IPacketSender {
        public:
            NetworkPacketSender(server::network::NetworkController& networkController)
                : m_networkController(networkController)
            {
            }

            bool send(const std::vector<unsigned char>& data, 
                     uint32_t type, 
                     const asio::ip::udp::endpoint& endpoint) override {
                return m_networkController.send(data, type, endpoint);
            }

            bool send(std::vector<unsigned char>&& data, 
                     uint32_t type, 
                     const asio::ip::udp::endpoint& endpoint) override {
                return m_networkController.send(std::move(data), type, endpoint);
            }

            bool send(const unsigned char* data, 
                     int size, 
                     uint32_t type, 
                     const asio::ip::udp::endpoint& endpoint) override {
                return m_networkController.send(data, size, type, endpoint);
            }

        private:
            server::network::NetworkController& m_networkController;
        };
    }
}
