#include "networkController.h"
#include "logger.h"
#include <thread>
#include <cstring>
#include <algorithm>
#include <atomic>
#include <limits>

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
        PacketType::SCREEN
    }
{
    asio::ip::udp::resolver resolver(m_context);
    asio::ip::udp::resolver::results_type endpoints = resolver.resolve(asio::ip::udp::v4(), "0.0.0.0", port);

    if (endpoints.empty()) {
        throw std::runtime_error("No endpoints found for port " + port);
    }

    m_serverEndpoint = *endpoints.begin();
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

void NetworkController::sendVoiceToClient(const asio::ip::udp::endpoint& clientEndpoint,
    const unsigned char* data, int length) {
    forwardChunkToClient(clientEndpoint, data, static_cast<std::size_t>(length), PacketType::VOICE);
}

void NetworkController::sendScreenToClient(const asio::ip::udp::endpoint& clientEndpoint,
    const unsigned char* data, int length) {
    forwardChunkToClient(clientEndpoint, data, static_cast<std::size_t>(length), PacketType::SCREEN);
}

void NetworkController::sendToClient(const asio::ip::udp::endpoint& clientEndpoint, PacketType type) {
    asio::post(m_socket.get_executor(),
        [this, clientEndpoint, type]() {
            if (!m_isRunning || !m_socket.is_open()) {
                return;
            }

            std::array<asio::const_buffer, 1> buffer = {
                asio::buffer(&type, sizeof(PacketType))
            };

            m_socket.async_send_to(buffer, clientEndpoint,
                [this](const asio::error_code& error, std::size_t bytesSent) {
                    if (error && error != asio::error::operation_aborted) {
                        LOG_ERROR("Packet send error: {}", error.message());
                        m_onNetworkErrorCallback();
                    }
                }
            );
        }
    );
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

void NetworkController::forwardChunkToClient(const asio::ip::udp::endpoint& clientEndpoint,
    const unsigned char* data, std::size_t length, PacketType type) {
    if (!m_isRunning || !m_socket.is_open()) {
        return;
    }

    if (data == nullptr || length == 0) {
        return;
    }

    auto chunkData = std::make_shared<std::vector<unsigned char>>(data, data + length);

    asio::post(m_socket.get_executor(),
        [this, clientEndpoint, chunkData, type]() {
            if (!m_isRunning || !m_socket.is_open()) {
                return;
            }

            std::array<asio::const_buffer, 2> buffers = {
                asio::buffer(chunkData->data(), chunkData->size()),
                asio::buffer(&type, sizeof(PacketType))
            };

            m_socket.async_send_to(buffers, clientEndpoint,
                [chunkData](const asio::error_code& error, std::size_t /*bytesSent*/) {
                    if (error && error != asio::error::operation_aborted) {
                        LOG_ERROR("Chunk forward error: {}", error.message());
                    }
                }
            );
        }
    );
}

void NetworkController::sendDataToClient(const asio::ip::udp::endpoint& clientEndpoint,
    const unsigned char* data, std::size_t length, PacketType type) {
    if (!m_isRunning || !m_socket.is_open()) {
        return;
    }

    if (data == nullptr || length == 0) {
        sendToClient(clientEndpoint, type);
        return;
    }

    const std::size_t maxPayloadSize = m_maxPacketSize - (sizeof(ChunkHeader) + sizeof(PacketType));
    if (maxPayloadSize == 0) {
        LOG_ERROR("Max payload size is zero, cannot send data");
        return;
    }

    const std::size_t chunksRequired = (length + maxPayloadSize - 1) / maxPayloadSize;
    if (chunksRequired > std::numeric_limits<std::uint16_t>::max()) {
        LOG_ERROR("Data packet requires too many chunks: {}", chunksRequired);
        return;
    }

    const std::uint16_t totalChunks = static_cast<std::uint16_t>(chunksRequired);
    const uint32_t chunkId = generateId();

    for (std::uint16_t chunkNumber = 0; chunkNumber < totalChunks; ++chunkNumber) {
        const std::size_t offset = static_cast<std::size_t>(chunkNumber) * maxPayloadSize;
        const std::size_t chunkSize = std::min(maxPayloadSize, length - offset);

        struct OutgoingChunk {
            ChunkHeader header;
            PacketType type;
            std::vector<unsigned char> payload;
        };

        auto chunk = std::make_shared<OutgoingChunk>();
        chunk->type = type;
        chunk->header.id = chunkId;
        chunk->header.number = chunkNumber;
        chunk->header.chunksCount = totalChunks;
        chunk->header.payloadSize = static_cast<uint32_t>(chunkSize);
        chunk->payload.resize(chunkSize);

        if (chunkSize > 0) {
            std::memcpy(chunk->payload.data(), data + offset, chunkSize);
        }

        asio::post(m_socket.get_executor(),
            [this, endpoint = clientEndpoint, chunk]() {
                if (!m_isRunning || !m_socket.is_open()) {
                    return;
                }

                std::array<asio::const_buffer, 3> buffers = {
                    asio::buffer(chunk->payload.data(), chunk->payload.size()),
                    asio::buffer(&chunk->header, sizeof(ChunkHeader)),
                    asio::buffer(&chunk->type, sizeof(PacketType))
                };

                m_socket.async_send_to(buffers, endpoint,
                    [chunk](const asio::error_code& error, std::size_t bytesSent) {
                        if (error && error != asio::error::operation_aborted) {
                            LOG_ERROR("Packet send error: {}", error.message());
                        }
                    }
                );
            }
        );
    }
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
        if (error != asio::error::operation_aborted) {
            if (error == asio::error::connection_refused) {
                startReceive();
                return;
            }

            m_onNetworkErrorCallback();
            return;
        }
    }

    if (bytesTransferred < sizeof(PacketType)) {
        LOG_WARN("Received packet too small: {} bytes (minimum: {})", bytesTransferred, sizeof(PacketType));
        if (m_isRunning) {
            startReceive();
        }
        return;
    }

    PacketType receivedType;
    std::memcpy(&receivedType,
        m_receiveBuffer.data() + bytesTransferred - sizeof(PacketType),
        sizeof(PacketType));

    const std::size_t payloadLength = bytesTransferred - sizeof(PacketType);
    if (payloadLength == 0) {
        if (m_onReceiveCallback) {
            m_onReceiveCallback(nullptr, 0, receivedType, m_receivedFromEndpoint);
        }

        if (m_isRunning) {
            startReceive();
        }
        return;
    }

    const std::size_t headerSize = sizeof(ChunkHeader);
    if (payloadLength < headerSize) {
        LOG_WARN("Received chunk too small: {} bytes, expected at least {}", payloadLength, headerSize);
        if (m_isRunning) {
            startReceive();
        }
        return;
    }

    ChunkHeader header;
    std::memcpy(&header,
        m_receiveBuffer.data() + payloadLength - headerSize,
        headerSize);

    const std::size_t maxPayloadSize = m_maxPacketSize - (sizeof(ChunkHeader) + sizeof(PacketType));
    const bool headerValid =
        header.chunksCount > 0 &&
        header.number < header.chunksCount &&
        header.payloadSize <= maxPayloadSize &&
        header.payloadSize > 0;

    if (!headerValid) {
        LOG_WARN("Invalid chunk metadata (id {}, number {}, total {}, payload {}, max {})",
            header.id,
            header.number,
            header.chunksCount,
            header.payloadSize,
            maxPayloadSize);
        if (m_isRunning) {
            startReceive();
        }
        return;
    }

    const std::size_t chunkSize = static_cast<std::size_t>(header.payloadSize);
    if (chunkSize > payloadLength - headerSize) {
        LOG_WARN("Declared payload size {} exceeds available data {}",
            chunkSize,
            payloadLength - headerSize);
        if (m_isRunning) {
            startReceive();
        }
        return;
    }

    if (m_forwardWithoutAssembly.find(receivedType) != m_forwardWithoutAssembly.end()) {
        if (m_onReceiveCallback) {
            m_onReceiveCallback(
                m_receiveBuffer.data(),
                static_cast<int>(payloadLength),
                receivedType,
                m_receivedFromEndpoint);
        }

        if (m_isRunning) {
            startReceive();
        }
        return;
    }

    std::vector<unsigned char> chunk(chunkSize);
    if (chunkSize > 0) {
        std::memcpy(chunk.data(), m_receiveBuffer.data(), chunkSize);
    }

    const std::string endpointKey = makeEndpointKey(m_receivedFromEndpoint);
    auto& endpointPackets = m_pendingPackets[endpointKey];
    auto& pendingPacket = endpointPackets[header.id];

    if (pendingPacket.chunks.empty()) {
        pendingPacket.totalChunks = header.chunksCount;
        pendingPacket.chunks.resize(header.chunksCount);
    }

    if (pendingPacket.totalChunks != header.chunksCount) {
        pendingPacket = PendingPacket{};
        pendingPacket.totalChunks = header.chunksCount;
        pendingPacket.chunks.resize(header.chunksCount);
    }

    if (header.number >= pendingPacket.chunks.size()) {
        LOG_WARN("Chunk number {} is out of range for total {}", header.number, pendingPacket.chunks.size());
        endpointPackets.erase(header.id);
        if (endpointPackets.empty()) {
            m_pendingPackets.erase(endpointKey);
        }

        if (m_isRunning) {
            startReceive();
        }
        return;
    }

    if (pendingPacket.chunks[header.number].empty()) {
        pendingPacket.totalSize += chunk.size();
        pendingPacket.receivedChunks++;
        pendingPacket.chunks[header.number] = std::move(chunk);
    }

    if (pendingPacket.receivedChunks == pendingPacket.totalChunks) {
        std::vector<unsigned char> aggregated;
        aggregated.reserve(pendingPacket.totalSize);

        for (const auto& part : pendingPacket.chunks) {
            aggregated.insert(aggregated.end(), part.begin(), part.end());
        }

        endpointPackets.erase(header.id);
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

uint32_t NetworkController::generateId() {
    static std::atomic<uint32_t> counter{ 0 };
    return counter++;
}