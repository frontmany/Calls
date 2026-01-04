#pragma once

#include <functional>
#include <variant>
#include <filesystem>

#include "utilities/safeDeque.h"
#include "packet.h"

#include "asio.hpp"

namespace serverUpdater
{
class FilesSender;

class PacketsSender {
public:
	PacketsSender(asio::io_context& asioContext,
		asio::ip::tcp::socket& socket, 
		utilities::SafeDeque<std::variant<Packet, std::filesystem::path>>& queue,
		FilesSender& filesSender,
		std::function<void()>&& onError
	);

	void send();

private:
	void writeHeader();
	void writeBody(const Packet* packet);
	void resolveSending();

private:
	utilities::SafeDeque<std::variant<Packet, std::filesystem::path>>& m_queue;
	FilesSender& m_filesSender;

	asio::ip::tcp::socket& m_socket;
	asio::io_context& m_asioContext;

	std::function<void()> m_onError;
};
}

