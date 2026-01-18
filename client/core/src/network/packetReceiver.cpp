#include "packetReceiver.h"

#include <chrono>
#include <system_error>
#include <string>

#include "utilities/logger.h"

namespace core
{
    namespace network 
    {
    PacketReceiver::PacketReceiver()
        : m_running(false) {
    }

    PacketReceiver::~PacketReceiver() {
        stop();
    }

    bool PacketReceiver::init(asio::ip::udp::socket& socket,
        std::function<void(const unsigned char*, int, uint32_t)> onPacketReceived,
        std::function<void()> onErrorCallback,
        std::function<void(uint32_t)> onPingReceived)
    {
        m_socket = std::ref(socket);
        m_onPacketReceived = std::move(onPacketReceived);
        m_onErrorCallback = std::move(onErrorCallback);
        m_onPingReceived = std::move(onPingReceived);
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

        m_processingThread = std::thread([this]() {
            processReceivedPackets();
            });

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

        {
            std::lock_guard<std::mutex> lock(m_stateMutex);
            m_pendingPackets.clear();
        }

        m_receivedPacketsQueue.clear();

        if (m_processingThread.joinable()) {
            m_processingThread.join();
        }
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
        const uint32_t packetType = readUint32(data + 14);

        const std::size_t actualPayload = bytesTransferred - m_headerSize;
        if (payloadLength > actualPayload) {
            LOG_WARN("Payload length mismatch: declared {}, available {}", payloadLength, actualPayload);
            return;
        }

        if (packetType == 0 || packetType == 1) {
            if (m_onPingReceived) {
                m_onPingReceived(packetType);
            }
            return;
        }

        if (payloadLength == 0) {
            ReceivedPacket receivedPacket;
            receivedPacket.data.clear();
            receivedPacket.type = packetType;
            m_receivedPacketsQueue.push(std::move(receivedPacket));
            return;
        }

        const unsigned char* payload = data + m_headerSize;
        const std::size_t payloadSize = payloadLength;

        if (payload == nullptr || payloadSize == 0) {
            return;
        }

        std::vector<unsigned char> assembledData;
        bool packetComplete = false;
        uint32_t completedType = 0;

        {
            std::unique_lock<std::mutex> lock(m_stateMutex);

            if (totalChunks == 0) {
                LOG_WARN("Received chunk with zero chunk count for packet {}", packetId);
                return;
            }

            const auto now = std::chrono::steady_clock::now();
            pruneExpiredPackets(m_pendingPackets, now);

            auto packetIt = m_pendingPackets.find(packetId);
            if (packetIt == m_pendingPackets.end()) {
                if (m_pendingPackets.size() >= m_maxPendingPackets) {
                    evictOldestPacket(m_pendingPackets);
                }
                packetIt = m_pendingPackets.emplace(packetId, PendingPacket{}).first;
                initPendingPacket(packetIt->second, packetId, totalChunks, packetType, now);
            }
            else if (packetIt->second.totalChunks != totalChunks) {
                LOG_WARN("Total chunks mismatch for packet {}: expected {}, got {}",
                    packetId,
                    packetIt->second.totalChunks,
                    totalChunks);
                initPendingPacket(packetIt->second, packetId, totalChunks, packetType, now);
            }
            else if (packetIt->second.type != packetType) {
                LOG_WARN("Packet type mismatch for packet {}: expected {}, got {}",
                    packetId,
                    static_cast<uint32_t>(packetIt->second.type),
                    static_cast<uint32_t>(packetType));
                initPendingPacket(packetIt->second, packetId, totalChunks, packetType, now);
            }

            auto& packet = packetIt->second;
            packet.lastUpdated = now;

            if (chunkIndex >= packet.chunks.size()) {
                LOG_WARN("Chunk index {} out of range for packet {} ({})", chunkIndex, packetId, packet.chunks.size());
                return;
            }

            if (packet.chunks[chunkIndex].empty()) {
                packet.chunks[chunkIndex] = std::vector<unsigned char>(payload, payload + payloadSize);
                packet.receivedChunks++;
            }

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

                m_pendingPackets.erase(packetIt);
            }
        }

        if (!packetComplete) {
            return;
        }

        ReceivedPacket receivedPacket;
        receivedPacket.data = std::move(assembledData);
        receivedPacket.type = completedType;
        m_receivedPacketsQueue.push(std::move(receivedPacket));
    }

    void PacketReceiver::initPendingPacket(PendingPacket& packet, uint64_t packetId, uint16_t totalChunks, uint32_t packetType,
        std::chrono::steady_clock::time_point now)
    {
        packet = PendingPacket{};
        packet.packetId = packetId;
        packet.totalChunks = totalChunks;
        packet.chunks.resize(totalChunks);
        packet.receivedChunks = 0;
        packet.type = packetType;
        packet.lastUpdated = now;
    }

    void PacketReceiver::pruneExpiredPackets(PendingPacketMap& packets, std::chrono::steady_clock::time_point now)
    {
        for (auto it = packets.begin(); it != packets.end(); ) {
            if (now - it->second.lastUpdated > m_pendingPacketTimeout) {
                it = packets.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    void PacketReceiver::evictOldestPacket(PendingPacketMap& packets)
    {
        if (packets.empty()) {
            return;
        }

        auto oldestIt = packets.begin();
        for (auto it = packets.begin(); it != packets.end(); ++it) {
            if (it->second.lastUpdated < oldestIt->second.lastUpdated) {
                oldestIt = it;
            }
        }

        if (oldestIt != packets.end()) {
            packets.erase(oldestIt);
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

    void PacketReceiver::processReceivedPackets()
    {
        const auto timeout = std::chrono::milliseconds(100);

        while (m_running.load()) {
            auto packetOpt = m_receivedPacketsQueue.pop_for(timeout);
            if (!packetOpt.has_value()) {
                continue;
            }

            if (!m_onPacketReceived) {
                continue;
            }

            const auto& packet = packetOpt.value();
            if (packet.data.empty()) {
                m_onPacketReceived(nullptr, 0, packet.type);
            }
            else {
                m_onPacketReceived(packet.data.data(), static_cast<int>(packet.data.size()), packet.type);
            }
        }
    }

    void PacketReceiver::setConnectionDown(bool isDown)
    {
        m_connectionDown = isDown;
    }

    void PacketReceiver::notifyError(const std::error_code& ec)
    {
        if (ec == asio::error::operation_aborted) {
            return;
        }

        if (!m_connectionDown.load()) {
            LOG_ERROR("Packet receiver error: {}", ec.message());
        }

        if (m_onErrorCallback) {
            m_onErrorCallback();
        }
    }
    }
}