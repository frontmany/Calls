#include "net_networkManager.h"

#include "net_packetsConnection.h"
#include "net_filesConnection.h"
#include "net_generatorID.h"

namespace net {
    NetworkManager::NetworkManager(uint16_t port)
        : m_asioAcceptor(m_asioContext,
            asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) 
    {
    }

    NetworkManager::~NetworkManager() {
        stop();
    }

    void NetworkManager::removeConnection(PacketsConnectionPtr connection) {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_setConnections.erase(connection);
        
    }

    bool NetworkManager::start() {
        try {
            m_contextThread = std::thread([this]() {m_asioContext.run(); });
            waitForClientConnections();
        }
        catch (std::runtime_error e) {
            std::cout << "[SERVER] Start Error: " << e.what() << "\n";
        }

        std::cout << "[SERVER] Started!\n";
        return true;
    }

    void NetworkManager::stop() {
        m_asioContext.stop();

        if (m_contextThread.joinable())
            m_contextThread.join();

        std::cout << "[SERVER] Stopped!\n";
    }

    void NetworkManager::waitForClientConnections() {
        m_asioAcceptor.async_accept([this](std::error_code ec, asio::ip::tcp::socket socket) {
            if (!ec) {
                std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << "\n";

                uint64_t newId = GeneratorID::getID();

                auto resolver = std::make_unique<net::ConnectionTypeResolver>(
                    m_asioContext,
                    std::move(socket),
                    newId,
                    [this](std::error_code ec, uint64_t resolverID) { onConnectError(ec, resolverID); },
                    [this](uint64_t resolverID, asio::ip::tcp::socket socket) {
                        createConnection(resolverID, std::move(socket));
                    },
                    [this](uint64_t resolverID, asio::ip::tcp::socket socket, std::string login) {
                        createFilesConnection(resolverID,std::move(socket), login);
                    }
                );

                m_mapResolvers.emplace(newId, std::move(resolver));
            }
            else {
                std::cout << "[SERVER] New Connection Error: " << ec.message() << "\n";
            }

            waitForClientConnections();
        });
    }

    void NetworkManager::sendPacket(PacketsConnectionPtr connection, const Packet& packet) {
        connection->send(packet);
    }

    void NetworkManager::sendFile(FilesConnectionPtr filesConnection, const FileMetadata& file) {
        filesConnection->sendFile(file);
    }

    void NetworkManager::update(size_t maxPacketsCount) {
        size_t processedPackets = 0;

        while (true) {
            if (!m_safeDequeIncomingFiles.empty()) {
                FileMetadata file = m_safeDequeIncomingFiles.pop_front();
                onFile(std::move(file));
            }

            if (!m_safeDequeIncomingPackets.empty() && processedPackets < maxPacketsCount) {
                OwnedPacket ownedPacket = m_safeDequeIncomingPackets.pop_front();
                onPacket(ownedPacket.relatedConnection, ownedPacket.packet);
                processedPackets++;
            }

            std::this_thread::yield();
        }
    }

    void NetworkManager::createConnection(uint64_t id, asio::ip::tcp::socket socket)
    {
        PacketsConnectionPtr newPacketsConnection = std::make_shared<PacketsConnection>(
            m_asioContext,
            std::move(socket),
            m_safeDequeIncomingPackets,
            [this](std::error_code ec, Packet unsentPacket) { onSendPacketError(ec, unsentPacket); },
            [this](std::string ownerLoginHash) { onDisconnect(ownerLoginHash); }
        );

        if (m_mapResolvers.contains(id)) {
            m_mapResolvers.erase(id);
        }

        if (isConnectionAllowed(newPacketsConnection)) {
            std::lock_guard<std::mutex> lock(m_mtx);
            m_setConnections.insert(newPacketsConnection);
        }
        else {
            std::cout << "[-----] Connection Denied\n";
        }
    }

    void NetworkManager::createFilesConnection(uint64_t id, asio::ip::tcp::socket socket, const std::string& login)
    {
        FilesConnectionPtr newFilesConnection = std::make_shared<FilesConnection>(
            m_asioContext,
            std::move(socket),
            m_safeDequeIncomingFiles,
            [this](std::error_code ec, std::optional<FileMetadata> unreceivedFile) { onReceiveFileError(ec, unreceivedFile); },
            [this](std::error_code ec, FileMetadata unsentFile) { onSendFileError(ec, unsentFile); },
            [this](FileMetadata file) { onFileSent(file); },
            [this](std::string ownerLoginHash) { onDisconnect(ownerLoginHash); }
        );

        if (m_mapResolvers.contains(id)) {
            m_mapResolvers.erase(id);
        }

        bindFilesConnectionToUser(newFilesConnection, login);
    }

    void NetworkManager::onConnectError(std::error_code ec, uint64_t id) {
        if (m_mapResolvers.contains(id)) {
            m_mapResolvers.erase(id);
            std::cout << "[-----] Connection wasn't established " << "\n";
        }
        else {
            std::cout << "[-----] Resolver not found in map by id: " << id << "\n";
        }
    }
}