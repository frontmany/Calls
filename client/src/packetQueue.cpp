#include "packetQueue.h"

#include <utility>

#include "logger.h"

using namespace calls;

PacketQueue::PacketQueue()
    : m_maxRegularSize(20)
{
}

bool PacketQueue::isCriticalPacket(const Packet& packet) const
{
    return packet.type == calls::PacketType::PING || 
           packet.type == calls::PacketType::PING_SUCCESS;
}

void PacketQueue::push(Packet packet)
{
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        
        if (isCriticalPacket(packet))
        {
            m_priorityQueue.push_back(std::move(packet));
            LOG_DEBUG("Critical packet (type: {}) added to priority queue", static_cast<int>(packet.type));
        }
        else
        {
            if (m_regularQueue.size() >= m_maxRegularSize)
            {
                auto droppedPacket = m_regularQueue.front();
                m_regularQueue.pop_front();
                LOG_WARN("Packet queue full, dropped packet type: {}", static_cast<int>(droppedPacket.type));
            }

            m_regularQueue.push_back(std::move(packet));
        }
    }

    m_cv.notify_one();
}

bool PacketQueue::tryPop(Packet& packet, std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_cv.wait_for(lock, timeout, [this]() { 
        return !m_priorityQueue.empty() || !m_regularQueue.empty(); 
    }))
    {
        return false;
    }

    if (!m_priorityQueue.empty())
    {
        packet = std::move(m_priorityQueue.front());
        m_priorityQueue.pop_front();
    }
    else
    {
        packet = std::move(m_regularQueue.front());
        m_regularQueue.pop_front();
    }
    
    return true;
}

void PacketQueue::clear()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_priorityQueue.clear();
    m_regularQueue.clear();
}