#include "networkController.h"
#include "logger.h"
#include <thread>
#include <cstring>
#include <algorithm>
#include <atomic>
#include <limits>
#include <vector>

namespace
{
    uint16_t readUint16(const unsigned char* data)
    {
        return static_cast<uint16_t>(
            (static_cast<uint16_t>(data[0]) << 8) |
            static_cast<uint16_t>(data[1]));
    }

    uint32_t readUint32(const unsigned char* data)
    {
        return (static_cast<uint32_t>(data[0]) << 24) |
            (static_cast<uint32_t>(data[1]) << 16) |
            (static_cast<uint32_t>(data[2]) << 8) |
            static_cast<uint32_t>(data[3]);
    }

    uint64_t readUint64(const unsigned char* data)
    {
        uint64_t value = 0;

        for (int index = 0; index < 8; ++index)
        {
            value = (value << 8) | static_cast<uint64_t>(data[index]);
        }

        return value;
    }

}

std::string NetworkController::makeEndpointKey(const asio::ip::udp::endpoint& endpoint)
{
    return endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
}


NetworkController::NetworkController(const std::string& port,
    std::function<void(const unsigned char*, int, PacketType, const asio::ip::udp::endpoint&)> onReceiveCallback,
    std::function<void()> onNetworkErrorCallback)
    : m_socket(m_context),
    m_workGuard(asio::make_work_guard(m_context)),
    m_onReceiveCallback(onReceiveCallback),
    m_onNetworkErrorCallback(onNetworkErrorCallback),
    m_forwardWithoutAssembly{
        PacketType::VOICE,
        PacketType::SCREEN,
        PacketType::CAMERA
    }
{
    asio::ip::udp::resolver resolver(m_context);
    asio::ip::udp::resolver::results_type endpoints = resolver.resolve(asio::ip::udp::v4(), "0.0.0.0", port);

    if (endpoints.empty()) {
        throw std::runtime_error("No endpoints found for port " + port);
    }

    m_serverEndpoint = *endpoints.begin();

    m_packetSender.init(m_outgoingQueue,
        m_socket,
        m_maxPacketSize,
        HEADER_SIZE,
        [this]() {
            if (m_onNetworkErrorCallback) {
                m_onNetworkErrorCallback();
            }
        });
}

NetworkController::~NetworkController() {
    stop();
}

bool NetworkController::start() {
    try {
        m_socket.open(asio::ip::udp::v4());
        m_socket.bind(m_serverEndpoint);

        m_asioThread = std::thread([this]() { m_context.run(); });
        m_isRunning = true;
        m_packetSender.start();
        startReceive();

        LOG_INFO("UDP Server started on {}:{}", m_serverEndpoint.address().to_string(), m_serverEndpoint.port());
        return true;
    }
    catch (const std::exception& e) {
        LOG_ERROR("Server start error: {}", e.what());
        m_isRunning = false;
        return false;
    }
}

void NetworkController::stop() {
    m_isRunning = false;
    m_pendingPackets.clear();
    m_packetSender.stop();
    m_outgoingQueue.clear();

    if (m_socket.is_open()) {
        asio::error_code ec;
        m_socket.close(ec);
    }

    m_context.stop();

    if (m_asioThread.joinable()) {
        m_asioThread.join();
    }
}

bool NetworkController::isRunning() const {
    return m_isRunning;
}

void NetworkController::sendToClient(const asio::ip::udp::endpoint& clientEndpoint, PacketType type) {
    sendDataToClient(clientEndpoint, nullptr, 0, type);
}

void NetworkController::sendToClient(const asio::ip::udp::endpoint& clientEndpoint,
    const std::string& data, PacketType type) {
    if (data.empty()) {
        sendToClient(clientEndpoint, type);
        return;
    }

    sendDataToClient(clientEndpoint,
        reinterpret_cast<const unsigned char*>(data.data()),
        data.size(),
        type);
}

void NetworkController::forwardDatagramToClient(const asio::ip::udp::endpoint& clientEndpoint,
    const unsigned char* data, std::size_t length) {
    if (!m_isRunning || !m_socket.is_open()) {
        return;
    }

    if (data == nullptr || length == 0) {
        return;
    }

    OutgoingPacket packet;
    packet.endpoint = clientEndpoint;
    packet.rawDatagram = true;
    packet.data.assign(data, data + length);

    m_outgoingQueue.push(std::move(packet));
}

