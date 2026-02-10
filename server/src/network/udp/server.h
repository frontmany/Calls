#pragma once
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "network/udp/packetSender.h"
#include "network/udp/packetReceiver.h"

#include "asio.hpp"

namespace server::network::udp
{
    class Server {
    public:
        Server();
        ~Server();

        bool init(const std::string& port,
            std::function<void(const unsigned char*, int, uint32_t, const asio::ip::udp::endpoint&)> onReceive);

        void start();
        void stop();
        bool isRunning() const;
        bool send(const std::vector<unsigned char>& data, uint32_t type, const asio::ip::udp::endpoint& endpoint);
        bool send(std::vector<unsigned char>&& data, uint32_t type, const asio::ip::udp::endpoint& endpoint);
        bool send(const unsigned char* data, int size, uint32_t type, const asio::ip::udp::endpoint& endpoint);

    private:
        void run();
        void stop_internal();
        bool reinit();
        uint64_t generateId();

    private:
        asio::io_context m_context;
        asio::ip::udp::socket m_socket;
        asio::ip::udp::endpoint m_serverEndpoint;
        asio::executor_work_guard<asio::io_context::executor_type> m_workGuard;

        udp::PacketReceiver m_packetReceiver;
        udp::PacketSender m_packetSender;

        std::atomic<bool> m_running;
        std::atomic<uint64_t> m_nextPacketId;

        std::string m_port;
        std::function<void(const unsigned char*, int, uint32_t, const asio::ip::udp::endpoint&)> m_onReceive;
        std::thread m_ctxThread;
    };
}
