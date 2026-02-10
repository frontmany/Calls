#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_set>

#include "tcp_packet.h"
#include "tcp_connection.h"
#include "utilities/safeQueue.h"
#include "asio.hpp"

namespace server
{
    namespace network
    {
        class TcpControlController {
        public:
            using OnControlPacket = std::function<void(OwnedTcpPacket&&)>;
            using OnDisconnect = std::function<void(TcpConnectionPtr)>;

            TcpControlController(uint16_t port, OnControlPacket onPacket, OnDisconnect onDisconnect);
            ~TcpControlController();

            void start();
            void stop();
            bool isRunning() const;

        private:
            void waitForClients();
            void createConnection(asio::ip::tcp::socket socket);
            void processQueue();
            void handleDisconnect(TcpConnectionPtr conn);

        private:
            std::atomic<bool> m_running{ false };
            uint16_t m_port;
            utilities::SafeQueue<OwnedTcpPacket> m_queue;
            std::mutex m_connMutex;
            std::unordered_set<TcpConnectionPtr> m_connections;

            asio::io_context m_ctx;
            asio::ip::tcp::acceptor m_acceptor;
            std::thread m_ctxThread;

            OnControlPacket m_onPacket;
            OnDisconnect m_onDisconnect;
        };
    }
}
