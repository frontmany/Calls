#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "network/udp/packetReceiver.h"
#include "network/udp/packetSender.h"
#include "asio.hpp"

namespace core::network::udp 
{
    class Client {
    public:
        explicit Client(asio::io_context& context);
        ~Client();

        bool initialize(const std::string& host,
            const std::string& port,
            std::function<void(const unsigned char*, int, uint32_t)> onReceive);

        void start();
        void stop();
        bool isRunning() const;

        uint16_t getLocalPort() const;

        bool send(const std::vector<unsigned char>& data, uint32_t type);
        bool send(std::vector<unsigned char>&& data, uint32_t type);

    private:
        uint64_t generateId();

        asio::io_context& m_context;
        asio::ip::udp::socket m_socket;
        asio::ip::udp::endpoint m_serverEndpoint;

        PacketReceiver m_packetReceiver;
        PacketSender m_packetSender;

        std::atomic<bool> m_running;
        std::atomic<uint64_t> m_nextPacketId;
        uint16_t m_localPort;

        std::function<void(const unsigned char*, int, uint32_t)> m_onReceive;
    };

}