void NetworkController::sendVoiceToClient(const asio::ip::udp::endpoint& clientEndpoint, const unsigned char* data, std::size_t length) {
    forwardDatagramToClient(clientEndpoint, data, length);
}

void NetworkController::sendScreenToClient(const asio::ip::udp::endpoint& clientEndpoint, const unsigned char* data, std::size_t length) {
    forwardDatagramToClient(clientEndpoint, data, length);
}

void NetworkController::sendCameraToClient(const asio::ip::udp::endpoint& clientEndpoint, const unsigned char* data, std::size_t length) {
    forwardDatagramToClient(clientEndpoint, data, length);
}

void NetworkController::sendDataToClient(const asio::ip::udp::endpoint& clientEndpoint,
    const unsigned char* data, std::size_t length, PacketType type) {
    if (!m_isRunning || !m_socket.is_open()) {
        return;
    }

    if (length > 0 && data == nullptr) {
        LOG_ERROR("Cannot send packet with null data and non-zero length");
        return;
    }

    OutgoingPacket packet;
    packet.id = generateId();
    packet.type = type;
    packet.endpoint = clientEndpoint;
    packet.rawDatagram = false;

    if (length > 0 && data != nullptr) {
        packet.data.assign(data, data + length);
    }

    m_outgoingQueue.push(std::move(packet));
}

void NetworkController::startReceive() {
    m_socket.async_receive_from(asio::buffer(m_receiveBuffer), m_receivedFromEndpoint,
        [this](const asio::error_code& ec, std::size_t bytesTransferred) {
            handleReceive(ec, bytesTransferred);
        }
    );
}

