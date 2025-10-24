#include "net_filesReceiver.h"
#include "net_filesConnection.h"
#include "packetType.h"


namespace net {
	FilesReceiver::FilesReceiver(asio::ip::tcp::socket& socket,
		SafeDeque<FileInfo>& incomingFilesQueue,
		std::function<void(AvatarInfo&&)> onAvatar)
		: m_socket(socket),
		m_incomingFilesQueue(incomingFilesQueue),
		m_onAvatar(onAvatar)
	{
		m_currentChunksCount = 0;
		m_expectedChunksCount = 0;
	}

	void FilesReceiver::startReceiving() {
		readMetadataHeader();
	}

	void FilesReceiver::readMetadataHeader() {
		asio::async_read(m_socket, asio::buffer(&m_metadataPacket.header_mut(), Packet::sizeOfHeader()),
			[this](std::error_code ec, std::size_t length) {
				if (ec) {
					if (ec != asio::error::connection_reset && ec != asio::error::operation_aborted) {
						reset();
					}
				}
				else {
					m_metadataPacket.body_mut().resize(m_metadataPacket.size() - Packet::sizeOfHeader());
					if (m_metadataPacket.type() == PacketType::PREPARE_TO_RECEIVE_FILE) {
						m_state = FilesReceiverState::FILE_RECEIVING;
					}
					else if (m_metadataPacket.type() == PacketType::UPDATE_MY_AVATAR) {
						m_state = FilesReceiverState::AVATAR_RECEIVING;
					}

					readMetadataBody();
				}
			});
	}


	void FilesReceiver::readMetadataBody() {
		asio::async_read(m_socket, asio::buffer(m_metadataPacket.body_mut().data(), m_metadataPacket.body().size()),
			[this](std::error_code ec, std::size_t length) {
				if (ec) {
					if (ec != asio::error::connection_reset && ec != asio::error::operation_aborted) {
						reset();
					}
				}
				else {
					if (m_state == FilesReceiverState::FILE_RECEIVING)
					{
						parseFileMetadata();
						openFile();
						readChunk();
					}

					else if (m_state == FilesReceiverState::AVATAR_RECEIVING)
					{
						parseAvatarMetadata();
						openFile();
						readChunk();
					}
				}
			});
	}

	void FilesReceiver::readChunk() {
		asio::async_read(m_socket,
			asio::buffer(m_receiveBuffer.data(), c_receivedChunkSize),
			[this](std::error_code ec, std::size_t bytesTransferred) {
				if (ec) {
					removePartiallyDownloadedFile();
					reset();
				}
				else {
					m_currentChunksCount++;

					m_fileStream.write(m_receiveBuffer.data(), c_receivedChunkSize);

					if (m_currentChunksCount < m_expectedChunksCount) {
						readChunk();
					}
					else {
						finalizeReceiving();
						reset();
					}
				}
			});
	}

	void FilesReceiver::finalizeReceiving() {
		m_fileStream.close();
		m_currentChunksCount = 0;
		m_expectedChunksCount = 0;

		if (m_state == FilesReceiverState::FILE_RECEIVING) {
			m_incomingFilesQueue.push_back(m_fileInfo);
		}
		else {
			m_onAvatar(std::move(m_avatarInfo));
		}

		readMetadataHeader();
	}

