#include "connection.h"

#include <random>

#include "utilities/logger.h"
#include "utilities/utilities.h"
#include "utilities/errorCodeForLog.h"

#ifdef _WIN32
#include <mstcpip.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <netinet/tcp.h>
#endif

namespace serverUpdater
{
static void configureKeepalive(asio::ip::tcp::socket& socket) {
	std::error_code ec;
	socket.set_option(asio::ip::tcp::socket::keep_alive(true), ec);
	if (ec) {
		LOG_WARN("Failed to enable keepalive: {}", utilities::errorCodeForLog(ec));
		return;
	}

#ifdef _WIN32
	struct tcp_keepalive ka{};
	ka.onoff = 1;
	ka.keepalivetime = 2000;
	ka.keepaliveinterval = 1000;
	DWORD bytesReturned = 0;
	WSAIoctl(socket.native_handle(), SIO_KEEPALIVE_VALS,
		&ka, sizeof(ka), nullptr, 0, &bytesReturned, nullptr, nullptr);
#elif defined(__linux__)
	int fd = socket.native_handle();
	int idle = 2;
	int intvl = 1;
	int cnt = 3;
	setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
	setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(intvl));
	setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &cnt, sizeof(cnt));
#elif defined(__APPLE__)
	int fd = socket.native_handle();
	int idle = 2;
	int intvl = 1;
	int cnt = 3;
	setsockopt(fd, IPPROTO_TCP, TCP_KEEPALIVE, &idle, sizeof(idle));
	setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(intvl));
	setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &cnt, sizeof(cnt));
#endif

	LOG_DEBUG("Keepalive enabled (idle=2s, interval=1s, probes=3)");
}

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
	configureKeepalive(m_socket);

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