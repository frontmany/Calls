#include "packetReceiver.h"

#include <chrono>
#include <system_error>
#include <string>

#include "logger.h"

using namespace calls;

PacketReceiver::PacketReceiver()
    : m_running(false)
{
}

PacketReceiver::~PacketReceiver() {
    stop();
}

bool PacketReceiver::init(asio::ip::udp::socket& socket,
    std::function<void(const unsigned char*, int, PacketType)> onPacketReceived,
    std::function<void()> onErrorCallback)
{
    m_socket = std::ref(socket);
    m_onPacketReceived = std::move(onPacketReceived);
    m_onErrorCallback = std::move(onErrorCallback);
    m_running = false;
    m_remoteEndpoint = asio::ip::udp::endpoint();

    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        m_pendingPackets.clear();
    }

    if (!m_socket.has_value() || !m_socket->get().is_open()) {
        LOG_ERROR("PacketReceiver init called with closed socket");
        return false;
    }

    return true;
}

void PacketReceiver::start()
{
    if (!m_socket.has_value() || !m_socket->get().is_open()) {
        LOG_ERROR("PacketReceiver cannot start without a valid socket");
        return;
    }

    if (m_running.exchange(true)) {
        return;
    }

    doReceive();
}

void PacketReceiver::stop()
{
    if (!m_running.exchange(false)) {
        return;
    }

    if (m_socket.has_value()) {
        auto& socket = m_socket->get();

        if (socket.is_open()) {
        std::error_code ec;
            socket.cancel(ec);
        if (ec && ec != asio::error::operation_aborted) {
            LOG_WARN("Failed to cancel receiver socket: {}", ec.message());
        }
        }
    }

    std::lock_guard<std::mutex> lock(m_stateMutex);
    m_pendingPackets.clear();
}

bool PacketReceiver::isRunning() const
{
    return m_running.load();
}

void PacketReceiver::doReceive()
{
    if (!m_socket.has_value()) {
        return;
    }

    auto& socket = m_socket->get();

    if (!socket.is_open()) {
        return;
    }

    socket.async_receive_from(asio::buffer(m_buffer), m_remoteEndpoint,
        [this](const std::error_code& ec, std::size_t bytesTransferred)
        {
            if (!m_running.load()) {
                return;
            }

            if (ec) {
                if (ec != asio::error::operation_aborted) {
                    notifyError(ec);
                }

                if (m_running.load()) {
                    doReceive();
                }

                return;
            }

            processDatagram(bytesTransferred);
            cleanupExpired();
            doReceive();
        });
}

void PacketReceiver::processDatagram(std::size_t bytesTransferred)
{
    if (bytesTransferred < m_headerSize) {
        LOG_WARN("Received datagram too small: {} bytes", bytesTransferred);
        return;
    }

    const unsigned char* data = m_buffer.data();

    const uint64_t packetId = readUint64(data);
    const uint16_t chunkIndex = readUint16(data + 8);
    const uint16_t totalChunks = readUint16(data + 10);
    const uint16_t payloadLength = readUint16(data + 12);
    const uint32_t packetTypeRaw = readUint32(data + 14);
    const PacketType packetType = static_cast<PacketType>(packetTypeRaw);

    const std::size_t actualPayload = bytesTransferred - m_headerSize;
    if (payloadLength > actualPayload) {
        LOG_WARN("Payload length mismatch: declared {}, available {}", payloadLength, actualPayload);
        return;
    }

    if (payloadLength == 0) {
        if (m_onPacketReceived) {
            m_onPacketReceived(nullptr, 0, packetType);
        }
        return;
    }

    handleChunk(packetId, chunkIndex, totalChunks, packetType, data + m_headerSize, payloadLength);
}

