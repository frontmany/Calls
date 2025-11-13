#include "networkController.h"
#include "logger.h"
#include <thread>
#include <algorithm>
#include <limits>

using namespace calls;

NetworkController::NetworkController()
    : m_socket(m_context),
    m_workGuard(asio::make_work_guard(m_context))
{
}

NetworkController::~NetworkController() {
    stop();
}

bool NetworkController::init(const std::string& host,
    const std::string& port,
    std::function<void(const unsigned char*, int, PacketType type)> onReceiveCallback,
    std::function<void()> onErrorCallback)
{
    m_receiveBuffer.resize(m_maxPacketSize);
    m_onErrorCallback = onErrorCallback;
    m_onReceiveCallback = onReceiveCallback;

    try {
        asio::ip::udp::resolver resolver(m_context);
        asio::ip::udp::resolver::results_type endpoints = resolver.resolve(asio::ip::udp::v4(), host, port);

        if (endpoints.empty()) {
            LOG_ERROR("No endpoints found for {}:{}", host, port);
            return false;
        }

        m_serverEndpoint = *endpoints.begin();
        m_socket.open(asio::ip::udp::v4());
        m_socket.bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), 0));

        LOG_INFO("Network controller initialized, server: {}:{}", host, port);
        return true;
    }
    catch (const std::exception& e) {
        LOG_ERROR("Initialization error: {}", e.what());
        m_workGuard.reset();
        stop();
        return false;
    }
}

void NetworkController::run() {
    m_asioThread = std::thread([this]() {m_context.run(); });
    startReceive();
}

void NetworkController::stop() {
    if (m_socket.is_open()) {
        asio::error_code ec;
        m_socket.close(ec);
        if (ec) {
            LOG_ERROR("Socket closing error: {}", ec.message());
        }
        else {
            LOG_DEBUG("Network controller stopped");
        }
    }

    if (!m_context.stopped()) {
        m_context.stop();
    }

    if (m_asioThread.joinable()) {
        m_asioThread.join();
    }
}

bool NetworkController::isRunning() const {
    return !m_context.stopped();
}

void NetworkController::send(const std::vector<unsigned char>& data, PacketType type) {
    if (data.empty() || !m_socket.is_open()) return;

    const std::size_t maxPayloadSize = m_maxPacketSize - (sizeof(ChunkHeader) + sizeof(PacketType));
    const std::uint16_t totalChunks = static_cast<std::uint16_t>((data.size() + maxPayloadSize - 1) / maxPayloadSize);
    uint32_t chunkId = generateId();

    for (std::uint16_t chunkNumber = 0; chunkNumber < totalChunks; ++chunkNumber) {
        auto chunk = std::make_shared<Chunk>();
        chunk->header.id = chunkId;
        chunk->header.chunksCount = totalChunks;
        chunk->header.number = chunkNumber;

        const std::size_t offset = static_cast<std::size_t>(chunkNumber) * maxPayloadSize;
        const std::size_t chunkSize = std::min(maxPayloadSize, data.size() - offset);

        chunk->type = type;
        chunk->payload.resize(chunkSize);
        std::memcpy(chunk->payload.data(), data.data() + offset, chunkSize);
        chunk->header.payloadSize = static_cast<uint32_t>(chunkSize);

        asio::post(m_socket.get_executor(),
            [this, chunk, type]() mutable {
                std::array<asio::const_buffer, 3> buffer = {
                    asio::buffer(chunk->payload.data(), chunk->payload.size()),
                    asio::buffer(&chunk->header, sizeof(ChunkHeader)),
                    asio::buffer(&chunk->type, sizeof(PacketType))
                };

                m_socket.async_send_to(buffer, m_serverEndpoint,
                    [this, chunk](const asio::error_code& error, std::size_t bytesSent) {
                        if (error && error != asio::error::operation_aborted) {
                            LOG_ERROR("Screen packet send error: {}", error.message());
                            m_onErrorCallback();
                        }
                    }
                );
            }
        );
    }
}

void NetworkController::send(std::vector<unsigned char>&& data, PacketType type) {
    send(data, type);
}

void NetworkController::send(PacketType type) {
    asio::post(m_socket.get_executor(),
        [this, type]() mutable {
            std::array<asio::const_buffer, 1> buffer = {
                asio::buffer(&type, sizeof(PacketType))
            };

            m_socket.async_send_to(buffer, m_serverEndpoint,
                [this](const asio::error_code& error, std::size_t bytesSent) {
                    if (error && error != asio::error::operation_aborted) {
                        LOG_ERROR("Screen packet send error: {}", error.message());
                        m_onErrorCallback();
                    }
                }
            );
        }
    );
}

