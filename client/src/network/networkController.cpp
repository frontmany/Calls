#include "networkController.h"

#include <thread>
#include <utility>

#include "logger.h"
#include "packet.h"

using namespace calls;

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
    std::function<void(const unsigned char*, int, PacketType type)> onReceiveCallback,
    std::function<void()> onErrorCallback)
{
    m_onReceiveCallback = std::move(onReceiveCallback);
    m_onErrorCallback = std::move(onErrorCallback);

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
            if (m_onErrorCallback) {
                m_onErrorCallback();
            }
        };

        if (!m_packetReceiver.init(m_socket, m_onReceiveCallback, errorHandler)) {
            LOG_ERROR("Failed to initialize packet receiver");
            return false;
        }

        m_packetSender.init(m_socket, m_serverEndpoint, errorHandler);

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

    m_asioThread = std::thread([this]() {
        m_context.run();
    });

    m_packetReceiver.start();
}

void NetworkController::stop() {
    if (!m_running.exchange(false) && !m_asioThread.joinable()) {
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

void NetworkController::send(const std::vector<unsigned char>& data, PacketType type) {
    Packet packet;
    packet.id = generateId();
    packet.type = static_cast<uint32_t>(type);
    packet.data = data;
    m_packetSender.send(packet);
}

void NetworkController::send(std::vector<unsigned char>&& data, PacketType type) {
    Packet packet;
    packet.id = generateId();
    packet.type = static_cast<uint32_t>(type);
    packet.data = std::move(data);
    m_packetSender.send(packet);
}

void NetworkController::send(PacketType type) {
    send({}, type);
}

uint64_t NetworkController::generateId() {
    return m_nextPacketId.fetch_add(1U, std::memory_order_relaxed);
}