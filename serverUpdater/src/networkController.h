#pragma once
#include "net_safeDeque.h"
#include "net_packet.h"
#include "net_fileMetadata.h"
#include "net_connectionTypeResolver.h"
#include "asio.hpp"

#include <unordered_set>

namespace net {
    class FilesConnection;
    class PacketsConnection;

    typedef std::shared_ptr<PacketsConnection> PacketsConnectionPtr;
    typedef std::shared_ptr<FilesConnection> FilesConnectionPtr;

    class NetworkManager {
    public:
        NetworkManager(uint16_t port);
        virtual ~NetworkManager();

        void waitForClientConnections();
        void sendPacket(PacketsConnectionPtr connection, const Packet& msg);
        void sendFile(FilesConnectionPtr filesConnection, const FileMetadata& file);
        void update(size_t maxPacketsCount = std::numeric_limits<unsigned long long>::max());
        void removeConnection(PacketsConnectionPtr connection);

        bool start();
        void stop();

    protected:
        virtual void onPacket(PacketsConnectionPtr connection, Packet& msg) = 0;
        virtual void onFile(FileMetadata file) = 0;
        virtual void onFileSent(FileMetadata sentFile) = 0;
        virtual void bindFilesConnectionToUser(FilesConnectionPtr filesConnection, std::string login) = 0;
        virtual bool isConnectionAllowed(PacketsConnectionPtr connection) = 0;
        virtual void onDisconnect(const std::string& ownerLoginHash) = 0;
        virtual void onSendPacketError(std::error_code ec, net::Packet& msg) = 0;
        virtual void onSendFileError(std::error_code ec, FileMetadata unsentFile) = 0;
        virtual void onReceiveFileError(std::error_code ec, std::optional<FileMetadata> unreadFile) = 0;

    private:
        void createConnection(uint64_t resolverID, asio::ip::tcp::socket socket);
        void createFilesConnection(uint64_t resolverID, asio::ip::tcp::socket socket, const std::string& login);
        void onConnectError(std::error_code ec, uint64_t id);

    private:
        SafeDeque<OwnedPacket> m_safeDequeIncomingPackets;
        SafeDeque<FileMetadata> m_safeDequeIncomingFiles;

        std::unordered_set<PacketsConnectionPtr> m_setConnections;
        std::unordered_map<uint64_t, std::unique_ptr<ConnectionTypeResolver>> m_mapResolvers;

        std::mutex m_mtx;
        asio::io_context m_asioContext;
        std::thread m_contextThread;
        asio::ip::tcp::acceptor m_asioAcceptor;
    };
}