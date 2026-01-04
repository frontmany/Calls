#pragma once
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "packetSender.h"
#include "packetReceiver.h"
#include "pingController.h"

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
            std::function<void(const unsigned char*, int, uint32_t, const asio::ip::udp::endpoint&)> onReceive,
            std::function<void(const asio::ip::udp::endpoint&)> onConnectionDown,
            std::function<void(const asio::ip::udp::endpoint&)> onConnectionRestored);

        void run();
        void stop();
        bool isRunning() const;
        bool send(const std::vector<unsigned char>& data, uint32_t type, const asio::ip::udp::endpoint& endpoint);
        bool send(std::vector<unsigned char>&& data, uint32_t type, const asio::ip::udp::endpoint& endpoint);
        bool send(const unsigned char* data, int size, uint32_t type, const asio::ip::udp::endpoint& endpoint);
        bool send(uint32_t type, const asio::ip::udp::endpoint& endpoint);
        void addEndpointForPing(const asio::ip::udp::endpoint& endpoint);
        void removePingMonitoring(const asio::ip::udp::endpoint& endpoint);

    private:
        void sendPing(const asio::ip::udp::endpoint& endpoint);
        uint64_t generateId();
        void sendPingResponse(const asio::ip::udp::endpoint& endpoint);

    private:
        asio::io_context m_context;
        asio::ip::udp::socket m_socket;
        asio::ip::udp::endpoint m_serverEndpoint;
        asio::executor_work_guard<asio::io_context::executor_type> m_workGuard;
        std::thread m_asioThread;

        PacketReceiver m_packetReceiver;
        PacketSender m_packetSender;
        std::unique_ptr<PingController> m_pingController;

        std::atomic<bool> m_running;
        std::atomic<uint64_t> m_nextPacketId;

        std::function<void(const unsigned char*, int, uint32_t, const asio::ip::udp::endpoint&)> m_onReceive;
        std::function<void(const asio::ip::udp::endpoint&)> m_onConnectionDown;
        std::function<void(const asio::ip::udp::endpoint&)> m_onConnectionRestored;
    };
    }
}

