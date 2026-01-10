#include "filesSender.h"
#include "packetsSender.h"
#include "utilities/logger.h"

namespace serverUpdater
{
FilesSender::FilesSender(asio::io_context& asioContext,
	asio::ip::tcp::socket& socket,
	utilities::SafeQueue<std::variant<Packet, std::filesystem::path>>& queue,
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
			std::optional<std::filesystem::path> pathCopy;
			m_queue.front_ref([&pathCopy](const std::variant<Packet, std::filesystem::path>& variant) {
				if (auto path = std::get_if<std::filesystem::path>(&variant)) {
					pathCopy = *path;
				}
			});
			if (!pathCopy.has_value() || !openFile(*pathCopy)) {
				return;
			}
		}
		else {
			return;
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
		m_queue.pop_ref([this](std::variant<Packet, std::filesystem::path>&&) {
			if (!m_queue.empty()) {
				m_queue.front_ref([this](const std::variant<Packet, std::filesystem::path>& variant) {
					if (auto path = std::get_if<std::filesystem::path>(&variant)) {
						m_buffer.fill(0);
						sendFileChunk();
					}
					else {
						m_packetsSender.send();
					}
				});
			}
		});
	}
}

bool FilesSender::openFile(const std::filesystem::path& path) {
	m_fileStream.open(path, std::ios::binary);

	if (!m_fileStream) {
		LOG_ERROR("Failed to open file: {}", path.string());
		return false;
	}

	LOG_DEBUG("Opened file for sending: {}", path.string());
	return true;
}
}