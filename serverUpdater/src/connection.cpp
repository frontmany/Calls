#include "connection.h"

#include <random>

#include "logger.h"

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
	bool allowed = m_queue.empty();
	m_queue.push_back(packet);

	if (allowed)
		m_packetsSender.send();
}

void Connection::sendFile(const std::filesystem::path& path) {
	bool allowed = m_queue.empty();
	m_queue.push_back(path);

	if (allowed)
		m_filesSender.sendFile();
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
                        LOG_ERROR("Socket close error: {}", ec.message());
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
				LOG_ERROR("Handshake write error: {}", ec.message());
				m_onDisconnected(shared_from_this());
			}
		});
}

void Connection::readHandshake() {
	asio::async_read(m_socket, asio::buffer(&m_handshakeIn, sizeof(uint64_t)),
		[this](std::error_code ec, std::size_t length) {
			if (ec) {
				LOG_ERROR("Handshake read error: {}", ec.message());
				m_onDisconnected(shared_from_this());
			}
			else {
				if (scramble(m_handshakeOut) == m_handshakeIn) {
					LOG_DEBUG("Handshake successful");
					asio::async_write(m_socket, asio::buffer(&m_handshakeIn, sizeof(uint64_t)),
						[this](std::error_code ec, std::size_t length) {
							if (ec) {
								LOG_ERROR("Handshake confirmation write error: {}", ec.message());
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