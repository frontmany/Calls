#include "packetQueue.h"

#include <utility>

PacketQueue::PacketQueue()
    : m_maxSize(5)
{
}

void PacketQueue::push(Packet packet)
{
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_queue.size() >= m_maxSize)
        {
            m_queue.pop_front();
        }

        m_queue.push_back(std::move(packet));
    }

    m_cv.notify_one();
}

bool PacketQueue::tryPop(Packet& packet, std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_cv.wait_for(lock, timeout, [this]() { return !m_queue.empty(); }))
    {
        return false;
    }

    packet = std::move(m_queue.front());
    m_queue.pop_front();
    return true;
}

void PacketQueue::clear()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_queue.clear();
}