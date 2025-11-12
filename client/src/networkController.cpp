#include "networkController.h"
#include "logger.h"
#include "screenChunk.h"
#include <thread>
#include <cstring>
#include <iostream>
#include <array>
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

bool NetworkController::stopped() const {
    return m_context.stopped();
}

void NetworkController::sendVoice(std::vector<unsigned char>&& data) {
    asio::post(m_socket.get_executor(),
        [this, data = std::move(data), endpoint = m_serverEndpoint]() mutable {
            if (!m_socket.is_open()) {
                return;
            }

            const size_t totalSize = data.size() + sizeof(PacketType);
            if (totalSize > m_maxPacketSize) {
                LOG_WARN("Voice packet too large: {} bytes (max: {})", totalSize, m_maxPacketSize);
                return;
            }

            PacketType type = PacketType::VOICE;
            std::array<asio::const_buffer, 2> buffers = {
                asio::buffer(data.data(), data.size()),
                asio::buffer(&type, sizeof(PacketType))
            };

            m_socket.async_send_to(buffers, endpoint,
                [this](const asio::error_code& error, std::size_t bytesSent) {
                    if (error && error != asio::error::operation_aborted) {
                        LOG_ERROR("Voice packet send error: {}", error.message());
                        m_onErrorCallback();
                    }
                }
            );
        }
    );
}

void NetworkController::sendScreen(std::vector<unsigned char>&& data) {
    asio::post(m_socket.get_executor(),
        [this, data = std::move(data)]() mutable {
            if (!m_socket.is_open()) return;

            const bool wasEmpty = m_screenChunksQueue.empty();

            splitAndEnqueueScreenFrames(std::move(data));

            if (wasEmpty && !m_screenChunksQueue.empty())
                sendScreenChunk();
        }
    );
}


void NetworkController::sendPacket(std::string&& data, PacketType type) {
    asio::post(m_socket.get_executor(),
        [this, data = std::move(data), type, endpoint = m_serverEndpoint]() mutable {
            if (!m_socket.is_open()) {
                return;
            }

            const size_t totalSize = data.size() + sizeof(PacketType);
            if (totalSize > m_maxPacketSize) {
                LOG_WARN("Data packet too large: {} bytes (max: {})", totalSize, m_maxPacketSize);
                return;
            }

            std::array<asio::const_buffer, 2> buffers = {
                asio::buffer(data.data(), data.size()),
                asio::buffer(&type, sizeof(PacketType))
            };

            m_socket.async_send_to(buffers, endpoint,
                [this](const asio::error_code& error, std::size_t bytesSent) {
                    if (error && error != asio::error::operation_aborted) {
                        LOG_ERROR("Data packet send error: {}", error.message());
                        m_onErrorCallback();
                    }
                }
            );
        }
    );
}

