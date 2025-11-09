#include "networkController.h"
#include "logger.h"
#include "screenPacket.h"
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
    m_receiveBuffer.resize(m_maxUdpPacketSize);
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

void NetworkController::sendVoice(std::vector<unsigned char>&& data, PacketType type) {
    asio::post(m_socket.get_executor(),
        [this, data = std::move(data), type, endpoint = m_serverEndpoint]() mutable {
            if (!m_socket.is_open()) {
                return;
            }

            const size_t totalSize = data.size() + sizeof(PacketType);
            if (totalSize > m_maxUdpPacketSize) {
                LOG_WARN("Voice packet too large: {} bytes (max: {})", totalSize, m_maxUdpPacketSize);
                return;
            }

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

void NetworkController::sendScreen(std::vector<unsigned char>&& data, PacketType type) {
    asio::post(m_socket.get_executor(),
        [this, data = std::move(data), type]() mutable {
            if (!m_socket.is_open()) {
                return;
            }

            constexpr std::size_t headerSize = ScreenChunkHeader::serializedSize();
            const std::size_t overhead = headerSize + sizeof(PacketType);
            if (overhead >= m_maxUdpPacketSize) {
                LOG_ERROR("Unable to send screen data: UDP packet size overhead exceeds limit ({} >= {})", overhead, m_maxUdpPacketSize);
                return;
            }

            const uint32_t frameId = m_nextScreenFrameId++;
            enqueueScreenFrame(std::move(data), type, frameId);

            if (!m_screenSendInProgress) {
                scheduleNextScreenDatagram();
            }
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
            if (totalSize > m_maxUdpPacketSize) {
                LOG_WARN("Data packet too large: {} bytes (max: {})", totalSize, m_maxUdpPacketSize);
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

void NetworkController::enqueueScreenFrame(std::vector<unsigned char>&& data, PacketType type, uint32_t frameId) {
    const std::size_t headerSize = ScreenChunkHeader::serializedSize();
    const std::size_t maxPayloadSize = m_maxUdpPacketSize - sizeof(PacketType) - headerSize;

    if (maxPayloadSize == 0) {
        LOG_ERROR("Unable to send screen data: max payload size resolved to zero");
        return;
    }

    const std::size_t totalChunks = std::max<std::size_t>(1, (data.size() + maxPayloadSize - 1) / maxPayloadSize);
    if (totalChunks > std::numeric_limits<uint16_t>::max()) {
        LOG_WARN("Screen frame {} requires {} chunks which exceeds supported limit", frameId, totalChunks);
        return;
    }

    for (std::size_t chunkIndex = 0; chunkIndex < totalChunks; ++chunkIndex) {
        auto datagram = std::make_shared<ScreenDatagram>();
        datagram->type = type;
        datagram->header.frameId = frameId;
        datagram->header.chunksCount = static_cast<uint16_t>(totalChunks);
        datagram->header.chunkIndex = static_cast<uint16_t>(chunkIndex);

        const std::size_t offset = chunkIndex * maxPayloadSize;
        const std::size_t remaining = offset < data.size() ? data.size() - offset : 0;
        const std::size_t chunkSize = std::min(maxPayloadSize, remaining);

        datagram->payload.resize(chunkSize);
        if (chunkSize > 0) {
            std::memcpy(datagram->payload.data(), data.data() + offset, chunkSize);
        }

        datagram->header.payloadSize = static_cast<uint32_t>(chunkSize);
        m_screenDatagramQueue.emplace_back(std::move(datagram));
    }
}

void NetworkController::scheduleNextScreenDatagram() {
    if (m_screenDatagramQueue.empty()) {
        m_screenSendInProgress = false;
        return;
    }

    m_screenSendInProgress = true;
    auto datagram = m_screenDatagramQueue.front();
    m_screenDatagramQueue.pop_front();

    std::array<asio::const_buffer, 3> buffers = {
        asio::buffer(&datagram->header, sizeof(ScreenChunkHeader)),
        asio::buffer(datagram->payload.data(), datagram->payload.size()),
        asio::buffer(&datagram->type, sizeof(PacketType))
    };

    m_socket.async_send_to(buffers, m_serverEndpoint,
        [this, datagram](const asio::error_code& error, std::size_t /*bytesSent*/) {
            if (error && error != asio::error::operation_aborted) {
                LOG_ERROR("Screen packet send error: {}", error.message());
                m_onErrorCallback();
            }

            scheduleNextScreenDatagram();
        }
    );
}

void NetworkController::handleScreenChunk(const unsigned char* data, std::size_t length) {
    if (!data || length == 0) {
        return;
    }

    constexpr std::size_t headerSize = ScreenChunkHeader::serializedSize();

    if (length < headerSize) {
        m_onReceiveCallback(data, static_cast<int>(length), PacketType::SCREEN);
        return;
    }

    ScreenChunkHeader header{};
    std::memcpy(&header, data, headerSize);

    const std::size_t payloadAvailable = length - headerSize;
    const bool headerValid =
        header.chunksCount > 0 &&
        header.chunkIndex < header.chunksCount &&
        header.payloadSize <= payloadAvailable;

    if (!headerValid) {
        LOG_WARN("Invalid screen chunk received (frame {}, chunk {}/{}, payload {}, available {})",
            header.frameId,
            header.chunkIndex,
            header.chunksCount,
            header.payloadSize,
            payloadAvailable);

        m_onReceiveCallback(data, static_cast<int>(length), PacketType::SCREEN);
        return;
    }

    auto& frame = m_pendingScreenFrames[header.frameId];

    if (frame.totalChunks != header.chunksCount) {
        frame.totalChunks = header.chunksCount;
        frame.receivedChunks = 0;
        frame.totalSize = 0;
        frame.chunks.assign(header.chunksCount, {});
        frame.received.assign(header.chunksCount, false);
    }

    if (header.chunkIndex >= frame.chunks.size()) {
        LOG_WARN("Received screen chunk index {} exceeds total {}", header.chunkIndex, frame.chunks.size());
        return;
    }

    if (!frame.received[header.chunkIndex]) {
        const unsigned char* chunkData = data + headerSize;
        const std::size_t chunkSize = static_cast<std::size_t>(header.payloadSize);
        frame.chunks[header.chunkIndex].assign(chunkData, chunkData + chunkSize);
        frame.received[header.chunkIndex] = true;
        frame.receivedChunks++;
        frame.totalSize += chunkSize;
    }

    if (frame.receivedChunks == frame.totalChunks) {
        std::vector<unsigned char> aggregated;
        aggregated.reserve(frame.totalSize);

        for (auto& chunk : frame.chunks) {
            aggregated.insert(aggregated.end(), chunk.begin(), chunk.end());
        }

        m_pendingScreenFrames.erase(header.frameId);

        m_reassembledScreenFrame = std::move(aggregated);
        m_onReceiveCallback(m_reassembledScreenFrame.data(), static_cast<int>(m_reassembledScreenFrame.size()), PacketType::SCREEN);
    }
}