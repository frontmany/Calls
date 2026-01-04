#pragma once

#include <memory>
#include <fstream>
#include <filesystem>
#include <variant>
#include <array>

#include "packet.h"
#include "../utilities/safeDeque.h"

#include "asio.hpp"
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

namespace serverUpdater
{
class PacketsSender;

class FilesSender {
public:
	FilesSender(asio::io_context& asioContext,
		asio::ip::tcp::socket& socket,
		utilities::SafeDeque<std::variant<Packet, std::filesystem::path>>& queue,
		PacketsSender& packetsSender,
		std::function<void()>&& onError
	);
	void sendFile();

private:
	void sendFileChunk();
	bool openFile(const std::filesystem::path& path);

private:
	PacketsSender& m_packetsSender;
	utilities::SafeDeque<std::variant<Packet, std::filesystem::path>>& m_queue;

	static constexpr uint32_t c_chunkSize = 8192;
	std::array<char, c_chunkSize> m_buffer{};
	std::ifstream m_fileStream;

	asio::io_context& m_asioContext;
	asio::ip::tcp::socket& m_socket;

	std::function<void()> m_onError;
};
}