void NetworkController::sendPacket(PacketType type) {
    asio::post(m_socket.get_executor(),
        [this, type]() {
            if (!m_socket.is_open()) {
                return;
            }

            std::array<asio::const_buffer, 1> buffer = {
                asio::buffer(&type, sizeof(PacketType))
            };

            m_socket.async_send_to(buffer, m_serverEndpoint,
                [this](const asio::error_code& error, std::size_t bytesSent) {
                    if (error && error != asio::error::operation_aborted) {
                        LOG_ERROR("Packet send error: {}", error.message());
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
        sizeof(PacketType));

    const std::size_t dataLength = bytesTransferred - sizeof(PacketType);

    if (receivedType == PacketType::SCREEN) {
        handleScreenChunk(m_receiveBuffer.data(), dataLength);
    }
    else {
        m_onReceiveCallback(m_receiveBuffer.data(), static_cast<int>(dataLength), receivedType);
    }

    startReceive();
}

uint32_t NetworkController::generateChunkId() {
    static std::atomic<uint32_t> s_counter{ 0 };
    return s_counter++;
}

void NetworkController::splitAndEnqueueScreenFrames(std::vector<unsigned char>&& data) {
    if (data.empty()) return;

    const std::size_t headerAndTypeSize = sizeof(ScreenChunkHeader) + sizeof(PacketType);
    if (m_maxPacketSize <= headerAndTypeSize) {
        LOG_ERROR("Max packet size {} too small for screen packets", m_maxPacketSize);
        return;
    }

    const std::size_t maxPayloadSize = m_maxPacketSize - headerAndTypeSize;
    const std::uint16_t totalChunks = static_cast<std::uint16_t>((data.size() + maxPayloadSize - 1) / maxPayloadSize);

    uint32_t chunkId = generateChunkId();

    for (std::uint16_t chunkNumber = 0; chunkNumber < totalChunks; ++chunkNumber) {
        auto chunk = std::make_shared<ScreenChunk>();
        chunk->header.id = chunkId;
        chunk->header.chunksCount = totalChunks;
        chunk->header.number = chunkNumber;

        const std::size_t offset = static_cast<std::size_t>(chunkNumber) * maxPayloadSize;
        const std::size_t chunkSize = std::min(maxPayloadSize, data.size() - offset);

        chunk->payload.resize(chunkSize);
        std::memcpy(chunk->payload.data(), data.data() + offset, chunkSize);

        chunk->header.payloadSize = static_cast<uint32_t>(chunkSize);
        m_screenChunksQueue.emplace(std::move(chunk));
    }
}

void NetworkController::sendScreenChunk() {
    auto chunk = m_screenChunksQueue.front();
    m_screenChunksQueue.pop();

    static const PacketType packetType = PacketType::SCREEN;

    std::array<asio::const_buffer, 3> buffer = {
        asio::buffer(chunk->payload.data(), chunk->payload.size()),
        asio::buffer(&chunk->header, sizeof(ScreenChunkHeader)),
        asio::buffer(&packetType, sizeof(PacketType))
    };

    m_socket.async_send_to(buffer, m_serverEndpoint,
        [this, chunk](const asio::error_code& error, std::size_t bytesSent) {
            if (error && error != asio::error::operation_aborted) {
                LOG_ERROR("Screen packet send error: {}", error.message());
                m_onErrorCallback();
            }

            if (!m_screenChunksQueue.empty()) {
                sendScreenChunk();
            }
        }
    );
}

void NetworkController::handleScreenChunk(const unsigned char* data, std::size_t length) {
    if (!data || length == 0) return;

    constexpr std::size_t headerSize = sizeof(ScreenChunkHeader);
    if (length < headerSize) {
        LOG_WARN("Screen chunk too small: {} bytes, expected at least {}", length, headerSize);
        return;
    }

    ScreenChunkHeader header;
    const unsigned char* headerData = data + (length - headerSize);
    std::memcpy(&header, headerData, headerSize);

    const std::size_t headerAndTypeSize = sizeof(ScreenChunkHeader) + sizeof(PacketType);
    if (m_maxPacketSize <= headerAndTypeSize) {
        LOG_ERROR("Max packet size {} too small for screen packets", m_maxPacketSize);
        return;
    }

    const std::size_t maxPayloadSize = m_maxPacketSize - headerAndTypeSize;
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

    auto& frame = m_pendingScreenFrames[header.id];

    if (frame.totalChunks == 0) {
        frame.totalChunks = header.chunksCount;
        frame.receivedChunks = 0;
        frame.totalSize = 0;
        frame.chunks.resize(header.chunksCount);
        frame.received.assign(header.chunksCount, false);
    }

    if (header.number >= frame.chunks.size()) {
        LOG_WARN("Received screen chunk index {} exceeds total {}", header.number, frame.chunks.size());
        return;
    }

    if (!frame.received[header.number]) {
        const unsigned char* chunkData = data;
        const std::size_t chunkSize = static_cast<std::size_t>(header.payloadSize);
        const std::size_t availablePayload = length - headerSize;

        if (chunkSize > availablePayload) {
            LOG_WARN("Screen chunk payload mismatch: declared {}, available {}", chunkSize, availablePayload);
            return;
        }

        frame.chunks[header.number].assign(chunkData, chunkData + chunkSize);
        frame.received[header.number] = true;
        frame.receivedChunks++;
        frame.totalSize += chunkSize;

        LOG_INFO("Chunk {}/{} received (size: {} bytes)",
            header.number + 1,
            header.chunksCount,
            chunkSize);
    }

    if (frame.receivedChunks == frame.totalChunks) {
        LOG_INFO("All chunks received, total {}", frame.chunks.size());

        std::vector<unsigned char> aggregated;
        aggregated.reserve(frame.totalSize);

        for (const auto& chunk : frame.chunks) {
            aggregated.insert(aggregated.end(), chunk.begin(), chunk.end());
        }

        m_pendingScreenFrames.erase(header.id);

        m_onReceiveCallback(aggregated.data(), static_cast<int>(aggregated.size()), PacketType::SCREEN);
    }
}