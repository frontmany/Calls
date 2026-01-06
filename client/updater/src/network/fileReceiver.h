#pragma once

#include <fstream>
#include <filesystem>
#include <queue>
#include <functional>

#include <asio.hpp>

namespace updater
{
	namespace network
	{
		struct FileMetadata
		{
			std::filesystem::path relativeFilePath;
			std::string fileHash;
			int expectedChunksCount;
			uint64_t lastChunkSize;
			uint64_t fileSize;
		};

		class FileReceiver
		{
		public:
			static constexpr int c_chunkSize = 8192;

			FileReceiver(asio::ip::tcp::socket& socket,
				std::function<void(double)>&& onProgress,
				std::function<void()>&& onError,
				std::function<void()>&& onFileComplete
			);

			void startReceivingFile(const FileMetadata& fileInfo);
			void receiveChunk();

			const FileMetadata& getCurrentFile() const;

		private:
			void openFile(const std::filesystem::path& filePath);
			void finalizeFile();

			asio::ip::tcp::socket& m_socket;
			std::ofstream m_fileStream;
			std::array<char, c_chunkSize> m_receiveBuffer{};

			FileMetadata m_currentFile;
			int m_currentChunksCount = 0;

			std::function<void(double)> m_onProgress;
			std::function<void()> m_onError;
			std::function<void()> m_onFileComplete;
		};
	}
}
