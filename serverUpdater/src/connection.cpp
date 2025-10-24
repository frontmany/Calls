#include "connection.h"


namespace net {
	Connection::Connection(
		asio::io_context& asioContext,
		asio::ip::tcp::socket socket,
		SafeDeque<FileInfo>& incomingFilesQueue,
		std::function<void(AvatarInfo&&)> onAvatar,
		std::function<void(std::error_code, std::optional<FileInfo>)> onReceiveFileError,
		std::function<void(std::error_code, FileInfo)> onSendFileError,
		std::function<void(FileInfo)> onFileSent,
		std::function<void(std::string)> onDisconnect)
		: m_asioContext(asioContext),
		m_socket(std::move(socket)),
		m_filesSender(this, asioContext, m_socket, onFileSent, onSendFileError),
		m_filesReceiver(this, m_socket, incomingFilesQueue, onAvatar)
	{
		m_filesReceiver.startReceiving();
	}

	Connection::~Connection() 
	{
	}

	void Connection::sendFile(const FileInfo& file) {
		m_filesSender.sendFile(file);
	}

    void Connection::close() {
        auto self = shared_from_this();

        asio::post(m_asioContext,
            [this, self]() {
                if (m_socket.is_open()) {
                    try {
                        std::error_code ec;

                        m_socket.close(ec);
                        if (ec) {
                            std::cerr << "Socket close error: " << ec.message() << "\n";
                        }
						else {
							std::cout << "files Connection closed successfully\n";
						}
                    }
                    catch (const std::exception& e) {
                        std::cerr << "Exception in Connection::close: " << e.what() << "\n";
                    }
                }
            });
    }
}