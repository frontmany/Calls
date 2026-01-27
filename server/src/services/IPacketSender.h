#pragma once

#include <vector>
#include <cstdint>
#include "packetType.h"
#include "asio.hpp"

namespace server
{
    namespace services
    {
        // Интерфейс для отправки пакетов
        class IPacketSender {
        public:
            virtual ~IPacketSender() = default;

            // Отправить пакет
            virtual bool send(const std::vector<unsigned char>& data, 
                            uint32_t type, 
                            const asio::ip::udp::endpoint& endpoint) = 0;

            // Отправить пакет (move семантика)
            virtual bool send(std::vector<unsigned char>&& data, 
                            uint32_t type, 
                            const asio::ip::udp::endpoint& endpoint) = 0;

            // Отправить пакет из raw данных
            virtual bool send(const unsigned char* data, 
                            int size, 
                            uint32_t type, 
                            const asio::ip::udp::endpoint& endpoint) = 0;
        };
    }
}
