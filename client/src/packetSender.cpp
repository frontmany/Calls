#include "packetSender.h"

#include <algorithm>
#include <chrono>
#include <thread>

#include "logger.h"

using namespace calls;

PacketSender::PacketSender()
    : m_running(false)
{
}

PacketSender::~PacketSender() {
    stop();
}

void PacketSender::init(PacketQueue& queue, asio::ip::udp::socket& socket, asio::ip::udp::endpoint remoteEndpoint, std::function<void()> onErrorCallback) {
    m_queue = std::ref(queue);
    m_socket = std::ref(socket);
    m_serverEndpoint = remoteEndpoint;
    m_onErrorCallback = std::move(onErrorCallback);
    m_running = false;
}

void PacketSender::start() {
    if (!m_queue.has_value() || !m_socket.has_value()) {
        LOG_ERROR("PacketSender is not properly initialized");
        return;
    }

    if (m_running.exchange(true)) {
        return;
    }

    m_thread = std::thread(&PacketSender::run, this);
}

void PacketSender::stop() {
    if (!m_running.exchange(false)) {
        return;
    }

    if (m_thread.joinable()) {
        m_thread.join();
    }
}

bool PacketSender::isRunning() const {
    return m_running.load();
}

void PacketSender::writeUint16(std::vector<unsigned char>& buffer, uint16_t value)
{
    buffer.push_back(static_cast<unsigned char>((value >> 8) & 0xFF));
    buffer.push_back(static_cast<unsigned char>(value & 0xFF));
}

void PacketSender::writeUint32(std::vector<unsigned char>& buffer, uint32_t value)
{
    buffer.push_back(static_cast<unsigned char>((value >> 24) & 0xFF));
    buffer.push_back(static_cast<unsigned char>((value >> 16) & 0xFF));
    buffer.push_back(static_cast<unsigned char>((value >> 8) & 0xFF));
    buffer.push_back(static_cast<unsigned char>(value & 0xFF));
}

void PacketSender::writeUint64(std::vector<unsigned char>& buffer, uint64_t value)
{
    for (int shift = 56; shift >= 0; shift -= 8) {
        buffer.push_back(static_cast<unsigned char>((value >> shift) & 0xFF));
    }
}

void PacketSender::run() {
    while (m_running.load()) {
        if (!m_queue.has_value()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        auto& queue = m_queue->get();
        Packet packet{};
        if (!queue.tryPop(packet, std::chrono::milliseconds(100))) {
            continue;
        }

        sendPacket(packet);
    }
}

void PacketSender::sendPacket(const Packet& packet) {
    if (!m_socket.has_value()) {
        return;
    }

    auto& socket = m_socket->get();
    auto datagrams = splitPacket(packet);

    for (const auto& datagram : datagrams) {
        std::error_code ec;
        socket.send_to(asio::buffer(datagram), m_serverEndpoint, 0, ec);
        if (ec) {
            LOG_ERROR("Failed to send datagram chunk: {}", ec.message());
            if (m_onErrorCallback) {
                m_onErrorCallback();
            }

            break;
        }
    }
}

std::vector<std::vector<unsigned char>> PacketSender::splitPacket(const Packet& packetData) {
    const bool hasPayload = !packetData.data.empty();
    const std::size_t totalChunks = hasPayload
        ? static_cast<std::size_t>((packetData.data.size() + m_maxPayloadSize - 1) / m_maxPayloadSize)
        : 1U;

    std::vector<std::vector<unsigned char>> packets;
    packets.reserve(totalChunks);

    for (std::size_t chunkIndex = 0; chunkIndex < totalChunks; ++chunkIndex) {
        const std::size_t offset = chunkIndex * m_maxPayloadSize;
        const std::size_t payloadSize = hasPayload
            ? std::min(m_maxPayloadSize, packetData.data.size() - offset)
            : 0U;

        std::vector<unsigned char> datagram;
        datagram.reserve(m_headerSize + payloadSize);

        writeUint64(datagram, packetData.id);
        writeUint16(datagram, static_cast<uint16_t>(chunkIndex));
        writeUint16(datagram, static_cast<uint16_t>(totalChunks));
        writeUint16(datagram, static_cast<uint16_t>(payloadSize));
        writeUint32(datagram, static_cast<uint32_t>(packetData.type));

        if (payloadSize > 0) {
            datagram.insert(datagram.end(),
                packetData.data.begin() + static_cast<std::ptrdiff_t>(offset),
                packetData.data.begin() + static_cast<std::ptrdiff_t>(offset + payloadSize)
            );
        }

        packets.push_back(std::move(datagram));
    }

    return packets;
}