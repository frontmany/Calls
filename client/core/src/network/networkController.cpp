#include "networkController.h"

#include <thread>
#include <utility>

#include "utilities/logger.h"
#include "packet.h"
#include "packetType.h"

namespace core {
    namespace network {
    NetworkController::NetworkController()
        : m_socket(m_context),
        m_workGuard(asio::make_work_guard(m_context)),
        m_running(false),
        m_nextPacketId(0U)
    {
    }

    NetworkController::~NetworkController() {
        stop();
    }

    bool NetworkController::init(const std::string& host,
        const std::string& udpPort,
        std::function<void(const unsigned char*, int, uint32_t)> onReceive,
        std::function<void()> onConnectionDown,
        std::function<void()> onConnectionRestored)
    {
        m_onReceive = std::move(onReceive);
        m_onConnectionDown = std::move(onConnectionDown);
        m_onConnectionRestored = std::move(onConnectionRestored);

        try {
            std::error_code ec;

            asio::ip::udp::resolver resolver(m_context);
            asio::ip::udp::resolver::results_type endpoints = resolver.resolve(asio::ip::udp::v4(), host, udpPort, ec);

            if (ec) {
                LOG_ERROR("Failed to resolve {}:{} - {}", host, udpPort, ec.message());
                return false;
            }

            if (endpoints.empty()) {
                LOG_ERROR("No endpoints found for {}:{}", host, udpPort);
                return false;
            }

            m_serverEndpoint = *endpoints.begin();

            if (m_socket.is_open()) {
                m_socket.close(ec);
                ec.clear();
            }

            m_socket.open(asio::ip::udp::v4(), ec);
            if (ec) {
                LOG_ERROR("Failed to open UDP socket: {}", ec.message());
                return false;
            }

            m_socket.set_option(asio::socket_base::reuse_address(true), ec);
            if (ec) {
                LOG_ERROR("Failed to set reuse_address on UDP socket: {}", ec.message());
                return false;
            }

            m_socket.bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), 0), ec);
            if (ec) {
                LOG_ERROR("Failed to bind UDP socket: {}", ec.message());
                return false;
            }

            ec.clear();
            auto localEp = m_socket.local_endpoint(ec);
            if (!ec)
                m_localUdpPort = static_cast<uint16_t>(localEp.port());

            std::function<void()> errorHandler = []() {
                // UDP errors no longer drive connection-down; TCP disconnect does.
            };

            auto pingReceivedHandler = [](uint32_t) {
                // Ping/pong removed; UDP is media-only. Ignore legacy types 0/1.
            };

            if (!m_packetReceiver.init(m_socket, m_onReceive, errorHandler, pingReceivedHandler, m_serverEndpoint)) {
                LOG_ERROR("Failed to initialize packet receiver");
                return false;
            }

            m_packetSender.init(m_socket, m_serverEndpoint, errorHandler);

            LOG_INFO("UDP controller initialized, server: {}:{}, local port {}", host, udpPort, m_localUdpPort);
            return true;
        }
        catch (const std::exception& e) {
            LOG_ERROR("Initialization error: {}", e.what());
            stop();
            return false;
        }
    }

    void NetworkController::start() {
        if (m_running.exchange(true)) {
            return;
        }

        m_asioThread = std::thread([this]() { m_context.run(); });
        m_packetReceiver.start();
    }

    void NetworkController::stop() {
        bool wasRunning = m_running.exchange(false);
        if (!wasRunning && !m_asioThread.joinable()) {
            return;
        }

        m_packetSender.stop();
        m_packetReceiver.stop();

        std::error_code ec;
        if (m_socket.is_open()) {
            m_socket.cancel(ec);
            if (ec) {
                LOG_WARN("Failed to cancel socket operations: {}", ec.message());
            }

            m_socket.close(ec);
            if (ec) {
                LOG_WARN("Failed to close socket: {}", ec.message());
            }
        }

        m_workGuard.reset();
        m_context.stop();

        if (m_asioThread.joinable()) {
            m_asioThread.join();
        }

        m_context.restart();
    }

    bool NetworkController::isRunning() const {
        return m_running.load();
    }

    uint16_t NetworkController::getLocalUdpPort() const {
        return m_localUdpPort;
    }

    bool NetworkController::send(const std::vector<unsigned char>& data, uint32_t type) {
        if (type == 0 || type == 1) {
            LOG_ERROR("Packet types 0 and 1 are reserved");
            return false;
        }

        Packet packet;
        packet.id = generateId();
        packet.type = type;
        packet.data = data;
        m_packetSender.send(packet);
        return true;
    }

    bool NetworkController::send(std::vector<unsigned char>&& data, uint32_t type) {
        if (type == 0 || type == 1) {
            LOG_ERROR("Packet types 0 and 1 are reserved");
            return false;
        }

        Packet packet;
        packet.id = generateId();
        packet.type = type;
        packet.data = std::move(data);
        m_packetSender.send(packet);
        return true;
    }

    uint64_t NetworkController::generateId() {
        return m_nextPacketId.fetch_add(1U, std::memory_order_relaxed);
    }

    void NetworkController::notifyConnectionRestored() {
        m_connectionDownNotified = false;
        m_packetReceiver.setConnectionDown(false);
    }

    void NetworkController::notifyConnectionDown() {
        if (!m_connectionDownNotified.exchange(true)) {
            m_packetReceiver.setConnectionDown(true);
            if (m_onConnectionDown) {
                m_onConnectionDown();
            }
        }
    }
    }
}