void PacketReceiver::handleChunk(uint64_t packetId, uint16_t chunkIndex,
    uint16_t chunkCount, PacketType packetType,
    const unsigned char* payload, std::size_t payloadSize)
{
    if (payload == nullptr || payloadSize == 0) {
        return;
    }

    std::vector<unsigned char> assembledData;
    bool packetComplete = false;
    PacketType completedType = packetType;

    {
        std::unique_lock<std::mutex> lock(m_stateMutex);
        const auto now = std::chrono::steady_clock::now();

        if (chunkCount == 0) {
            LOG_WARN("Received chunk with zero chunk count for packet {}", packetId);
            return;
        }

        auto& packet = m_pendingPackets[packetId];
        if (packet.chunks.empty() || packet.totalChunks != chunkCount) {
            packet = PendingPacket{};
            packet.totalChunks = chunkCount;
            packet.chunks.resize(chunkCount);
            packet.receivedChunks = 0;
        }

        if (!packet.typeSet) {
            packet.type = packetType;
            packet.typeSet = true;
        }
        else if (packet.type != packetType) {
            LOG_WARN("Packet type mismatch for packet {}: expected {}, got {}",
                packetId,
                static_cast<uint32_t>(packet.type),
                static_cast<uint32_t>(packetType));
            packet = PendingPacket{};
            packet.totalChunks = chunkCount;
            packet.chunks.resize(chunkCount);
            packet.type = packetType;
            packet.typeSet = true;
        }

        if (chunkIndex >= packet.chunks.size()) {
            LOG_WARN("Chunk index {} out of range for packet {} ({})", chunkIndex, packetId, packet.chunks.size());
            return;
        }

        if (packet.chunks[chunkIndex].empty()) {
            packet.chunks[chunkIndex] = std::vector<unsigned char>(payload, payload + payloadSize);
            packet.receivedChunks++;
        }

        packet.lastUpdate = now;

        if (packet.receivedChunks == packet.totalChunks) {
            packetComplete = true;
            completedType = packet.type;

            std::size_t totalSize = 0;
            for (const auto& chunk : packet.chunks) {
                totalSize += chunk.size();
            }

            assembledData.reserve(totalSize);
            for (const auto& chunk : packet.chunks) {
                assembledData.insert(assembledData.end(), chunk.begin(), chunk.end());
            }

            m_pendingPackets.erase(packetId);
        }
    }

    if (!packetComplete) {
        return;
    }

    if (!m_onPacketReceived) {
        return;
    }

    if (assembledData.empty()) {
        m_onPacketReceived(nullptr, 0, completedType);
        return;
    }

    m_onPacketReceived(assembledData.data(), static_cast<int>(assembledData.size()), completedType);
}

void PacketReceiver::cleanupExpired()
{
    std::unique_lock<std::mutex> lock(m_stateMutex);
    const auto now = std::chrono::steady_clock::now();

    for (auto it = m_pendingPackets.begin(); it != m_pendingPackets.end();) {
        if (now - it->second.lastUpdate > m_packetTimeout) {
            it = m_pendingPackets.erase(it);
        }
        else {
            ++it;
        }
    }
}

uint16_t PacketReceiver::readUint16(const unsigned char* data)
{
    return static_cast<uint16_t>((static_cast<uint16_t>(data[0]) << 8) | static_cast<uint16_t>(data[1]));
}

uint32_t PacketReceiver::readUint32(const unsigned char* data)
{
    uint32_t value = 0;
    for (int i = 0; i < 4; ++i) {
        value = static_cast<uint32_t>((value << 8) | data[i]);
    }
    return value;
}

uint64_t PacketReceiver::readUint64(const unsigned char* data)
{
    uint64_t value = 0;
    for (int i = 0; i < 8; ++i) {
        value = (value << 8) | data[i];
    }
    return value;
}

void PacketReceiver::notifyError(const std::error_code& ec)
{
    if (ec == asio::error::operation_aborted) {
        return;
    }

    LOG_ERROR("Packet receiver error: {}", ec.message());
    if (m_onErrorCallback) {
        m_onErrorCallback();
    }
}
