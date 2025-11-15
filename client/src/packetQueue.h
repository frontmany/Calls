#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <vector>

#include "packetTypes.h"

struct Packet
{
    uint64_t id;
    calls::PacketType type = calls::PacketType::PING;
    std::vector<unsigned char> data;
};

class PacketQueue
{
public:
    PacketQueue();

    void push(Packet packet);
    bool tryPop(Packet& packet, std::chrono::milliseconds timeout);
    void clear();

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::deque<Packet> m_queue;
    std::size_t m_maxSize;
};

