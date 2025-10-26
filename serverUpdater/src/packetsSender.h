#pragma once

#include <functional>

#include "safeDeque.h"
#include "packet.h"

#include "asio.hpp"

class PacketsSender {
public:
	PacketsSender(asio::io_context& asioContext,
		asio::ip::tcp::socket& socket, 
		std::function<void()>&& onError
	);

	void send(const Packet& packet);

private:
	void writeHeader();
	void writeBody();

private:
	asio::ip::tcp::socket& m_socket;
	asio::io_context& m_asioContext;

	SafeDeque<Packet> m_queue;

	std::function<void()> m_onError;
};

