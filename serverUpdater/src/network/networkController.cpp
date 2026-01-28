#include "networkController.h"

#include <chrono>

#include "packetType.h"
#include "connection.h"
#include "utilities/safeQueue.h"
#include "utilities/logger.h"
#include "utilities/errorCodeForLog.h"

using namespace std::chrono_literals;
using namespace serverUpdater;

namespace serverUpdater
{
NetworkController::NetworkController(uint16_t port,
    std::function<void(ConnectionPtr, Packet&&)> onUpdatesCheck,
    std::function<void(ConnectionPtr, Packet&&)> onUpdateAccepted)
    : m_running(false),
    m_asioAcceptor(m_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
    m_onUpdatesCheck(onUpdatesCheck),
    m_onUpdateAccepted(onUpdateAccepted)
{
}

NetworkController::~NetworkController() {
    stop();
}

void NetworkController::start() {
    LOG_INFO("[SERVER] Starting network controller");
    m_contextThread = std::thread([this]() {m_context.run(); });
    waitForClientConnections();

    m_running = true;
    processQueue();
}

void NetworkController::stop() {
    m_running = false;
    m_context.stop();

    if (m_contextThread.joinable())
        m_contextThread.join();

    LOG_INFO("[SERVER] Server stopped");
}

void NetworkController::waitForClientConnections() {
    m_asioAcceptor.async_accept([this](std::error_code ec, asio::ip::tcp::socket socket) {
        if (!ec) {
            auto endpoint = socket.remote_endpoint();
            LOG_INFO("[SERVER] New connection from {}:{}", endpoint.address().to_string(), endpoint.port());
            createConnection(std::move(socket));
        }
        else {
            LOG_ERROR("[SERVER] Connection error: {}", utilities::errorCodeForLog(ec));
        }

        waitForClientConnections();
    });
}

void NetworkController::processQueue() {
    while (m_running) {
        auto packet = m_queue.pop_for(100ms);
        if (packet.has_value()) {
            handlePacket(std::move(*packet));
        }
    }
}

void NetworkController::onDisconnect(ConnectionPtr connection) {
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    m_setConnections.erase(connection);
    LOG_INFO("[SERVER] Client disconnected (active connections: {})", m_setConnections.size());
}

void NetworkController::handlePacket(OwnedPacket&& packet) {
    if (packet.packet.type() == static_cast<int>(PacketType::UPDATE_CHECK)) {
        LOG_DEBUG("[SERVER] Handling CHECK_UPDATES request");
        m_onUpdatesCheck(packet.connection, std::move(packet.packet));
    }
    else if (packet.packet.type() == static_cast<int>(PacketType::UPDATE_ACCEPT)) {
        LOG_DEBUG("[SERVER] Handling UPDATE_ACCEPT request");
        m_onUpdateAccepted(packet.connection, std::move(packet.packet));
    }
    else {
        LOG_WARN("[SERVER] Unknown request type: {}", packet.packet.type());
    }
}

void NetworkController::createConnection(asio::ip::tcp::socket socket)
{
    ConnectionPtr connection = std::make_shared<Connection>(
        m_context,
        std::move(socket),
        [this](OwnedPacket&& packet) {m_queue.push(std::move(packet));  },
        [this](ConnectionPtr connection) { onDisconnect(connection); }
    );

    {
        std::lock_guard<std::mutex> lock(m_connectionsMutex);
        m_setConnections.insert(connection);
        LOG_INFO("[SERVER] New client connected (active connections: {})", m_setConnections.size());
    }
}
}