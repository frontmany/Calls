#include "connection.h"

#include <random>

#include "utilities/logger.h"
#include "utilities/utilities.h"
#include "utilities/errorCodeForLog.h"

namespace serverUpdater
{
Connection::Connection(
	asio::io_context& asioContext,
	asio::ip::tcp::socket&& socket,
	std::function<void(OwnedPacket&&)>&& queueReceivedPacket,
	std::function<void(std::shared_ptr<Connection>)>&& onDisconnected)
	: m_asioContext(asioContext),
	m_socket(std::move(socket)),
	m_queueReceivedPacket(std::move(queueReceivedPacket)),
	m_onDisconnected(std::move(onDisconnected)),
	m_filesSender(m_asioContext, m_socket, m_queue, m_packetsSender, [this]() {m_onDisconnected(shared_from_this()); }),
	m_packetsSender(m_asioContext, m_socket, m_queue, m_filesSender, [this]() {m_onDisconnected(shared_from_this()); }),
	m_packetsReceiver(m_socket, [this](Packet&& packet) {
	m_queueReceivedPacket(OwnedPacket{ shared_from_this(), std::move(packet) }); }, [this]() {m_onDisconnected(shared_from_this()); })
{
	// Enable TCP keepalive to detect dead connections
	std::error_code ec;
	m_socket.set_option(asio::ip::tcp::socket::keep_alive(true), ec);
	if (ec) {
		LOG_WARN("Failed to enable keepalive: {}", utilities::errorCodeForLog(ec));
	}

	std::random_device rd;
	std::mt19937_64 gen(rd());
	std::uniform_int_distribution<uint64_t> dis;
	m_handshakeOut = dis(gen);

	readHandshake();
	writeHandshake();
}

Connection::~Connection() 
{
}

void Connection::sendPacket(const Packet& packet) {
	m_queue.push(packet);
	
	if (m_queue.size() == 1) {
		m_packetsSender.send();
	}
}

void Connection::sendFile(const std::filesystem::path& path) {
	bool wasEmpty = m_queue.empty();
	m_queue.push(path);
	
	if (wasEmpty) {
		m_filesSender.sendFile();
	}
}

void Connection::close() {
    auto self = shared_from_this();

    asio::post(m_asioContext,
        [this, self]() {
            if (m_socket.is_open()) {
                try {
                    std::error_code ec;

                    m_socket.close(ec);
                    if (ec) {
                        LOG_ERROR("Socket close error: {}", utilities::errorCodeForLog(ec));
                    }
					else {
						LOG_DEBUG("Connection closed successfully");
					}
                }
                catch (const std::exception& e) {
                    LOG_ERROR("Exception in Connection::close: {}", e.what());
                }
            }
        });
}

void Connection::writeHandshake() {
	asio::async_write(m_socket, asio::buffer(&m_handshakeOut, sizeof(uint64_t)),
		[this](std::error_code ec, std::size_t length) {
			if (ec) {
				LOG_ERROR("Handshake write error: {}", utilities::errorCodeForLog(ec));
				m_onDisconnected(shared_from_this());
			}
		});
}

void Connection::readHandshake() {
	asio::async_read(m_socket, asio::buffer(&m_handshakeIn, sizeof(uint64_t)),
		[this](std::error_code ec, std::size_t length) {
			if (ec) {
				LOG_ERROR("Handshake read error: {}", utilities::errorCodeForLog(ec));
				m_onDisconnected(shared_from_this());
			}
			else {
				if (utilities::scramble(m_handshakeOut) == m_handshakeIn) {
					LOG_DEBUG("Handshake successful");
					asio::async_write(m_socket, asio::buffer(&m_handshakeIn, sizeof(uint64_t)),
						[this](std::error_code ec, std::size_t length) {
							if (ec) {
								LOG_ERROR("Handshake confirmation write error: {}", utilities::errorCodeForLog(ec));
								m_onDisconnected(shared_from_this());
							}
						});
					m_packetsReceiver.startReceiving();
				}
				else {
					LOG_WARN("Handshake validation failed");
					m_onDisconnected(shared_from_this());
				}
			}
		});
}
}