	void FilesReceiver::parseAvatarMetadata() {
		try {
			nlohmann::json jsonObject(m_metadataPacket.getData());

			m_avatarInfo.setOwnerUID(jsonObject[MY_UID].get<std::string>());
			m_avatarInfo.setSize(jsonObject[AVATAR_SIZE].get<std::string>());

			std::vector<std::string> userUIDsToBroadcast;
			if (jsonObject.contains(FRIENDS_UIDS) && jsonObject[FRIENDS_UIDS].is_array()) {
				for (const auto& friendUID : jsonObject[FRIENDS_UIDS]) {
					userUIDsToBroadcast.push_back(friendUID.get<std::string>());
				}
			}

			m_avatarInfo.setUserUIDsToBroadcastVec(userUIDsToBroadcast);
			m_avatarInfo.setPath(createAvatarFilePath(m_avatarInfo.getOwnerUID()));

			m_expectedChunksCount = static_cast<int>(std::ceil(static_cast<double>(std::stoi(m_avatarInfo.getSize())) / c_receivedChunkSize));
			int lastChunksSize = std::stoi(m_avatarInfo.getSize()) - ((m_expectedChunksCount - 1) * c_receivedChunkSize);
			if (lastChunksSize <= 0) {
				m_lastChunkSize = c_receivedChunkSize;
			}
			else {
				m_lastChunkSize = lastChunksSize;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "Error parsing avatar metadata JSON: " << e.what() << std::endl;
			m_avatarInfo.reset();
		}
	}

	void FilesReceiver::parseFileMetadata() {
		try {
			nlohmann::json jsonObject(m_metadataPacket.getData());

			m_fileInfo.setEncryptedFileKey(jsonObject[ENCRYPTED_KEY].get<std::string>());
			m_fileInfo.setFileId(jsonObject[FILE_ID].get<std::string>());
			m_fileInfo.setBlobUID(jsonObject[BLOB_ID].get<std::string>());
			m_fileInfo.setReceiverUID(jsonObject[FRIEND_UID].get<std::string>());
			m_fileInfo.setSenderUID(jsonObject[MY_UID].get<std::string>());
			m_fileInfo.setFileSize(jsonObject[FILE_SIZE].get<std::string>());
			m_fileInfo.setFileName(jsonObject[FILE_NAME].get<std::string>());
			m_fileInfo.setTimestamp(jsonObject[TIMESTAMP].get<std::string>());
			m_fileInfo.setFilesInBlobCount(jsonObject[FILES_COUNT_IN_BLOB].get<std::string>());
			m_fileInfo.setFilePath(createFilePath(m_fileInfo.getFileId()));

			if (jsonObject.contains(CAPTION)) {
				m_fileInfo.setCaption(jsonObject[CAPTION].get<std::string>());
			}
			else {
				m_fileInfo.setCaption(std::nullopt);
			}

			m_expectedChunksCount = static_cast<int>(std::ceil(static_cast<double>(std::stoi(m_fileInfo.getFileSize())) / c_decryptedChunkSize));
		}
		catch (const std::exception& e) {
			std::cerr << "Error parsing file metadata JSON: " << e.what() << std::endl;
			m_fileInfo.reset();
		}
	}

	void FilesReceiver::openFile() {
		try {
			std::filesystem::path filePath;
			if (m_state == FilesReceiverState::AVATAR_RECEIVING) {
				filePath = std::filesystem::u8path(m_avatarInfo.getPath());
			}
			else if (m_state == FilesReceiverState::FILE_RECEIVING) {
				filePath = std::filesystem::u8path(m_fileInfo.getFilePath());
			}

			m_fileStream.open(filePath, std::ios::binary | std::ios::trunc);

			if (!m_fileStream.is_open()) {
				std::cerr << "Failed to open file: " << filePath.string() << '\n';
				return;
			}

		}
		catch (const std::exception& e) {
			std::cerr << "Error opening file: " << e.what() << '\n';
			return;
		}
	}

	std::string FilesReceiver::createFilePath(const std::string& fileId) {
		namespace fs = std::filesystem;
		std::string baseName = fileId;
		std::string extension = ".deadlock";

		fs::create_directory("ReceivedFiles");
		std::string filePath = "ReceivedFiles/" + baseName + extension;

		int counter = 1;
		while (fs::exists(filePath)) {
			filePath = "ReceivedFiles/" + baseName + "_" + std::to_string(counter) + extension;
			counter++;
		}

		return filePath;
	}

	std::string FilesReceiver::createAvatarFilePath(const std::string& userLoginHash) {
		namespace fs = std::filesystem;
		std::string baseName = userLoginHash;
		std::string extension = ".dph";

		fs::create_directory("ReceivedPhotos");
		std::string filePath = "ReceivedPhotos/" + baseName + extension;

		return filePath;
	}

	void FilesReceiver::removePartiallyDownloadedFile() {

		std::filesystem::path path;
		if (m_state == FilesReceiverState::AVATAR_RECEIVING) {
			path = std::filesystem::u8path(m_avatarInfo.getPath());
		}
		else if (m_state == FilesReceiverState::FILE_RECEIVING) {
			path = std::filesystem::u8path(m_avatarInfo.getPath());
		}

		if (path.empty()) {
			return;
		}

		m_fileStream.close();
		m_currentChunksCount = 0;
		m_expectedChunksCount = 0;

		std::error_code ec;
		bool removed = std::filesystem::remove(path, ec);

		if (ec) {
			std::cerr << "Failed to delete " << path << ": " << ec.message() << "\n";
		}
	}

	void FilesReceiver::reset() {
		if (m_fileStream.is_open()) {
			m_fileStream.close();
		}
		m_avatarInfo.reset();
		m_fileInfo.reset();
		m_metadataPacket.clear();
		m_state = FilesReceiverState::WAIT;
		m_lastChunkSize = 0;
		m_currentChunksCount = 0;
		m_expectedChunksCount = 0;
	}
}

