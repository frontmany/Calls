#include "networkController.h"
#include "connection.h"
#include "safeDeque.h"


NetworkController::NetworkController(uint16_t port)
    : m_asioAcceptor(m_context,
        asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) 
{
}

NetworkController::~NetworkController() {
    stop();
}

void NetworkController::removeConnection(ConnectionPtr connection) {
    m_setConnections.erase(connection);
}

bool NetworkController::start() {
    try {
        m_contextThread = std::thread([this]() {m_context.run(); });
        waitForClientConnections();
    }
    catch (std::runtime_error e) {
        std::cout << "[SERVER] Start Error: " << e.what() << "\n";
    }

    std::cout << "[SERVER] Started!\n";
    return true;
}

void NetworkController::stop() {
    m_context.stop();

    if (m_contextThread.joinable())
        m_contextThread.join();

    std::cout << "[SERVER] Stopped!\n";
}

void NetworkController::waitForClientConnections() {
    m_asioAcceptor.async_accept([this](std::error_code ec, asio::ip::tcp::socket socket) {
        if (!ec) {
            std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << "\n";
            ConnectionPtr connection = std::make_shared<Connection>();
        }
        else {
            std::cout << "[SERVER] New Connection Error: " << ec.message() << "\n";
        }

        waitForClientConnections();
    });
}

void NetworkController::update(size_t maxPacketsCount) {
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

void NetworkController::createConnection(uint64_t id, asio::ip::tcp::socket socket)
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

void NetworkController::createFilesConnection(uint64_t id, asio::ip::tcp::socket socket, const std::string& login)
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