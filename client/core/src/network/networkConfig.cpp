#include "networkConfig.h"

namespace core::network
{
	const std::string& NetworkConfig::getTcpHost() {
		return m_tcpHost;
	}

	const std::string& NetworkConfig::getTcpPort() {
		return m_tcpPort;
	}

	const std::string& NetworkConfig::getUdpHost() {
		return m_udpHost;
	}

	const std::string& NetworkConfig::getUdpPort() {
		return m_udpPort;
	}

	void NetworkConfig::setTcpParameters(std::string tcpHost, std::string tcpPort) {
		m_tcpHost = tcpHost;
		m_tcpPort = tcpPort;
	}

	void NetworkConfig::setUdpParameters(std::string udpHost, std::string udpPort) {
		m_udpHost = udpHost;
		m_udpPort = udpPort;
	}

	void NetworkConfig::resetTcpParameters() {
		m_tcpHost.clear();
		m_tcpPort.clear();
	}

	void NetworkConfig::resetUdpParameters() {
		m_udpHost.clear();
		m_udpPort.clear();
	}
}