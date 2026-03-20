#include "filesSender.h"
#include "packetsSender.h"
#include "utilities/logger.h"
#include "utilities/errorCodeForLog.h"

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

bool FilesSender::prepareNextFile() {
    auto front = m_queue.front();
    if (!front.has_value()) {
        return false;
    }
    auto path = std::get_if<std::filesystem::path>(&(*front));
    if (!path) {
        m_packetsSender.send();
        return false;
    }
    m_currentFilePath = *path;
    if (!openFile(*m_currentFilePath)) {
        // Drop problematic file and continue with remaining queue.
        m_queue.try_pop();
        m_currentFilePath.reset();
        if (!m_queue.empty()) {
            sendFileChunk();
        }
        return false;
    }
    return true;
}

void FilesSender::sendFileChunk() {
	if (!m_fileStream.is_open()) {
        if (!prepareNextFile()) {
			return;
		}
	}
	
	m_buffer.fill(0);
	m_fileStream.read(m_buffer.data(), c_chunkSize);
	std::streamsize bytesRead = m_fileStream.gcount();

	if (bytesRead > 0) {
		asio::async_write(
			m_socket,
			asio::buffer(m_buffer.data(), static_cast<size_t>(bytesRead)),
			[this](std::error_code ec, std::size_t) {
				if (ec) 
				{
					LOG_ERROR("Error sending file chunk: {}", utilities::errorCodeForLog(ec));
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
        m_currentFilePath.reset();
		m_queue.try_pop();
		if (!m_queue.empty()) {
            auto front = m_queue.front();
            if (front.has_value() && std::holds_alternative<std::filesystem::path>(*front)) {
                m_buffer.fill(0);
                sendFileChunk();
            } else {
                m_packetsSender.send();
            }
		}
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