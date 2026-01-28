#pragma once
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "packetSender.h"
#include "packetReceiver.h"
#include "asio.hpp"

namespace server
{
    namespace network
    {
        class NetworkController {
        public:
            NetworkController();
            ~NetworkController();

            bool init(const std::string& port,
                std::function<void(const unsigned char*, int, uint32_t, const asio::ip::udp::endpoint&)> onReceive);

            void run();
            void stop();
            bool isRunning() const;
            bool send(const std::vector<unsigned char>& data, uint32_t type, const asio::ip::udp::endpoint& endpoint);
            bool send(std::vector<unsigned char>&& data, uint32_t type, const asio::ip::udp::endpoint& endpoint);
            bool send(const unsigned char* data, int size, uint32_t type, const asio::ip::udp::endpoint& endpoint);

        private:
            uint64_t generateId();

        private:
            asio::io_context m_context;
            asio::ip::udp::socket m_socket;
            asio::ip::udp::endpoint m_serverEndpoint;
            asio::executor_work_guard<asio::io_context::executor_type> m_workGuard;

            PacketReceiver m_packetReceiver;
            PacketSender m_packetSender;

            std::atomic<bool> m_running;
            std::atomic<uint64_t> m_nextPacketId;

            std::function<void(const unsigned char*, int, uint32_t, const asio::ip::udp::endpoint&)> m_onReceive;
        };
    }
}
