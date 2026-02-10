#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "constants/packetType.h"
#include "network/tcp/client.h"
#include "network/udp/client.h"
#include "network/networkConfig.h"
#include "asio.hpp"

namespace core::network
{
    class NetworkController {
    public:
        using OnPacketReceived = std::function<void(const unsigned char* data, int size, core::constant::PacketType type)>;
        using OnConnectionDown = std::function<void()>;
        
        NetworkController(OnPacketReceived onPacketReceived, OnConnectionDown onConnectionDown);
        ~NetworkController();

        bool connectTCP(const std::string& tcpHost, const std::string& tcpPort);
        bool runUDP(const std::string& udpHost, const std::string& udpPort);
        void disconnectTCP();
        void stopUDP();
        bool tryReconnectTCP(int attempts);

        bool sendTCP(const std::vector<unsigned char>& data, core::constant::PacketType type);
        bool sendUDP(const std::vector<unsigned char>& data, core::constant::PacketType type);

        bool isTCPConnected() const;
        bool isUDPRunning() const;

        uint16_t getUDPLocalPort() const;

    private:
        void onTCPPacketReceived(uint32_t type, const unsigned char* data, size_t size);
        void onUDPPacketReceived(const unsigned char* data, int size, uint32_t type);
        void onTCPConnectionDownInternal();

    private:
        asio::io_context m_context;
        std::unique_ptr<asio::executor_work_guard<asio::io_context::executor_type>> m_workGuard;
        std::thread m_ioThread;

        NetworkConfig m_networkConfig;

        std::unique_ptr<tcp::Client> m_tcpClient;
        std::unique_ptr<udp::Client> m_udpClient;

        OnPacketReceived m_onPacketReceived;
        OnConnectionDown m_onConnectionDown;
            
        std::atomic<bool> m_connected{false};
    };
}