#pragma once

#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "network/tcp/packet.h"
#include "network/tcp/packetReceiver.h"
#include "network/tcp/packetSender.h"
#include "utilities/safeQueue.h"
#include "asio.hpp"

namespace core::network::tcp {

class Client {
public:
    Client(asio::io_context& context,
        std::function<void(uint32_t type, const unsigned char* data, size_t size)>&& onPacket,
        std::function<void()>&& onConnectionDown);
    ~Client();

    void connect(const std::string& host, const std::string& port);
    bool connectSync(const std::string& host, const std::string& port, int timeoutMs);
    void disconnect();
    bool isConnected() const;
    bool send(uint32_t type, const std::vector<unsigned char>& body);
    bool send(uint32_t type, const void* data, size_t size);

private:
    void runConnect(const std::string& host, uint16_t port);
    void readHandshake();
    void readHandshakeConfirmation();
    void writeHandshake();
    void initializeAfterHandshake();
    void processPacketQueue();
    void signalConnectResult(bool success);

    asio::io_context& m_context;
    asio::ip::tcp::socket m_socket;
    std::thread m_processThread;

    core::utilities::SafeQueue<Packet> m_inQueue;
    core::utilities::SafeQueue<Packet> m_outQueue;

    std::unique_ptr<PacketsReceiver> m_receiver;
    std::unique_ptr<PacketsSender> m_sender;

    uint64_t m_handshakeIn = 0;
    uint64_t m_handshakeOut = 0;
    uint64_t m_handshakeConfirmation = 0;

    std::shared_ptr<std::promise<bool>> m_connectPromise;

    std::function<void(uint32_t type, const unsigned char* data, size_t size)> m_onPacket;
    std::function<void()> m_onConnectionDown;

    std::atomic<bool> m_connecting{ false };
    std::atomic<bool> m_connected{ false };
    std::atomic<bool> m_shuttingDown{ false };
};

}
