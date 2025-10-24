#include "net_filesSender.h"
#include "packetsBuilder.h"
#include "fileLocationInfo.h"
#include "file.h"
#include "avatar.h"


namespace net
{
	FilesSender::FilesSender(asio::io_context& asioContext,
		asio::ip::tcp::socket& socket,
		std::function<void(FileLocationInfo&&, uint32_t)> onSendProgress,
		std::function<void()> onAvatarSent,
		std::function<void(std::error_code ec, FileLocationInfo&&)> onSendError)
		: m_socket(socket),
		m_onSendProgressUpdate(onSendProgress),
		m_onAvatarSent(onAvatarSent),
		m_onSendError(onSendError),
		m_asioContext(asioContext)
	{
		m_totalBytesSent = 0;
	}

	void FilesSender::sendFile(const std::variant<FileTransferData, AvatarTransferData>& file) {
		asio::post(m_asioContext, [this, file]() {
			bool isSendingAllowed = m_outgoingFilesQueue.empty();
			m_outgoingFilesQueue.push_back(file);
			if (isSendingAllowed) {
				sendMetadata();
			}
		});
	}

	void FilesSender::sendMetadata() {
		utility::generateAESKey(m_sessionKey);

		auto& file = m_outgoingFilesQueue.front_mut();
		if (auto data = std::get_if<FileTransferData>(&file)) {
			m_metadataPacket.setType(PacketType::PREPARE_TO_RECEIVE_FILE);
			m_metadataPacket.add(PacketsBuilder::getFileMetadataPacket(*data, m_sessionKey));

		}
		else if (auto data = std::get_if<AvatarTransferData>(&file)) {
			m_metadataPacket.setType(PacketType::UPDATE_MY_AVATAR);
			m_metadataPacket.add(PacketsBuilder::getAvatarMetadataPacket(*data));
		}

		asio::async_write(
			m_socket,
			asio::buffer(&m_metadataPacket.header(), Packet::sizeOfHeader()),
			[this](std::error_code ec, std::size_t length) {
				if (ec)
				{
					processError(ec);
				}
				else
				{
					asio::async_write(
						m_socket,
						asio::buffer(m_metadataPacket.body().data(), m_metadataPacket.body().size()),
						[this](std::error_code ec, std::size_t length) {
							if (ec) 
							{
								processError(ec);
							}
							else {
								if (m_metadataPacket.type() == PacketType::UPDATE_MY_AVATAR) {
									sendFileChunkWithoutEncryption();
								}
								else {
									sendFileChunk();
								}
							}
						}
					);
				}
			}
		);
	}

	void FilesSender::sendFileChunkWithoutEncryption() {
		if (!m_fileStream.is_open()) {
			bool isOpen = openFile();
			if (!isOpen)
				return;
		}

		m_fileStream.read(m_encryptedBuffer.data(), c_encryptedOutputChunkSize);
		std::streamsize bytesRead = m_fileStream.gcount();

		if (bytesRead > 0) {
			asio::async_write(
				m_socket,
				asio::buffer(m_encryptedBuffer.data(), c_encryptedOutputChunkSize),
				[this](std::error_code ec, std::size_t) {
					if (ec) 
					{
						processError(ec);
					}
					else {
						m_totalBytesSent += c_readChunkSize;
						sendFileChunkWithoutEncryption();
					}
				}
			);
		}
		else {
			m_totalBytesSent = 0;
			m_fileStream.close();
			m_metadataPacket.clear();
			m_outgoingFilesQueue.pop_front();
			m_sessionKey = CryptoPP::SecByteBlock{};

			if (!m_outgoingFilesQueue.empty())
			{
				sendMetadata();
			}
		}
	}

	void FilesSender::sendFileChunk() {
		if (!m_fileStream.is_open()) {
			bool isOpen = openFile();
			if (!isOpen)
				return;
		}

		m_fileStream.read(m_readBuffer.data(), c_readChunkSize);
		std::streamsize bytesRead = m_fileStream.gcount();

		if (bytesRead > 0) {
			m_encryptedBuffer = utility::AESEncrypt(m_sessionKey, m_readBuffer);

			asio::async_write(
				m_socket,
				asio::buffer(m_encryptedBuffer.data(), c_encryptedOutputChunkSize),
				[this](std::error_code ec, std::size_t) {
					if (ec) 
					{
						processError(ec);
					}
					else {
						notifySendFileProgress();
						sendFileChunk();
					}
				}
			);
		}
		else {
			m_totalBytesSent = 0;
			m_fileStream.close();
			m_metadataPacket.clear();
			m_outgoingFilesQueue.pop_front();
			m_sessionKey = CryptoPP::SecByteBlock{};

			notifySendFileProgress(true);

			if (!m_outgoingFilesQueue.empty())
			{
				sendMetadata();
			}
		}
	}

	void FilesSender::processError(std::error_code ec) {
		auto file = m_outgoingFilesQueue.pop_front();
		if (auto data = std::get_if<FileTransferData>(&file)) {
			FileLocationInfo fileLocationInfo;
			fileLocationInfo.blobId = data->file->getBlobId();
			fileLocationInfo.fileId = data->file->getId();
			fileLocationInfo.friendUID = data->friendUID;
			m_onSendError(ec, std::move(fileLocationInfo));
		}
	}

	void FilesSender::notifySendFileProgress(bool hundredPercentSent) {
		auto& file = m_outgoingFilesQueue.front_mut();
		auto& data = std::get<FileTransferData>(file);

		FileLocationInfo fileLocationInfo;
		fileLocationInfo.blobId = data.file->getBlobId();
		fileLocationInfo.fileId = data.file->getId();
		fileLocationInfo.friendUID = data.friendUID;

		if (hundredPercentSent) {
			m_onSendProgressUpdate(std::move(fileLocationInfo), 100);
			return;
		}
		else {
			m_totalBytesSent += c_readChunkSize;
		}

		if (m_totalBytesSent < data.file->getFileSize()) {
			const uint32_t fileSize = data.file->getFileSize();
			uint32_t progressPercent = static_cast<uint32_t>(std::min<uint64_t>((m_totalBytesSent * 100) / fileSize, 100));
			m_onSendProgressUpdate(std::move(fileLocationInfo), progressPercent);
		}
	}

	bool FilesSender::openFile() {
		std::error_code ec;

		std::string path;
		auto& file = m_outgoingFilesQueue.front_mut();
		if (auto data = std::get_if<FileTransferData>(&file)) {
			path = data->file->getFilePath();
		}
		else if (auto data = std::get_if<AvatarTransferData>(&file)) {
			path = data->avatar->getPath();
		}

#ifdef _WIN32
		int size_needed = MultiByteToWideChar(CP_UTF8, 0,
			path.c_str(),
			-1, nullptr, 0);
		if (size_needed == 0) {
			std::cerr << "UTF-8 to UTF-16 conversion failed\n";
			return false;
		}

		std::wstring filePath(size_needed, 0);
		MultiByteToWideChar(CP_UTF8, 0,
			path.c_str(),
			-1, &filePath[0], size_needed);

		if (!filePath.empty() && filePath.back() == L'\0')
			filePath.pop_back();
#else
		std::string filePath = path;
#endif

		m_fileStream.open(filePath, std::ios::binary);
		if (!m_fileStream) {
			std::cerr << "Failed to open file\n";
			return false;
		}

		return true;
	}
}