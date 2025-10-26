#pragma once

#include <memory>
#include <fstream>
#include <filesystem>
#include <array>

#include "utility.h"
#include "fileData.h"
#include "safeDeque.h"

#include "asio.hpp"
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

class FilesSender {
public:
	FilesSender(asio::io_context& asioContext,
		asio::ip::tcp::socket& socket,
		std::function<void()>&& onError
	);

	void sendFile(const std::filesystem::path& filePath);

private:
	void sendFileChunk();
	bool openFile();

private:
	static constexpr uint32_t c_chunkSize = 8192;
	std::array<char, c_chunkSize> m_buffer{};
	SafeDeque<std::filesystem::path> m_queue;
	std::ifstream m_fileStream;

	asio::io_context& m_asioContext;
	asio::ip::tcp::socket& m_socket;

	std::function<void()> m_onError;
};
