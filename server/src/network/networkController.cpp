#include "network/networkController.h"

namespace server::network
{
	NetworkController::NetworkController(
		uint16_t tcpPort,
		const std::string& udpPort,
		tcp::Server::OnPacket onTcpPacket,
		tcp::Server::OnDisconnect onTcpDisconnect,
		std::function<void(const unsigned char*, int, uint32_t, const asio::ip::udp::endpoint&)> onUdpReceive)
		: m_tcpServer(tcpPort, std::move(onTcpPacket), std::move(onTcpDisconnect))
	{
		m_udpServer.init(udpPort, std::move(onUdpReceive));
	}

	NetworkController::~NetworkController() {
		stop();
	}

	void NetworkController::start() {
		m_udpServer.start();
		m_tcpServer.start();
	}

	void NetworkController::stop() {
		m_tcpServer.stop();
		m_udpServer.stop();
	}

	bool NetworkController::sendUdp(const std::vector<unsigned char>& data, uint32_t type, const asio::ip::udp::endpoint& endpoint) {
		return m_udpServer.send(data, type, endpoint);
	}

	bool NetworkController::sendUdp(std::vector<unsigned char>&& data, uint32_t type, const asio::ip::udp::endpoint& endpoint) {
		return m_udpServer.send(std::move(data), type, endpoint);
	}

	bool NetworkController::sendUdp(const unsigned char* data, int size, uint32_t type, const asio::ip::udp::endpoint& endpoint) {
		return m_udpServer.send(data, size, type, endpoint);
	}
}
