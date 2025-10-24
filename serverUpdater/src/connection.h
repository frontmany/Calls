#pragma once

#include "net_filesSender.h"
#include "net_filesReceiver.h"   

#include "asio.hpp"

#include <iostream>
#include <string>

class AvatarInfo;
class FileInfo;

namespace net {
	class FilesConnection : public std::enable_shared_from_this<FilesConnection> {
	public:
		FilesConnection(
			asio::io_context& asioContext,
			asio::ip::tcp::socket socket,
			SafeDeque<FileInfo>& incomingFilesQueue,
			std::function<void(AvatarInfo&&)> onAvatar,
			std::function<void(std::error_code, std::optional<FileInfo>)> onReceiveFileError,
			std::function<void(std::error_code, FileInfo)> onSendFileError,
			std::function<void(FileInfo)> onFileSent,
			std::function<void(std::string)> onDisconnect
		);

		~FilesConnection();

		void sendFile(const FileInfo& file);
		void close();

	private:
		asio::ip::tcp::socket m_socket;
		FilesSender m_filesSender;
		FilesReceiver m_filesReceiver;
		asio::io_context& m_asioContext;
	};
}
