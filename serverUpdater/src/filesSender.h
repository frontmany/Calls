#pragma once

#include <memory>
#include <fstream>

#include "packetType.h"
#include "utility.h"

#include "net_safeDeque.h"
#include "net_fileTransferData.h"
#include "net_avatarTransferData.h"
#include "net_packet.h"


#include "asio.hpp"
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

class File;
class FileLocationInfo;

namespace net 
{
	class FilesSender {
	public:
		FilesSender(asio::io_context& asioContext,
			asio::ip::tcp::socket& socket,
			std::function<void(FileLocationInfo&&, uint32_t)> onSendProgress,
			std::function<void()> onAvatarSent,
			std::function<void(std::error_code ec, FileLocationInfo&&)> onSendError
		);

		void sendFile(const std::variant<FileTransferData, AvatarTransferData>& file);

	private:
		void notifySendFileProgress(bool hundredPercentSent = false);
		void processError(std::error_code ec);
		void sendMetadata();
		void sendFileChunkWithoutEncryption();
		void sendFileChunk();
		bool openFile();

	private:
		static constexpr uint32_t c_readChunkSize = 8192;
		static constexpr uint32_t c_encryptedOutputChunkSize = 8220;

		asio::ip::tcp::socket& m_socket;
		SafeDeque<std::variant<FileTransferData, AvatarTransferData>> m_outgoingFilesQueue;

		std::array<char, c_readChunkSize> m_readBuffer{};
		std::array<char, c_encryptedOutputChunkSize> m_encryptedBuffer{};
		std::ifstream m_fileStream;
		uint64_t m_totalBytesSent;
		CryptoPP::SecByteBlock m_sessionKey;
		Packet m_metadataPacket;

		std::function<void(FileLocationInfo&&, uint32_t)> m_onSendProgressUpdate;
		std::function<void()> m_onAvatarSent;
		std::function<void(std::error_code, FileLocationInfo&&)> m_onSendError;
		std::function<void()> m_onAllFilesSent;
		asio::io_context& m_asioContext;
	};
}