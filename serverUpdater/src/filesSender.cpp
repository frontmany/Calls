#include "filesSender.h"
#include "packetsSender.h"
#include "logger.h"



FilesSender::FilesSender(asio::io_context& asioContext,
	asio::ip::tcp::socket& socket,
	SafeDeque<std::variant<Packet, std::filesystem::path>>& queue,
	PacketsSender& packetsSender,
	std::function<void()>&& onError)
	: m_asioContext(asioContext),
	m_socket(socket),
	m_queue(queue),
	m_packetsSender(packetsSender),
	m_onError(std::move(onError))
{
}

void FilesSender::sendFile() {
	sendFileChunk();
} 

void FilesSender::sendFileChunk() {
	if (!m_fileStream.is_open()) {
		if (!m_queue.empty()) {
			auto& variant = m_queue.front();

			if (auto path = std::get_if<std::filesystem::path>(&variant)) {
				if (!openFile(*path)) return;
			}
		}
	}
	
	m_fileStream.read(m_buffer.data(), c_chunkSize);
	std::streamsize bytesRead = m_fileStream.gcount();

	if (bytesRead > 0) {
		asio::async_write(
			m_socket,
			asio::buffer(m_buffer.data(), c_chunkSize),
			[this](std::error_code ec, std::size_t) {
				if (ec) 
				{
					LOG_ERROR("Error sending file chunk: {}", ec.message());
					m_onError();
				}
				else {
					sendFileChunk();
				}
			}
		);
	}
	else {
		LOG_DEBUG("File transfer completed");
		m_fileStream.close();
		m_queue.pop_front();


		if (!m_queue.empty()) {
			auto& variant = m_queue.front();

			if (auto path = std::get_if<std::filesystem::path>(&variant)) {
				m_buffer.fill(0);
				sendFileChunk();
			}
			else {
				m_packetsSender.send();
			}
		}
	}
}

bool FilesSender::openFile(const std::filesystem::path& path) {
	auto& variant = m_queue.front();

	m_fileStream.open(path, std::ios::binary);

	if (!m_fileStream) {
		LOG_ERROR("Failed to open file: {}", path.string());
		return false;
	}

	LOG_DEBUG("Opened file for sending: {}", path.string());
	return true;
}
