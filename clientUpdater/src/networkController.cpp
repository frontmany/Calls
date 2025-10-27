#include "networkController.h"
#include "operationSystemType.h"
#include "utility.h"
#include "jsonTypes.h"

#include "json.hpp"

namespace updater {

NetworkController::NetworkController(
	std::function<void(CheckResult)> onCheckResult,
	std::function<void()> onAllFilesLoaded,
	std::function<void()> onError)
	: m_onCheckResult(onCheckResult),
	m_onAllFilesLoaded(onAllFilesLoaded),
	m_socket(m_context),
	m_onError(onError),
	m_currentChunksCount(0)
{

}

void NetworkController::sendPacket(const Packet& packet) {
	writeHeader(packet);
}

void NetworkController::connect(const std::string& host, const std::string& port) {
	m_asioThread = std::thread([this]() { m_context.run(); });
	createConnection(host, std::stoi(port));
}

void NetworkController::disconnect() {
	if (m_socket.is_open()) {
		std::error_code ec;
		m_socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
		m_socket.close(ec);
	}

	if (m_fileStream.is_open()) {
		m_fileStream.close();
	}

	reset();
}


void NetworkController::createConnection(const std::string& host, const uint16_t port) {
	try {
		asio::ip::tcp::resolver resolver(m_context);
		auto serverEndpoint = resolver.resolve(host, std::to_string(port));

		asio::async_connect(m_socket, serverEndpoint,
			[this](std::error_code ec, const asio::ip::tcp::endpoint& endpoint) {
				if (ec) {
					m_onError();
				}
				else {
					readHandshake();
				}
			});
	}
	catch (std::exception& e) {
		m_onError();
	}
}

void NetworkController::writeHeader(const Packet& packet) {
	asio::async_write(
		m_socket,
		asio::buffer(&packet.header(), Packet::sizeOfHeader()),
		[this, packet](std::error_code ec, std::size_t length) {
			if (ec)
				m_onError();
			else
				writeBody(packet);
		});
}

void NetworkController::writeBody(const Packet& packet) {
	asio::async_write(
		m_socket,
		asio::buffer(packet.body().data(), packet.body().size()),
		[this](std::error_code ec, std::size_t length) {
			if (ec) {
				m_onError();
			}
		}
	);
}

void NetworkController::readHandshake() {
	asio::async_read(m_socket, asio::buffer(&m_handshakeIn, sizeof(uint64_t)),
		[this](std::error_code ec, std::size_t length) {
			if (ec) {
				m_onError();
			}
			else {
				m_handshakeOut = scramble(m_handshakeIn);
				writeHandshake();
			}
		});
}

void NetworkController::writeHandshake() {
	asio::async_write(m_socket, asio::buffer(&m_handshakeOut, sizeof(uint64_t)),
		[this](std::error_code ec, std::size_t length) {
			if (ec) {
				m_onError();
			}
			else {
				readCheckResult();
			}
		});
}

void NetworkController::readCheckResult() {
	asio::async_read(m_socket, asio::buffer(&m_metadata.header_mut(), Packet::sizeOfHeader()),
		[this](std::error_code ec, std::size_t length) {
			if (ec) {
				m_onError();
			}
			else {
				if (m_metadata.header().type == static_cast<int>(CheckResult::POSSIBLE_UPDATE))
					m_onCheckResult(CheckResult::POSSIBLE_UPDATE);
				else if (m_metadata.header().type == static_cast<int>(CheckResult::REQUIRED_UPDATE))
					m_onCheckResult(CheckResult::REQUIRED_UPDATE);
				else
					m_onError();

				readMetadataHeader();
			}
		});
}

void NetworkController::readMetadataHeader() {
	asio::async_read(m_socket, asio::buffer(&m_metadata.header_mut(), Packet::sizeOfHeader()),
		[this](std::error_code ec, std::size_t length) {
			if (ec) {
				m_onError();
			}
			else {
				m_metadata.body_mut().resize(m_metadata.size() - Packet::sizeOfHeader());
				readMetadataBody();
			}
		});
}

void NetworkController::readMetadataBody() {
	asio::async_read(m_socket, asio::buffer(m_metadata.body_mut().data(), m_metadata.body().size()),
		[this](std::error_code ec, std::size_t length) {
			if (ec) {
				m_onError();
			}
			else {
				parseMetadata();
				openFile();
				readChunk();
			}
		});
}

uint64_t NetworkController::scramble(uint64_t inputNumber) {
	uint64_t out = inputNumber ^ 0xDEADBEEFC;
	out = (out & 0xF0F0F0F0F) >> 4 | (out & 0x0F0F0F0F0F) << 4;
	return out ^ 0xC0DEFACE12345678;
}

void NetworkController::parseMetadata() {
	nlohmann::json jsonObject(std::move(m_metadata.getData()));

	try {
		if (jsonObject.contains(VERSION)) {
			std::filesystem::path versionsDir = "versions";
			if (!std::filesystem::exists(versionsDir)) {
				std::filesystem::create_directories(versionsDir);
			}

			std::ofstream outFile("config.json");
			if (outFile.is_open()) {
				nlohmann::json versionJson;
				versionJson[VERSION] = jsonObject[VERSION].get<std::string>();
				outFile << versionJson.dump(4);
				outFile.close();
			}
		}

		if (jsonObject.contains(FILES_TO_DOWNLOAD)) {
			for (const auto& fileEntry : jsonObject[FILES_TO_DOWNLOAD]) {
				if (fileEntry.contains(RELATIVE_FILE_PATH) && fileEntry.contains(FILE_SIZE) && fileEntry.contains(FILE_HASH)) {
					std::string relativeFilePath = fileEntry[RELATIVE_FILE_PATH].get<std::string>();
					std::string fileHash = fileEntry[FILE_HASH].get<std::string>();
					uint64_t fileSize = fileEntry[FILE_SIZE].get<uint64_t>();

					int chunksCount = static_cast<int>(std::ceil(static_cast<double>(fileSize) / c_chunkSize));
					uint64_t lastChunkSize = fileSize % c_chunkSize;

					if (lastChunkSize == 0) {
						lastChunkSize = c_chunkSize;
					}

					FileMetadata fileInfo;
					fileInfo.fileHash = fileHash;
					fileInfo.expectedChunksCount = chunksCount;
					fileInfo.relativeFilePath = relativeFilePath;
					fileInfo.lastChunkSize = lastChunkSize;

					m_expectedFiles.push(std::move(fileInfo));
				}
				else {
					m_onError();
					return;
				}
			}
		}

		if (jsonObject.contains(FILES_TO_DELETE)) {
			for (const auto& filePath : jsonObject[FILES_TO_DELETE]) {
				if (filePath.is_string()) {
					std::filesystem::path pathToDelete = filePath.get<std::string>();
					m_filesToDelete.push_back(std::move(pathToDelete));
				}
				else {
					m_onError();
					return;
				}
			}
		}

		m_currentChunksCount = 0;
	}
	catch (const nlohmann::json::exception& e) {
		m_onError();
	}
}

void NetworkController::readChunk() {
	asio::async_read(m_socket,
		asio::buffer(m_receiveBuffer.data(), c_chunkSize),
		[this](std::error_code ec, std::size_t bytesTransferred) {
			if (ec) {
				reset();
				deleteReceived();
			}
			else {
				m_currentChunksCount++;
				if (m_currentChunksCount < m_expectedFiles.front().expectedChunksCount) {
					m_fileStream.write(m_receiveBuffer.data(), c_chunkSize);
					readChunk();
				}
				else if (m_currentChunksCount == m_expectedFiles.front().expectedChunksCount) {
					m_fileStream.write(m_receiveBuffer.data(), m_expectedFiles.front().lastChunkSize);

					if (m_expectedFiles.empty()) {
						finalizeReceiving();
					}
					else {
						m_currentChunksCount = 0;
						m_fileStream.close();

						if (m_expectedFiles.front().fileHash != calculateFileHash(m_expectedFiles.front().relativeFilePath)) {
							m_expectedFiles.pop();
							m_onError();
						}
						else {
							m_expectedFiles.pop();
							openFile();
							readChunk();
						}
					}
				}
			}
		});
}

void NetworkController::finalizeReceiving() {
	reset();

	std::filesystem::path versionsDir = "versions";
	if (!std::filesystem::exists(versionsDir)) {
		std::filesystem::create_directories(versionsDir);
	}

	nlohmann::json removeJson;
	nlohmann::json filesArray = nlohmann::json::array();

	for (const auto& filePath : m_filesToDelete) {
		filesArray.push_back(filePath.string());
	}

	removeJson[FILES] = filesArray;

	std::filesystem::path removeJsonPath = versionsDir / "remove.json";
	std::ofstream file(removeJsonPath);
	if (file) {
		file << std::setw(4) << removeJson << std::endl;
		file.close();
	}

	m_onAllFilesLoaded();
}

void NetworkController::openFile() {
	try {
		const std::filesystem::path tempDirectory = "update_temp";
		std::filesystem::remove_all(tempDirectory);
		std::filesystem::create_directories(tempDirectory);

		if (m_expectedFiles.empty()) return;

		std::filesystem::path filePath = tempDirectory / m_expectedFiles.front().relativeFilePath;
		std::filesystem::create_directories(filePath.parent_path());

		m_fileStream.open(filePath, std::ios::binary | std::ios::trunc);

		if (!m_fileStream.is_open()) {
			m_onError();
		}
	}
	catch (const std::exception& e) {
		m_onError();
		return;
	}
}

void NetworkController::reset() {
	if (m_fileStream.is_open()) {
		m_fileStream.close();
	}
	m_metadata.clear();

	m_context.stop();
	if (m_asioThread.joinable()) {
		m_asioThread.join();
	}

	std::queue<FileMetadata> emptyQueue;
	m_expectedFiles.swap(emptyQueue);

	m_currentChunksCount = 0;

	m_handshakeOut = 0;
	m_handshakeIn = 0;
}

void NetworkController::deleteReceived() {
	const std::filesystem::path tempDirectory = "update_temp";
	std::filesystem::remove_all(tempDirectory);
	std::filesystem::remove(tempDirectory);
}
}