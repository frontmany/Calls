#include "networkController.h"
#include "utilities/logger.h"

using namespace core::constant;
using namespace core::utilities;

namespace core::network 
{
    NetworkController::NetworkController(OnPacketReceived onPacketReceived, OnConnectionDown onConnectionDown)
        : m_onPacketReceived(onPacketReceived)
        , m_onConnectionDown(onConnectionDown)
    {
        m_workGuard = std::make_unique<asio::executor_work_guard<asio::io_context::executor_type>>(
            asio::make_work_guard(m_context));
        m_ioThread = std::thread([this]() { m_context.run(); });

        m_tcpClient = std::make_unique<tcp::Client>(
            m_context,
            [this](uint32_t type, const unsigned char* data, size_t size) {
                onTCPPacketReceived(type, data, size);
            },
            [this]() {
                onTCPConnectionDownInternal();
            }
        );

        m_udpClient = std::make_unique<udp::Client>(m_context);
    }

    NetworkController::~NetworkController() {
        disconnect();

        m_workGuard.reset();
        if (m_ioThread.joinable()) {
            m_ioThread.join();
        }
    }

    bool NetworkController::connectTCP(const std::string& tcpHost, const std::string& tcpPort) {
        try {
            m_networkConfig.setTcpParameters(tcpHost, tcpPort);

            if (m_tcpClient->connectSync(tcpHost, tcpPort, 10000)) {
                m_connected = true;
                LOG_INFO("NetworkController TCP connected to {}:{}", tcpHost, tcpPort);
                return true;
            }

            LOG_ERROR("Failed to connect TCP to {}:{}", tcpHost, tcpPort);
            m_networkConfig.resetTcpParameters();
            return false;
        }
        catch (const std::exception& e) {
            LOG_ERROR("Failed to connect TCP NetworkController: {}", e.what());
            m_networkConfig.resetTcpParameters();
            return false;
        }
    }

    bool NetworkController::establishConnection(const std::string& tcpHost, const std::string& tcpPort,
        const std::string& udpHost, const std::string& udpPort)
    {
        if (isTCPConnected()) {
            return true;
        }
        const auto& savedHost = m_networkConfig.getTcpHost();
        const auto& savedPort = m_networkConfig.getTcpPort();
        if (!savedHost.empty() && !savedPort.empty()) {
            return tryReconnectTCP(5);
        }
        return connectTCP(tcpHost, tcpPort) && runUDP(udpHost, udpPort);
    }

    void NetworkController::disconnect() {
        disconnectTCP();
        stopUDP();
        m_networkConfig.resetTcpParameters();
        m_networkConfig.resetUdpParameters();
    }

    bool NetworkController::tryReconnectTCP(int attempts) {
        const auto& host = m_networkConfig.getTcpHost();
        const auto& port = m_networkConfig.getTcpPort();

        if (host.empty() || port.empty()) {
            LOG_ERROR("Cannot reconnect TCP: no saved host/port");
            return false;
        }

        disconnectTCP();

        for (int i = 0; i < attempts; ++i) {
            LOG_INFO("TCP reconnect attempt {}/{}", i + 1, attempts);

            if (m_tcpClient->connectSync(host, port, 5000)) {
                m_connected = true;
                LOG_INFO("TCP reconnected on attempt {}", i + 1);
                return true;
            }

            m_tcpClient->disconnect();

            if (i + 1 < attempts) {
                std::this_thread::sleep_for(std::chrono::milliseconds(600));
            }
        }

        LOG_WARN("TCP reconnect failed after {} attempts", attempts);
        return false;
    }

    bool NetworkController::runUDP(const std::string& udpHost, const std::string& udpPort) {
        try {
            m_networkConfig.setUdpParameters(udpHost, udpPort);

            // Initialize UDP
            if (!m_udpClient->initialize(udpHost, udpPort,
                [this](const unsigned char* data, int size, uint32_t type) {
                    onUDPPacketReceived(data, size, type);
                })) {
                LOG_ERROR("Failed to initialize UDP client");
                return false;
            }

            // Start UDP
            m_udpClient->start();
            LOG_INFO("NetworkController UDP running on {}:{}", udpHost, udpPort);
            return true;
        }
        catch (const std::exception& e) {
            LOG_ERROR("Failed to start UDP NetworkController: {}", e.what());
            m_networkConfig.resetUdpParameters();

            return false;
        }
    }

    void NetworkController::disconnectTCP() {
        if (m_tcpClient) {
            m_tcpClient->disconnect();
        }
        m_connected = false;
        LOG_INFO("NetworkController TCP disconnected");
    }

    void NetworkController::stopUDP() {
        if (m_udpClient) {
            m_udpClient->stop();
        }
        LOG_INFO("NetworkController UDP stopped");
    }

    bool NetworkController::sendTCP(const std::vector<unsigned char>& data, PacketType type) {
        if (!m_tcpClient || !m_tcpClient->isConnected()) {
            LOG_WARN("TCP client not connected for packet type {}", packetTypeToString(type));
            return false;
        }
        return m_tcpClient->send(static_cast<uint32_t>(type), data);
    }

    bool NetworkController::sendUDP(const std::vector<unsigned char>& data, PacketType type) {
        if (!m_udpClient || !m_udpClient->isRunning()) {
            LOG_WARN("UDP client not running for packet type {}", packetTypeToString(type));
            return false;
        }
        return m_udpClient->send(data, static_cast<uint32_t>(type));
    }

    bool NetworkController::isTCPConnected() const {
        return m_tcpClient && m_tcpClient->isConnected();
    }

    bool NetworkController::isUDPRunning() const {
        return m_udpClient && m_udpClient->isRunning();
    }

    uint16_t NetworkController::getUDPLocalPort() const {
        return m_udpClient ? m_udpClient->getLocalPort() : 0;
    }

    void NetworkController::onTCPPacketReceived(uint32_t type, const unsigned char* data, size_t size) {
        PacketType packetType = static_cast<PacketType>(type);
        if (m_onPacketReceived) {
            m_onPacketReceived(data, static_cast<int>(size), packetType);
        }
    }

    void NetworkController::onUDPPacketReceived(const unsigned char* data, int size, uint32_t type) {
        PacketType packetType = static_cast<PacketType>(type);
        if (m_onPacketReceived) {
            m_onPacketReceived(data, size, packetType);
        }
    }

    void NetworkController::onTCPConnectionDownInternal() {
        LOG_WARN("TCP connection lost");
        if (m_onConnectionDown) {
            m_onConnectionDown();
        }
    }
}
