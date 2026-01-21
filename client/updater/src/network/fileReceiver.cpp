#include "network/fileReceiver.h"

namespace updater
{
	namespace network
	{
		FileReceiver::FileReceiver(asio::ip::tcp::socket& socket,
			std::function<void(double)>&& onProgress,
			std::function<void()>&& onError,
			std::function<void()>&& onFileComplete)
			: m_socket(socket),
			m_onProgress(std::move(onProgress)),
			m_onError(std::move(onError)),
			m_onFileComplete(std::move(onFileComplete))
		{
		}

		void FileReceiver::startReceivingFile(const FileMetadata& fileInfo)
		{
			if (m_fileStream.is_open()) {
				m_fileStream.flush();
				m_fileStream.close();
			}
			
			m_currentFile = fileInfo;
			m_currentChunksCount = 0;
			m_receiveBuffer.fill(0);

			openFile(fileInfo.relativeFilePath);
		}

		void FileReceiver::receiveChunk()
		{
			asio::async_read(m_socket,
				asio::buffer(m_receiveBuffer.data(), c_chunkSize),
				[this](std::error_code ec, std::size_t bytesTransferred) {
					if (ec) {
						if (ec != asio::error::connection_reset && ec != asio::error::operation_aborted) {
							m_onError();
						}
					}
					else {
						m_currentChunksCount++;

						double fileProgress = 0.0;
						if (m_currentFile.expectedChunksCount > 0) {
							fileProgress = (static_cast<double>(m_currentChunksCount) * 100.0) / static_cast<double>(m_currentFile.expectedChunksCount);
							fileProgress = std::max(0.0, std::min(100.0, fileProgress));
							fileProgress = std::round(fileProgress * 100.0) / 100.0;
						}
						m_onProgress(fileProgress);

						if (m_currentChunksCount < m_currentFile.expectedChunksCount) {
							m_fileStream.write(m_receiveBuffer.data(), c_chunkSize);
							if (!m_fileStream.good()) {
								m_onError();
								return;
							}
							receiveChunk();
						}
						else if (m_currentChunksCount == m_currentFile.expectedChunksCount) {
							m_fileStream.write(m_receiveBuffer.data(), m_currentFile.lastChunkSize);
							if (!m_fileStream.good()) {
								m_onError();
								return;
							}

							finalizeFile();
							m_onFileComplete();
						}
					}
				});
		}

		const FileMetadata& FileReceiver::getCurrentFile() const
		{
			return m_currentFile;
		}

		void FileReceiver::openFile(const std::filesystem::path& filePath)
		{
			try {
				std::filesystem::create_directories(filePath.parent_path());
				m_fileStream.open(filePath, std::ios::binary | std::ios::trunc);

				if (!m_fileStream.is_open()) {
					m_onError();
				}
			}
			catch (const std::exception& e) {
				m_onError();
			}
		}

		void FileReceiver::finalizeFile()
		{
			if (m_fileStream.is_open()) {
				m_fileStream.flush();
				m_fileStream.close();
			}
			m_currentChunksCount = 0;
		}
	}
}
