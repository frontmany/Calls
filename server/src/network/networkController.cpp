#include "networkController.h"

#include <utility>
#include <functional>

#include "utilities/logger.h"
#include "packet.h"

namespace server {
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

    bool NetworkController::init(const std::string& port,
        std::function<void(const unsigned char*, int, uint32_t, const asio::ip::udp::endpoint&)> onReceive,
        std::function<void(const asio::ip::udp::endpoint&)> onConnectionDown,
        std::function<void(const asio::ip::udp::endpoint&)> onConnectionRestored)
    {
        m_onReceive = std::move(onReceive);
        m_onConnectionDown = std::move(onConnectionDown);
        m_onConnectionRestored = std::move(onConnectionRestored);

        try {
            std::error_code ec;

            asio::ip::udp::resolver resolver(m_context);
            asio::ip::udp::resolver::results_type endpoints = resolver.resolve(asio::ip::udp::v4(), "0.0.0.0", port, ec);

            if (ec) {
                LOG_ERROR("Failed to resolve 0.0.0.0:{} - {}", port, ec.message());
                return false;
            }

            if (endpoints.empty()) {
                LOG_ERROR("No endpoints found for 0.0.0.0:{}", port);
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

            m_socket.bind(m_serverEndpoint, ec);
            if (ec) {
                LOG_ERROR("Failed to bind UDP socket: {}", ec.message());
                return false;
            }

            std::function<void()> errorHandler = [this]() {
                LOG_ERROR("Packet sender/receiver error on server - send or receive failed");
            };

            auto pingReceivedHandler = [this](uint32_t pingType, const asio::ip::udp::endpoint& endpoint) {
                if (pingType == 0) {
                    sendPingResponse(endpoint);
                    if (m_pingController) {
                        m_pingController->addEndpoint(endpoint);
                    }
                }
                else {
                    if (m_pingController) {
                        m_pingController->handlePingSuccess(endpoint);
                    }
                }
            };

            if (!m_packetReceiver.init(m_socket, m_onReceive, errorHandler, pingReceivedHandler)) {
                LOG_ERROR("Failed to initialize packet receiver");
                return false;
            }

            m_packetSender.init(m_socket, errorHandler);

            auto sendPingCallback = [this](const asio::ip::udp::endpoint& endpoint) {
                sendPing(endpoint);
            };

            m_pingController = std::make_unique<PingController>(
                sendPingCallback,
                std::bind(&NetworkController::handleConnectionDown, this, std::placeholders::_1),
                std::bind(&NetworkController::handleConnectionRestored, this, std::placeholders::_1));

            LOG_INFO("Network controller initialized, server port: {}", port);
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

        m_packetReceiver.start();
        m_pingController->start();

        try {
            m_context.run();
        }
        catch (const std::exception& e) {
            LOG_ERROR("asio io_context.run() threw; receive/send loop stopped: {}", e.what());
            throw;
        }
        catch (...) {
            LOG_ERROR("asio io_context.run() threw unknown exception; receive/send loop stopped");
            throw;
        }
    }

    void NetworkController::stop() {
        bool wasRunning = m_running.exchange(false);
        if (!wasRunning) {
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

        if (m_pingController) {
            m_pingController->stop();
        }

        m_context.restart();
    }

    bool NetworkController::isRunning() const {
        return m_running.load();
    }

    bool NetworkController::send(const std::vector<unsigned char>& data, uint32_t type, const asio::ip::udp::endpoint& endpoint) {
        if (type == 0 || type == 1) {
            LOG_ERROR("Packet types 0 and 1 are reserved for ping");
            return false;
        }

        Packet packet;
        packet.id = generateId();
        packet.type = type;
        packet.data = data;
        packet.endpoint = endpoint;
        m_packetSender.send(packet);
        return true;
    }

    bool NetworkController::send(std::vector<unsigned char>&& data, uint32_t type, const asio::ip::udp::endpoint& endpoint) {
        if (type == 0 || type == 1) {
            LOG_ERROR("Packet types 0 and 1 are reserved for ping");
            return false;
        }

        Packet packet;
        packet.id = generateId();
        packet.type = type;
        packet.data = std::move(data);
        packet.endpoint = endpoint;
        m_packetSender.send(packet);
        return true;
    }

    bool NetworkController::send(const unsigned char* data, int size, uint32_t type, const asio::ip::udp::endpoint& endpoint) {
        if (type == 0 || type == 1) {
            LOG_ERROR("Packet types 0 and 1 are reserved for ping");
            return false;
        }

        if (!data || size <= 0) {
            LOG_ERROR("Invalid data pointer or size");
            return false;
        }

        std::vector<unsigned char> dataVector(data, data + size);
        return send(std::move(dataVector), type, endpoint);
    }

    bool NetworkController::send(uint32_t type, const asio::ip::udp::endpoint& endpoint) {
        if (type == 0 || type == 1) {
            LOG_ERROR("Packet types 0 and 1 are reserved for ping");
            return false;
        }

        Packet packet;
        packet.id = generateId();
        packet.type = type;
        packet.data.clear();
        packet.endpoint = endpoint;
        m_packetSender.send(packet);
        return true;
    }

    void NetworkController::sendPing(const asio::ip::udp::endpoint& endpoint) {
        Packet packet;
        packet.id = generateId();
        packet.type = 0;
        packet.data.clear();
        packet.endpoint = endpoint;
        m_packetSender.send(packet);
    }

    uint64_t NetworkController::generateId() {
        return m_nextPacketId.fetch_add(1U, std::memory_order_relaxed);
    }

    void NetworkController::sendPingResponse(const asio::ip::udp::endpoint& endpoint) {
        Packet packet;
        packet.id = generateId();
        packet.type = 1;
        packet.data.clear();
        packet.endpoint = endpoint;
        m_packetSender.send(packet);
    }

    std::string NetworkController::makeEndpointKey(const asio::ip::udp::endpoint& endpoint) const {
        return endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
    }

    void NetworkController::handleConnectionDown(const asio::ip::udp::endpoint& endpoint) {
        if (!m_onConnectionDown) {
            return;
        }

        bool shouldNotify = false;
        {
            std::lock_guard<std::mutex> lock(m_connectionStateMutex);
            shouldNotify = m_downEndpoints.insert(makeEndpointKey(endpoint)).second;
        }

        if (shouldNotify) {
            m_onConnectionDown(endpoint);
        }
    }

    void NetworkController::handleConnectionRestored(const asio::ip::udp::endpoint& endpoint) {
        if (!m_onConnectionRestored) {
            return;
        }

        bool wasDown = false;
        {
            std::lock_guard<std::mutex> lock(m_connectionStateMutex);
            wasDown = m_downEndpoints.erase(makeEndpointKey(endpoint)) > 0;
        }

        if (wasDown) {
            m_onConnectionRestored(endpoint);
        }
    }

    void NetworkController::addEndpointForPing(const asio::ip::udp::endpoint& endpoint) {
        if (m_pingController) {
            m_pingController->addEndpoint(endpoint);
        }

        std::lock_guard<std::mutex> lock(m_connectionStateMutex);
        m_downEndpoints.erase(makeEndpointKey(endpoint));
    }

    void NetworkController::removePingMonitoring(const asio::ip::udp::endpoint& endpoint) {
        if (m_pingController) {
            m_pingController->removeEndpoint(endpoint);
        }

        std::lock_guard<std::mutex> lock(m_connectionStateMutex);
        m_downEndpoints.erase(makeEndpointKey(endpoint));
    }
    }
}

