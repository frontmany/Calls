#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_set>

#include "network/tcp/packet.h"
#include "network/tcp/connection.h"
#include "utilities/safeQueue.h"
#include "asio.hpp"

namespace server::network::tcp
{
    class Server {
    public:
        using OnPacket = std::function<void(OwnedPacket&&)>;
        using OnDisconnect = std::function<void(ConnectionPtr)>;

        Server(uint16_t port, OnPacket onPacket, OnDisconnect onDisconnect);
        ~Server();

        void start();
        void stop();
        bool isRunning() const;

    private:
        void waitForClients();
        void createConnection(asio::ip::tcp::socket socket);
        void processQueue();
        void handleDisconnect(ConnectionPtr conn);

    private:
        std::atomic<bool> m_running{ false };
        uint16_t m_port;
        utilities::SafeQueue<OwnedPacket> m_queue;
        std::mutex m_connMutex;
        std::unordered_set<ConnectionPtr> m_connections;

        asio::io_context m_ctx;
        asio::ip::tcp::acceptor m_acceptor;
        std::thread m_ctxThread;

        OnPacket m_onPacket;
        OnDisconnect m_onDisconnect;
    };
}
