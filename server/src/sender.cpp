#include "sender.h"
#include "logger.h"

#include <algorithm>
#include <limits>
#include <thread>

void OutgoingPacketQueue::push(OutgoingPacket packet)
{
    {
        std::unique_lock<std::mutex> lock(m_mutex);

        m_queue.push_back(std::move(packet));
    }

    m_cv.notify_one();
}

bool OutgoingPacketQueue::tryPop(OutgoingPacket& packet, std::chrono::milliseconds timeout)
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

void OutgoingPacketQueue::clear()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_queue.clear();
}

ServerPacketSender::ServerPacketSender()
    : m_running(false)
{
}

ServerPacketSender::~ServerPacketSender()
{
    stop();
}

void ServerPacketSender::init(OutgoingPacketQueue& queue,
    asio::ip::udp::socket& socket,
    std::size_t maxPacketSize,
    std::size_t headerSize,
    std::function<void()> onErrorCallback)
{
    m_queue = std::ref(queue);
    m_socket = std::ref(socket);
    m_maxPacketSize = maxPacketSize;
    m_headerSize = headerSize;
    m_onErrorCallback = std::move(onErrorCallback);
    m_running = false;
}

void ServerPacketSender::start()
{
    if (!m_queue.has_value() || !m_socket.has_value() || m_maxPacketSize == 0 || m_headerSize == 0)
    {
        LOG_ERROR("ServerPacketSender is not properly initialized");
        return;
    }

    if (m_running.exchange(true))
    {
        return;
    }

    m_thread = std::thread(&ServerPacketSender::run, this);
}

void ServerPacketSender::stop()
{
    if (!m_running.exchange(false))
    {
        return;
    }

    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

bool ServerPacketSender::isRunning() const
{
    return m_running.load();
}

void ServerPacketSender::run()
{
    while (m_running.load())
    {
        if (!m_queue.has_value())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        OutgoingPacket packet{};

        if (!m_queue->get().tryPop(packet, std::chrono::milliseconds(100)))
        {
            continue;
        }

        sendPacket(packet);
    }
}

void ServerPacketSender::sendPacket(const OutgoingPacket& packet)
{
    if (!m_socket.has_value())
    {
        return;
    }

    if (packet.rawDatagram)
    {
        sendRawDatagram(packet);
        return;
    }

    sendStructuredPacket(packet);
}

void ServerPacketSender::sendRawDatagram(const OutgoingPacket& packet)
{
    if (packet.data.empty())
    {
        return;
    }

    auto& socket = m_socket->get();

    if (!socket.is_open())
    {
        return;
    }

    std::error_code ec;
    socket.send_to(asio::buffer(packet.data), packet.endpoint, 0, ec);

    if (ec && ec != asio::error::operation_aborted)
    {
        if (ec == asio::error::connection_refused || 
            ec == asio::error::host_unreachable || 
            ec == asio::error::network_unreachable ||
            ec == asio::error::no_buffer_space)
        {
            LOG_DEBUG("Failed to forward datagram to {}:{} (client may have disconnected): {}", 
                packet.endpoint.address().to_string(), packet.endpoint.port(), ec.message());
        }
        else
        {
            LOG_ERROR("Failed to forward datagram: {}", ec.message());

            if (m_onErrorCallback)
            {
                m_onErrorCallback();
            }
        }
    }
}

void ServerPacketSender::sendStructuredPacket(const OutgoingPacket& packet)
{
    auto& socket = m_socket->get();

    if (!socket.is_open())
    {
        return;
    }

    if (m_maxPacketSize <= m_headerSize)
    {
        LOG_ERROR("Invalid packet size limits (maxPacketSize={}, headerSize={})", m_maxPacketSize, m_headerSize);
        return;
    }

    const std::size_t maxPayloadSize = m_maxPacketSize - m_headerSize;

    if (maxPayloadSize == 0)
    {
        LOG_ERROR("Max payload size is zero");
        return;
    }

    const bool hasPayload = !packet.data.empty();
    const std::size_t chunksRequired = hasPayload
        ? ((packet.data.size() + maxPayloadSize - 1) / maxPayloadSize)
        : 1U;

    if (chunksRequired > std::numeric_limits<std::uint16_t>::max())
    {
        LOG_ERROR("Packet requires too many chunks: {}", chunksRequired);
        return;
    }

    const uint16_t totalChunks = static_cast<uint16_t>(chunksRequired);

    for (uint16_t chunkNumber = 0; chunkNumber < totalChunks; ++chunkNumber)
    {
        const std::size_t offset = static_cast<std::size_t>(chunkNumber) * maxPayloadSize;
        const std::size_t remaining = (offset < packet.data.size())
            ? (packet.data.size() - offset)
            : 0U;
        const std::size_t chunkSize = hasPayload
            ? std::min(maxPayloadSize, remaining)
            : 0U;

        std::vector<unsigned char> datagram;
        datagram.reserve(m_headerSize + chunkSize);

        writeUint64(datagram, packet.id);
        writeUint16(datagram, chunkNumber);
        writeUint16(datagram, totalChunks);
        writeUint16(datagram, static_cast<uint16_t>(chunkSize));
        writeUint32(datagram, static_cast<uint32_t>(packet.type));

        if (chunkSize > 0)
        {
            datagram.insert(datagram.end(),
                packet.data.begin() + static_cast<std::ptrdiff_t>(offset),
                packet.data.begin() + static_cast<std::ptrdiff_t>(offset + chunkSize));
        }

        std::error_code ec;
        socket.send_to(asio::buffer(datagram), packet.endpoint, 0, ec);

        if (ec && ec != asio::error::operation_aborted)
        {
            if (ec == asio::error::connection_refused || 
                ec == asio::error::host_unreachable || 
                ec == asio::error::network_unreachable ||
                ec == asio::error::no_buffer_space)
            {
                LOG_DEBUG("Failed to send packet chunk to {}:{} (client may have disconnected): {}", 
                    packet.endpoint.address().to_string(), packet.endpoint.port(), ec.message());
                break;
            }
            else
            {
                LOG_ERROR("Failed to send packet chunk: {}", ec.message());

                if (m_onErrorCallback)
                {
                    m_onErrorCallback();
                }

                break;
            }
        }
    }
}

void ServerPacketSender::writeUint16(std::vector<unsigned char>& buffer, uint16_t value)
{
    buffer.push_back(static_cast<unsigned char>((value >> 8) & 0xFF));
    buffer.push_back(static_cast<unsigned char>(value & 0xFF));
}

void ServerPacketSender::writeUint32(std::vector<unsigned char>& buffer, uint32_t value)
{
    buffer.push_back(static_cast<unsigned char>((value >> 24) & 0xFF));
    buffer.push_back(static_cast<unsigned char>((value >> 16) & 0xFF));
    buffer.push_back(static_cast<unsigned char>((value >> 8) & 0xFF));
    buffer.push_back(static_cast<unsigned char>(value & 0xFF));
}

void ServerPacketSender::writeUint64(std::vector<unsigned char>& buffer, uint64_t value)
{
    for (int shift = 56; shift >= 0; shift -= 8)
    {
        buffer.push_back(static_cast<unsigned char>((value >> shift) & 0xFF));
    }
}


