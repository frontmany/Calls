#include "packetSender.h"

#include <algorithm>

#include "utilities/logger.h"
#include "utilities/errorCodeForLog.h"

namespace server::network::udp
{
    PacketSender::PacketSender()
        : m_isSending(false), m_currentDatagramIndex(0)
    {
    }

    PacketSender::~PacketSender() {
        stop();
    }

    void PacketSender::init(asio::ip::udp::socket& socket, std::function<void()> onErrorCallback) {
        m_socket = std::ref(socket);
        m_onErrorCallback = std::move(onErrorCallback);
        m_isSending = false;
        m_currentDatagrams.clear();
        m_currentDatagramIndex = 0;

        m_packetQueue.clear();
    }

    void PacketSender::send(const Packet& packet) {
        bool wasEmpty = m_packetQueue.empty();
        m_packetQueue.push(packet);

        if (wasEmpty && !m_isSending.load()) {
            startSendingIfIdle();
        }
    }

    void PacketSender::stop() {
        m_isSending = false;
        m_currentDatagrams.clear();
        m_currentDatagramIndex = 0;

        m_packetQueue.clear();
    }

    void PacketSender::startSendingIfIdle() {
        if (m_isSending.exchange(true)) {
            return;
        }

        if (!m_socket.has_value()) {
            m_isSending = false;
            return;
        }

        processNextPacketFromQueue();
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

    void PacketSender::processNextPacketFromQueue() {
        if (!m_socket.has_value()) {
            m_isSending = false;
            return;
        }

        auto packetOpt = m_packetQueue.try_pop();
        if (!packetOpt.has_value()) {
            m_isSending = false;
            return;
        }

        Packet packet = std::move(*packetOpt);
        m_currentEndpoint = packet.endpoint;
        m_currentDatagrams = splitPacket(packet);
        m_currentDatagramIndex = 0; 

        if (!m_currentDatagrams.empty()) {
            sendNextDatagram();
        }
        else {
            processNextPacketFromQueue();
        }
    }

    void PacketSender::sendNextDatagram() {
        if (!m_socket.has_value()) {
            m_isSending = false;
            return;
        }

        if (m_currentDatagramIndex >= m_currentDatagrams.size()) {
            processNextPacketFromQueue();
            return;
        }

        auto& socket = m_socket->get();
        auto& datagram = m_currentDatagrams[m_currentDatagramIndex];

        socket.async_send_to(
            asio::buffer(datagram),
            m_currentEndpoint,
            [this](std::error_code ec, std::size_t bytesTransferred) {
                if (ec) {
                    LOG_ERROR("Failed to send datagram chunk: {}", server::utilities::errorCodeForLog(ec));
                    m_isSending = false;
                    if (m_onErrorCallback) {
                        m_onErrorCallback();
                    }
                    // Resume sending from queue; otherwise one send error permanently stops all sending
                    // (pongs stop -> both clients see Reconnecting). Transient errors can recover.
                    processNextPacketFromQueue();
                    return;
                }

                ++m_currentDatagramIndex;

                if (m_currentDatagramIndex >= m_currentDatagrams.size()) {
                    processNextPacketFromQueue();
                }
                else {
                    sendNextDatagram();
                }
            }
        );
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
}

