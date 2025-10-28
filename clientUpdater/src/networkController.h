#pragma once

#include <fstream>
#include <filesystem>
#include <queue>
#include <vector>
#include <string>
#include <thread>

#include "checkResult.h"
#include "safeDeque.h"
#include "packet.h"

#include "asio.hpp"
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

namespace updater {

class NetworkController {
private:
	struct FileMetadata {
		std::filesystem::path relativeFilePath;
		std::string fileHash;
		int expectedChunksCount;
		uint64_t lastChunkSize;
	};

public:
	NetworkController(std::function<void(CheckResult)> onCheckResult, std::function<void()> onAllFilesLoaded, std::function<void()> onConnected, std::function<void()> onError);
	void sendPacket(const Packet& packet);
	void connect(const std::string& host, const std::string& port);
	void disconnect();

private:
	void writeHeader(const Packet& packet);
	void writeBody(const Packet& packet);

	void readChunk();
	void readHandshake();
	void readHandshakeConfirmation();
	void readCheckResult();
	void readMetadataHeader();
	void readMetadataBody();
	void writeHandshake();

	void parseMetadata();
	void createConnection(const std::string& host, const uint16_t port);
	void reset(bool stopContext);
	void finalizeReceiving();
	void openFile();
	void deleteTempDirectory();

private:
	static constexpr int c_chunkSize = 8192;

	uint64_t m_handshakeOut = 0;
	uint64_t m_handshakeIn = 0;
	uint64_t m_handshakeConfirmation = 0;

	int m_currentChunksCount;

	Packet m_metadata;
	std::queue<FileMetadata> m_expectedFiles;
	std::vector<std::filesystem::path> m_filesToDelete;
	
	std::array<char, c_chunkSize> m_receiveBuffer{};
	std::ofstream m_fileStream;

	asio::io_context m_context;
	asio::ip::tcp::socket m_socket;
	asio::executor_work_guard<asio::io_context::executor_type, void, void> m_workGuard;
	std::thread m_asioThread;

	std::function<void(CheckResult)> m_onCheckResult;
	std::function<void()> m_onAllFilesLoaded;
	std::function<void()> m_onConnected;
	std::function<void()> m_onError;
};

}