#pragma once

#include <fstream>
#include <filesystem>
#include <condition_variable>
#include <queue>
#include <memory>
#include <vector>
#include <string>
#include <thread>
#include <atomic>

#include "updatesCheckResult.h"
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
	NetworkController(std::function<void(UpdatesCheckResult)>&& onCheckResult, std::function<void(double)>&& onLoadingProgress, std::function<void(bool)>&& onAllFilesLoaded, std::function<void()>&& onConnected, std::function<void()>&& onError);
	void sendPacket(const Packet& packet);
	void connect(const std::string& host, const std::string& port);
	void disconnect();
	void requestShutdown();

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
	void moveConfigFromTemp();
	bool shouldIgnoreError(const std::error_code& ec) const;

private:
	static constexpr int c_chunkSize = 8192;

	uint64_t m_handshakeOut = 0;
	uint64_t m_handshakeIn = 0;
	uint64_t m_handshakeConfirmation = 0;

	int m_currentChunksCount = 0;
	uint64_t m_bytesReceived = 0;
	uint64_t m_totalBytes = 0;

	Packet m_metadata;
	std::queue<FileMetadata> m_expectedFiles;
	std::vector<std::filesystem::path> m_filesToDelete;
	
	std::array<char, c_chunkSize> m_receiveBuffer{};
	std::ofstream m_fileStream;

	asio::io_context m_context;
	asio::ip::tcp::socket m_socket;
	std::unique_ptr<asio::executor_work_guard<asio::io_context::executor_type>> m_workGuard;
	std::thread m_asioThread;

	std::function<void(UpdatesCheckResult)> m_onCheckResult;
	std::function<void(double)> m_onLoadingProgress;
	std::function<void(bool)> m_onAllFilesLoaded;
	std::function<void()> m_onConnected;
	std::function<void()> m_onError;

	std::atomic_bool m_shuttingDown = false;
};

}