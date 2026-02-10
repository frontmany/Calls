#pragma once

#include <string>

namespace core::network 
{
	class NetworkConfig {
	public:
		NetworkConfig() = default;
		~NetworkConfig() = default;

		const std::string& getTcpHost();
		const std::string& getTcpPort();
		const std::string& getUdpHost();
		const std::string& getUdpPort();

		void setTcpParameters(std::string tcpHost, std::string tcpPort);
		void setUdpParameters(std::string udpHost, std::string udpPort);
		void resetTcpParameters();
		void resetUdpParameters();

	private:
		std::string m_tcpHost;
		std::string m_tcpPort;
		std::string m_udpHost;
		std::string m_udpPort;
	};
}