#pragma once

#include <functional>
#include <filesystem>
#include <vector>

#include "safeDeque.h"
#include "packet.h"

#include "asio.hpp"
#include "json.hpp"

#include <unordered_set>

class Connection;

typedef std::shared_ptr<Connection> ConnectionPtr;

class NetworkController {
public:
    NetworkController(uint16_t port, std::function<void(ConnectionPtr, Packet&&)> onUpdatesCheck, std::function<void(ConnectionPtr, Packet&&)> onUpdateAccepted);
    ~NetworkController();
    void sendUpdate(ConnectionPtr connection, const Packet& packet, const std::vector<std::filesystem::path>& paths);
    void sendUpdateRequiredPacket(ConnectionPtr connection, const Packet& packet);
    void sendUpdatePossiblePacket(ConnectionPtr connection, const Packet& packet);
    void start();
    void stop();

private:
    void waitForClientConnections();
    void processQueue();
    void handlePacket(OwnedPacket&& packet);
    void createConnection(asio::ip::tcp::socket socket);
    void onDisconnect(ConnectionPtr connection);

private:
    bool m_running;
    SafeDeque<OwnedPacket> m_queue;

    std::unordered_set<ConnectionPtr> m_setConnections;
    std::thread m_contextThread;

    asio::io_context m_context;
    asio::ip::tcp::acceptor m_asioAcceptor;

    std::function<void(ConnectionPtr, Packet&&)> m_onUpdatesCheck;
    std::function<void(ConnectionPtr, Packet&&)> m_onUpdateAccepted;
};
