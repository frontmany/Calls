#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#include "packetTypes.h"

#include "asio.hpp"

struct OutgoingPacket
{
    uint64_t id = 0;
    PacketType type = PacketType::PING;
    std::vector<unsigned char> data;
    asio::ip::udp::endpoint endpoint;
    bool rawDatagram = false;
};

class OutgoingPacketQueue
{
public:
    void push(OutgoingPacket packet);
    bool tryPop(OutgoingPacket& packet, std::chrono::milliseconds timeout);
    void clear();

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::deque<OutgoingPacket> m_queue;
};

class ServerPacketSender
{
public:
    ServerPacketSender();
    ~ServerPacketSender();

    void init(OutgoingPacketQueue& queue,
        asio::ip::udp::socket& socket,
        std::size_t maxPacketSize,
        std::size_t headerSize,
        std::function<void()> onErrorCallback);

    void start();
    void stop();
    bool isRunning() const;

private:
    void run();
    void sendPacket(const OutgoingPacket& packet);
    void sendRawDatagram(const OutgoingPacket& packet);
    void sendStructuredPacket(const OutgoingPacket& packet);
    void writeUint16(std::vector<unsigned char>& buffer, uint16_t value);
    void writeUint32(std::vector<unsigned char>& buffer, uint32_t value);
    void writeUint64(std::vector<unsigned char>& buffer, uint64_t value);

private:
    std::optional<std::reference_wrapper<OutgoingPacketQueue>> m_queue;
    std::optional<std::reference_wrapper<asio::ip::udp::socket>> m_socket;
    std::size_t m_maxPacketSize = 0;
    std::size_t m_headerSize = 0;
    std::function<void()> m_onErrorCallback;

    std::atomic<bool> m_running;
    std::thread m_thread;
};