void NetworkController::startReceive() {
    if (!m_socket.is_open()) return;

    m_socket.async_receive_from(asio::buffer(m_receiveBuffer), m_receivedFromEndpoint,
        [this](const asio::error_code& ec, std::size_t bytesTransferred) {
            if (ec && ec != asio::error::operation_aborted) {
                m_onErrorCallback();
            }

            handleReceive(bytesTransferred);
        }
    );
}

void NetworkController::handleReceive(std::size_t bytesTransferred) {
    if (bytesTransferred < sizeof(PacketType)) {
        LOG_WARN("Received packet too small: {} bytes (minimum: {})", bytesTransferred, sizeof(PacketType));
        startReceive();
        return;
    }

    PacketType receivedType;
    std::memcpy(&receivedType,
        m_receiveBuffer.data() + bytesTransferred - sizeof(PacketType),
        sizeof(PacketType)
    );

    const std::size_t payloadLength = bytesTransferred - sizeof(PacketType);

    if (payloadLength == 0) {
        m_onReceiveCallback(nullptr, 0, receivedType);
    }
    else {
        handleChunk(m_receiveBuffer.data(), payloadLength, receivedType);
    }

    startReceive();
}

uint32_t NetworkController::generateId() {
    static std::atomic<uint32_t> s_counter{ 0 };
    return s_counter++;
}

void NetworkController::handleChunk(const unsigned char* data, std::size_t length, PacketType type) {
    if (!data || length == 0) return;

    constexpr std::size_t headerSize = sizeof(ChunkHeader);
    if (length < headerSize) {
        LOG_WARN("Screen chunk too small: {} bytes, expected at least {}", length, headerSize);
        return;
    }

    ChunkHeader header;
    const unsigned char* headerData = data + (length - headerSize);
    std::memcpy(&header, headerData, headerSize);

    const std::size_t maxPayloadSize = m_maxPacketSize - (sizeof(ChunkHeader) + sizeof(PacketType));
    const bool headerValid =
        header.chunksCount > 0 &&
        header.number < header.chunksCount &&
        header.payloadSize <= maxPayloadSize &&
        header.payloadSize > 0;

    if (!headerValid) {
        LOG_WARN("Invalid screen chunk received (frame {}, chunk {}/{}, payload {}, max available {})",
            header.id,
            header.number,
            header.chunksCount,
            header.payloadSize,
            maxPayloadSize);
        return;
    }

    if (m_pendingPacket.chunks.empty() || m_pendingPacket.id != header.id) {
        m_pendingPacket = ScreenFrame{};
        m_pendingPacket.id = header.id;
        m_pendingPacket.totalChunks = header.chunksCount;
        m_pendingPacket.chunks.resize(header.chunksCount);
    }
    else if (m_pendingPacket.totalChunks != header.chunksCount) {
        LOG_WARN("Chunk count mismatch for frame {}: expected {}, got {}. Resetting aggregation.",
            header.id,
            m_pendingPacket.totalChunks,
            header.chunksCount);
        m_pendingPacket = ScreenFrame{};
        m_pendingPacket.id = header.id;
        m_pendingPacket.totalChunks = header.chunksCount;
        m_pendingPacket.chunks.resize(header.chunksCount);
    }

    const std::size_t chunkSize = static_cast<std::size_t>(header.payloadSize);

    auto& slot = m_pendingPacket.chunks[header.number];
    if (slot.empty()) {
        slot.assign(data, data + chunkSize);
        m_pendingPacket.totalSize += chunkSize;
        m_pendingPacket.receivedChunks++;
    }
    else {
        LOG_WARN("Duplicate chunk {}/{} received for frame {}", header.number + 1, header.chunksCount, header.id);
    }

    LOG_INFO("Chunk {}/{} received (size: {} bytes), type: {}",
        header.number + 1,
        header.chunksCount,
        chunkSize,
        packetTypeToString(type)
    );


    if (m_pendingPacket.receivedChunks == m_pendingPacket.totalChunks) {
        LOG_INFO("All chunks received, total {}", m_pendingPacket.chunks.size());

        std::vector<unsigned char> aggregated;
        aggregated.reserve(m_pendingPacket.totalSize);

        for (const auto& chunk : m_pendingPacket.chunks) {
            aggregated.insert(aggregated.end(), chunk.begin(), chunk.end());
        }

        m_onReceiveCallback(aggregated.data(), static_cast<int>(aggregated.size()), type);

        m_pendingPacket = ScreenFrame{};
    }
}