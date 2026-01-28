#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "tcp_packet.h"
#include "tcp_packets_receiver.h"
#include "tcp_packets_sender.h"
#include "utilities/safeQueue.h"
#include "asio.hpp"

namespace core
{
    namespace network
    {
        class TcpControlClient {
        public:
            using OnControlPacket = std::function<void(uint32_t type, const unsigned char* data, size_t size)>;

            TcpControlClient(OnControlPacket onControlPacket, std::function<void()> onConnectionDown);
            ~TcpControlClient();

            void connect(const std::string& host, const std::string& tcpPort);
            void disconnect();
            bool isConnected() const;
            bool send(uint32_t type, const std::vector<unsigned char>& body);
            bool send(uint32_t type, const void* data, size_t size);

        private:
            void runConnect(const std::string& host, uint16_t port);
            void readHandshake();
            void readHandshakeConfirmation();
            void writeHandshake();
            void initAfterHandshake();
            void processPacketQueue();

            asio::io_context m_context;
            asio::ip::tcp::socket m_socket;
            std::unique_ptr<asio::executor_work_guard<asio::io_context::executor_type>> m_workGuard;
            std::thread m_asioThread;
            std::thread m_processThread;

            core::utilities::SafeQueue<TcpPacket> m_inQueue;
            core::utilities::SafeQueue<TcpPacket> m_outQueue;

            std::unique_ptr<TcpPacketsReceiver> m_receiver;
            std::unique_ptr<TcpPacketsSender> m_sender;

            uint64_t m_handshakeIn = 0;
            uint64_t m_handshakeOut = 0;
            uint64_t m_handshakeConfirmation = 0;

            OnControlPacket m_onControlPacket;
            std::function<void()> m_onConnectionDown;

            std::atomic<bool> m_connecting{ false };
            std::atomic<bool> m_connected{ false };
            std::atomic<bool> m_shuttingDown{ false };
        };
    }
}
