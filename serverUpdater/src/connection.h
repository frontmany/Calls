#pragma once

#include <iostream>
#include <string>

#include "safedeque.h"

#include "asio.hpp"

class AvatarInfo;
class FileInfo;

namespace net {
	class Connection : public std::enable_shared_from_this<Connection> {
	public:
		Connection(
			asio::io_context& asioContext,
			asio::ip::tcp::socket socket,
			SafeDeque<FileInfo>& incomingFilesQueue,
			std::function<void(AvatarInfo&&)> onAvatar,
			std::function<void(std::error_code, std::optional<FileInfo>)> onReceiveFileError,
			std::function<void(std::error_code, FileInfo)> onSendFileError,
			std::function<void(FileInfo)> onFileSent,
			std::function<void(std::string)> onDisconnect
		);

		~Connection();

		void sendFile(const FileInfo& file);
		void close();

	private:
		asio::ip::tcp::socket m_socket;
		FilesSender m_filesSender;
		FilesReceiver m_filesReceiver;
		asio::io_context& m_asioContext;
	};
}
