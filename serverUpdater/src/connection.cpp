#include "connection.h"



Connection::Connection(
	asio::io_context& asioContext,
	asio::ip::tcp::socket&& socket,
	std::function<void(OwnedPacket&&)>&& queueReceivedPacket,
	std::function<void(std::shared_ptr<Connection>)>&& onDisconnected)
	: m_asioContext(asioContext),
	m_socket(std::move(socket)),
    m_queueReceivedPacket(std::move(queueReceivedPacket)), 
    m_onDisconnected(std::move(onDisconnected)),
    m_filesSender(m_asioContext, m_socket, [this]() {m_onDisconnected(shared_from_this()); }),
	m_packetsSender(m_asioContext, m_socket, [this]() {m_onDisconnected(shared_from_this()); }),
    m_packetsReceiver(m_socket, [this](Packet packet) {m_queueReceivedPacket(OwnedPacket{ shared_from_this(), packet }); }, [this]() {m_onDisconnected(shared_from_this()); })
{
	m_packetsReceiver.startReceiving();
}

Connection::~Connection() 
{
}

void Connection::sendUpdateRequiredPacket(const Packet& packet) {
    m_packetsSender.send(packet);
}

void Connection::sendUpdatePossiblePacket(const Packet& packet) {
    m_packetsSender.send(packet);
}

void Connection::sendUpdateNotNeededPacket(const Packet& packet) {
    m_packetsSender.send(packet);
}

void Connection::sendUpdate(const Packet& packet, const std::vector<std::filesystem::path>& paths) {
	m_packetsSender.send(packet);

	for (auto& filePath : paths) {
		m_filesSender.sendFile(filePath);
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
                        std::cerr << "Socket close error: " << ec.message() << "\n";
                    }
					else {
						std::cout << "files Connection closed successfully\n";
					}
                }
                catch (const std::exception& e) {
                    std::cerr << "Exception in Connection::close: " << e.what() << "\n";
                }
            }
        });
}
