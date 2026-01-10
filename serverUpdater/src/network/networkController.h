#pragma once

#include <functional>
#include <filesystem>
#include <vector>

#include "../utilities/safeQueue.h"
#include "packet.h"

#include "asio.hpp"
#include "json.hpp"

#include <unordered_set>
#include <atomic>
#include <mutex>

namespace serverUpdater
{
class Connection;

typedef std::shared_ptr<Connection> ConnectionPtr;

class NetworkController {
public:
    NetworkController(uint16_t port, std::function<void(ConnectionPtr, Packet&&)> onUpdatesCheck, std::function<void(ConnectionPtr, Packet&&)> onUpdateAccepted);
    ~NetworkController();
    void start();
    void stop();

private:
    void processQueue();
    void waitForClientConnections();
    void handlePacket(OwnedPacket&& packet);
    void createConnection(asio::ip::tcp::socket socket);
    void onDisconnect(ConnectionPtr connection);

private:
    std::atomic<bool> m_running;
    utilities::SafeQueue<OwnedPacket> m_queue;

    std::mutex m_connectionsMutex;
    std::unordered_set<ConnectionPtr> m_setConnections;
    std::thread m_contextThread;

    asio::io_context m_context;
    asio::ip::tcp::acceptor m_asioAcceptor;

    std::function<void(ConnectionPtr, Packet&&)> m_onUpdatesCheck;
    std::function<void(ConnectionPtr, Packet&&)> m_onUpdateAccepted;
};
}