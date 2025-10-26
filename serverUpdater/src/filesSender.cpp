#include "filesSender.h"
#include "logger.h"



FilesSender::FilesSender(asio::io_context& asioContext,
	asio::ip::tcp::socket& socket,
	std::function<void()>&& onError)
	: m_asioContext(asioContext),
	m_socket(socket),
	m_onError(std::move(onError))
{
}

void FilesSender::sendFile(const std::filesystem::path& fileData) {
	asio::post(m_asioContext, [this, fileData]() {
		bool allowed = m_queue.empty();

		m_queue.push_back(fileData);

		if (allowed) 
			sendFileChunk();
	});
} 

void FilesSender::sendFileChunk() {
	if (!m_fileStream.is_open()) 
		if (!openFile()) return;
	

	m_fileStream.read(m_buffer.data(), c_chunkSize);
	std::streamsize bytesRead = m_fileStream.gcount();

	if (bytesRead > 0) {
		asio::async_write(
			m_socket,
			asio::buffer(m_buffer.data(), c_chunkSize),
			[this](std::error_code ec, std::size_t) {
				if (ec) 
				{
					m_onError();
				}
				else {
					sendFileChunk();
				}
			}
		);
	}
	else {
		m_fileStream.close();
		m_queue.pop_front();

		if (!m_queue.empty())
			sendFileChunk();
	}
}

bool FilesSender::openFile() {
	auto& filePath = m_queue.front();
	m_fileStream.open(filePath, std::ios::binary);

	if (!m_fileStream) {
		DEBUG_LOG("Failed to open file");
		return false;
	}

	return true;
}