void NetworkController::handleReceive(const asio::error_code& error, std::size_t bytesTransferred) {
    if (error) {
        if (error == asio::error::operation_aborted) {
            return;
        }

        if (error == asio::error::connection_refused || 
            error == asio::error::host_unreachable || 
            error == asio::error::network_unreachable ||
            error == asio::error::message_size ||
            error == asio::error::timed_out)
        {
            LOG_DEBUG("UDP receive error (normal for UDP): {}", error.message());
            if (m_isRunning) {
                startReceive();
            }
            return;
        }

        LOG_WARN("Critical UDP receive error: {}", error.message());
        if (m_onNetworkErrorCallback) {
            m_onNetworkErrorCallback();
        }
        return;
    }

    if (bytesTransferred < HEADER_SIZE) {
        LOG_WARN("Received packet too small: {} bytes (minimum: {})", bytesTransferred, HEADER_SIZE);
        if (m_isRunning) {
            startReceive();
        }
        return;
    }

    const unsigned char* rawData = m_receiveBuffer.data();
    const uint64_t frameId = readUint64(rawData);
    const uint16_t chunkIndex = readUint16(rawData + 8);
    const uint16_t chunksCount = readUint16(rawData + 10);
    const uint16_t payloadLengthField = readUint16(rawData + 12);
    const uint32_t typeRaw = readUint32(rawData + 14);
    const PacketType receivedType = static_cast<PacketType>(typeRaw);
    const std::size_t availablePayload = bytesTransferred - HEADER_SIZE;

    if (payloadLengthField > availablePayload) {
        LOG_WARN("Declared payload size {} exceeds available {}",
            payloadLengthField,
            availablePayload);
        if (m_isRunning) {
            startReceive();
        }
        return;
    }

    if (m_forwardWithoutAssembly.find(receivedType) != m_forwardWithoutAssembly.end()) {
        if (m_onReceiveCallback) {
            m_onReceiveCallback(
                rawData,
                static_cast<int>(bytesTransferred),
                receivedType,
                m_receivedFromEndpoint);
        }

        if (m_isRunning) {
            startReceive();
        }
        return;
    }

    if (chunksCount == 0) {
        LOG_WARN("Received frame {} with zero chunk count", frameId);
        if (m_isRunning) {
            startReceive();
        }
        return;
    }

    if (chunkIndex >= chunksCount) {
        LOG_WARN("Chunk index {} is out of range for frame {} (total {})",
            chunkIndex,
            frameId,
            chunksCount);
        const std::string endpointKeyInvalid = makeEndpointKey(m_receivedFromEndpoint);
        auto endpointIt = m_pendingPackets.find(endpointKeyInvalid);
        if (endpointIt != m_pendingPackets.end()) {
            endpointIt->second.erase(frameId);
            if (endpointIt->second.empty()) {
                m_pendingPackets.erase(endpointIt);
            }
        }

        if (m_isRunning) {
            startReceive();
        }
        return;
    }

    if (m_maxPacketSize <= HEADER_SIZE) {
        LOG_ERROR("Configured max packet size {} is not enough for header {}", m_maxPacketSize, HEADER_SIZE);
        if (m_isRunning) {
            startReceive();
        }
        return;
    }

    const std::size_t maxPayloadSize = m_maxPacketSize - HEADER_SIZE;
    if (payloadLengthField > maxPayloadSize) {
        LOG_WARN("Payload {} exceeds configured maximum {}", payloadLengthField, maxPayloadSize);
        if (m_isRunning) {
            startReceive();
        }
        return;
    }

    const unsigned char* payloadStart = rawData + HEADER_SIZE;
    const std::size_t payloadLength = static_cast<std::size_t>(payloadLengthField);
    std::vector<unsigned char> chunk(payloadLength);

    if (payloadLength > 0) {
        std::memcpy(chunk.data(), payloadStart, payloadLength);
    }

    const std::string endpointKey = makeEndpointKey(m_receivedFromEndpoint);
    auto& endpointPackets = m_pendingPackets[endpointKey];
    auto& pendingPacket = endpointPackets[frameId];

    if (pendingPacket.chunks.empty()) {
        pendingPacket.totalChunks = chunksCount;
        pendingPacket.chunks.resize(chunksCount);
    }

    if (pendingPacket.totalChunks != chunksCount) {
        pendingPacket = PendingPacket{};
        pendingPacket.totalChunks = chunksCount;
        pendingPacket.chunks.resize(chunksCount);
    }

    if (!pendingPacket.typeSet) {
        pendingPacket.type = receivedType;
        pendingPacket.typeSet = true;
    }
    else if (pendingPacket.type != receivedType) {
        LOG_WARN("Packet type mismatch for frame {}: expected {}, got {}",
            frameId,
            static_cast<uint32_t>(pendingPacket.type),
            typeRaw);
        pendingPacket = PendingPacket{};
        pendingPacket.type = receivedType;
        pendingPacket.typeSet = true;
        pendingPacket.totalChunks = chunksCount;
        pendingPacket.chunks.resize(chunksCount);
    }

    if (chunkIndex >= pendingPacket.chunks.size()) {
        LOG_WARN("Chunk index {} is out of range for frame {} (allocated {})",
            chunkIndex,
            frameId,
            pendingPacket.chunks.size());
        endpointPackets.erase(frameId);
        if (endpointPackets.empty()) {
            m_pendingPackets.erase(endpointKey);
        }

        if (m_isRunning) {
            startReceive();
        }
        return;
    }

    auto& chunkData = pendingPacket.chunks[chunkIndex];

    if (!chunkData.received) {
        chunkData.received = true;
        chunkData.payload = std::move(chunk);
        pendingPacket.totalSize += payloadLength;
        pendingPacket.receivedChunks++;
    }

    if (pendingPacket.receivedChunks == pendingPacket.totalChunks) {
        std::vector<unsigned char> aggregated;
        aggregated.reserve(pendingPacket.totalSize);

        for (const auto& part : pendingPacket.chunks) {
            aggregated.insert(aggregated.end(), part.payload.begin(), part.payload.end());
        }

        endpointPackets.erase(frameId);
        if (endpointPackets.empty()) {
            m_pendingPackets.erase(endpointKey);
        }

        if (m_onReceiveCallback) {
            m_onReceiveCallback(
                aggregated.empty() ? nullptr : aggregated.data(),
                static_cast<int>(aggregated.size()),
                receivedType,
                m_receivedFromEndpoint);
        }
    }

    if (m_isRunning) {
        startReceive();
    }
}

uint64_t NetworkController::generateId() {
    static std::atomic<uint64_t> counter{ 0ULL };
    return counter.fetch_add(1ULL, std::memory_order_relaxed);
}