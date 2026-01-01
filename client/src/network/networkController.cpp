#include "networkController.h"
#include "pingController.h"

#include <thread>
#include <utility>

#include "logger.h"
#include "packet.h"
#include "packetType.h"

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
        const std::string& port,
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
            asio::ip::udp::resolver::results_type endpoints = resolver.resolve(asio::ip::udp::v4(), host, port, ec);

            if (ec) {
                LOG_ERROR("Failed to resolve {}:{} - {}", host, port, ec.message());
                return false;
            }

            if (endpoints.empty()) {
                LOG_ERROR("No endpoints found for {}:{}", host, port);
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

            m_socket.connect(m_serverEndpoint, ec);
            if (ec) {
                LOG_ERROR("Failed to connect UDP socket to server {}:{} - {}", host, port, ec.message());
                return false;
            }

            std::function<void()> errorHandler = [this]() {
                if (m_onConnectionDown) {
                    m_onConnectionDown();
                }
                if (m_pingController) {
                    m_pingController->setConnectionError();
                }
            };

            auto pingReceivedHandler = [this](uint32_t pingType) {
                if (pingType == 0) {
                    sendPingResponse();
                }
                else {
                    m_pingController->handlePingSuccess();
                }
                };

            if (!m_packetReceiver.init(m_socket, m_onReceive, errorHandler, pingReceivedHandler)) {
                LOG_ERROR("Failed to initialize packet receiver");
                return false;
            }

            m_packetSender.init(m_socket, m_serverEndpoint, errorHandler);

            auto sendPingCallback = [this]() {
                sendPing();
            };

            m_pingController = std::make_unique<PingController>(sendPingCallback, m_onConnectionDown, m_onConnectionRestored);

            LOG_INFO("Network controller initialized, server: {}:{}", host, port);
            return true;
        }
        catch (const std::exception& e) {
            LOG_ERROR("Initialization error: {}", e.what());
            stop();
            return false;
        }
    }

    void NetworkController::run() {
        if (m_running.exchange(true)) {
            return;
        }

        m_asioThread = std::thread([this]() {m_context.run(); });
        m_packetReceiver.start();
        m_pingController->start();
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

        if (m_pingController) {
            m_pingController->stop();
        }

        m_context.restart();
    }

    bool NetworkController::isRunning() const {
        return m_running.load();
    }

    bool NetworkController::send(const std::vector<unsigned char>& data, uint32_t type) {
        if (type == 0 || type == 1) {
            LOG_ERROR("Packet types 0 and 1 are reserved for ping");
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
            LOG_ERROR("Packet types 0 and 1 are reserved for ping");
            return false;
        }

        Packet packet;
        packet.id = generateId();
        packet.type = type;
        packet.data = std::move(data);
        m_packetSender.send(packet);
        return true;
    }

    bool NetworkController::send(uint32_t type) {
        if (type == 0 || type == 1) {
            LOG_ERROR("Packet types 0 and 1 are reserved for ping");
            return false;
        }

        Packet packet;
        packet.id = generateId();
        packet.type = type;
        packet.data.clear();
        m_packetSender.send(packet);
        return true;
    }

    void NetworkController::sendPing() {
        Packet packet;
        packet.id = generateId();
        packet.type = 0;
        packet.data.clear();
        m_packetSender.send(packet);
    }

    uint64_t NetworkController::generateId() {
        return m_nextPacketId.fetch_add(1U, std::memory_order_relaxed);
    }

    void NetworkController::sendPingResponse() {
        Packet packet;
        packet.id = generateId();
        packet.type = 1;
        packet.data.clear();
        m_packetSender.send(packet);
    }

    void NetworkController::setConnectionError() {
        if (m_pingController) {
            m_pingController->setConnectionError();
        }
    }
}