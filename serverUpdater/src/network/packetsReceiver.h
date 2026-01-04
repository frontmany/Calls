#pragma once

#include <functional>

#include "packet.h"

#include "asio.hpp"

namespace serverUpdater
{
class PacketsReceiver {
public:
	PacketsReceiver(asio::ip::tcp::socket& socket,
		std::function<void(Packet&&)>&& queueReceivedPacket,
		std::function<void()>&& onDisconnect
	);

	void startReceiving();

private:
	void readHeader();
	void readBody();

private:
	asio::ip::tcp::socket& m_socket;
	Packet m_temporaryPacket;

	std::function<void()> m_onClientDisconnected;
	std::function<void(Packet&&)> m_queueReceivedPacket;
	std::function<void()> m_onDisconnect;
};
}

