#include "server.h"
#include "logic/packetFactory.h"
#include "constants/jsonType.h"
#include "utilities/crypto.h"
#include "utilities/logger.h"
#include "models/pendingCall.h"
#include "network/tcp/packet.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <string>

using namespace server;
using namespace server::constant;
using namespace server::utilities;

namespace
{
    std::vector<unsigned char> toBytes(const std::string& s) {
        return std::vector<unsigned char>(s.begin(), s.end());
    }
}

namespace server
{
    Server::Server(const std::string& tcpPort, const std::string& udpPort)
        : m_networkController(
            static_cast<uint16_t>(std::stoul(tcpPort)),
            udpPort,
            [this](network::tcp::OwnedPacket&& packet) {handleReceiveTcp(std::move(packet)); },
            [this](network::tcp::ConnectionPtr connection) {handleConnectionWithUserDown(connection); },
            [this](const unsigned char* data, int size, uint32_t type, const asio::ip::udp::endpoint& ep) {handleReceiveUdp(data, size, type, ep);})
    {
        registerHandlers();
    }

    void Server::registerHandlers() {
        m_packetHandlers.emplace(PacketType::AUTHORIZATION, [this](const nlohmann::json& json, network::tcp::ConnectionPtr conn) { handleAuthorization(json, conn); });
        m_packetHandlers.emplace(PacketType::RECONNECT, [this](const nlohmann::json& json, network::tcp::ConnectionPtr conn) { handleReconnect(json, conn); });
        m_packetHandlers.emplace(PacketType::LOGOUT, [this](const nlohmann::json& json, network::tcp::ConnectionPtr conn) { handleLogout(json, conn); });
        m_packetHandlers.emplace(PacketType::GET_USER_INFO, [this](const nlohmann::json& json, network::tcp::ConnectionPtr conn) { handleGetFriendInfo(json, conn); });
        m_packetHandlers.emplace(PacketType::CALLING_BEGIN, [this](const nlohmann::json& json, network::tcp::ConnectionPtr conn) { handleStartOutgoingCall(json, conn); });
        m_packetHandlers.emplace(PacketType::CALLING_END, [this](const nlohmann::json& json, network::tcp::ConnectionPtr conn) { handleStopOutgoingCall(json, conn); });
        m_packetHandlers.emplace(PacketType::CALL_ACCEPT, [this](const nlohmann::json& json, network::tcp::ConnectionPtr conn) { handleAcceptCall(json, conn); });
        m_packetHandlers.emplace(PacketType::CALL_DECLINE, [this](const nlohmann::json& json, network::tcp::ConnectionPtr conn) { handleDeclineCall(json, conn); });
        m_packetHandlers.emplace(PacketType::CALL_END, [this](const nlohmann::json& json, network::tcp::ConnectionPtr conn) { handleEndCall(json, conn); });
        m_packetHandlers.emplace(PacketType::SCREEN_SHARING_BEGIN, [this](const nlohmann::json& json, network::tcp::ConnectionPtr conn) { redirectPacket(json, PacketType::SCREEN_SHARING_BEGIN, conn); });
        m_packetHandlers.emplace(PacketType::SCREEN_SHARING_END, [this](const nlohmann::json& json, network::tcp::ConnectionPtr conn) { redirectPacket(json, PacketType::SCREEN_SHARING_END, conn); });
        m_packetHandlers.emplace(PacketType::CAMERA_SHARING_BEGIN, [this](const nlohmann::json& json, network::tcp::ConnectionPtr conn) { redirectPacket(json, PacketType::CAMERA_SHARING_BEGIN, conn); });
        m_packetHandlers.emplace(PacketType::CAMERA_SHARING_END, [this](const nlohmann::json& json, network::tcp::ConnectionPtr conn) { redirectPacket(json, PacketType::CAMERA_SHARING_END, conn); });
    }

    void Server::run() {
        m_networkController.start();
    }

    void Server::stop() {
        m_networkController.stop();
    }

    void Server::handleReceiveUdp(const unsigned char* data, int size, uint32_t rawType, const asio::ip::udp::endpoint& endpointFrom) {
        PacketType type = static_cast<PacketType>(rawType);
        if (type != PacketType::VOICE && type != PacketType::SCREEN && type != PacketType::CAMERA)
            return;

        UserPtr sender = m_userRepository.findUserByEndpoint(endpointFrom);
        if (!sender) {
            LOG_DEBUG("[UDP] Media from unknown endpoint {}:{}", endpointFrom.address().to_string(), endpointFrom.port());
            return;
        }

        UserPtr partner = sender->getCallPartner();
        if (!partner || !m_userRepository.containsUser(partner->getNicknameHash()))
            return;
        if (partner->isConnectionDown())
            return;

        m_networkController.sendUdp(data, size, rawType, partner->getEndpoint());
    }

    void Server::handleReceiveTcp(network::tcp::OwnedPacket&& owned) {
        network::tcp::ConnectionPtr conn = owned.connection;
        network::tcp::Packet& p = owned.packet;
        uint32_t rawType = p.type;

        if (p.body.empty()) {
            LOG_WARN("[TCP] Control packet type {} has empty body", rawType);
            return;
        }

        std::string jsonStr(p.body.begin(), p.body.end());
        nlohmann::json json;
        try {
            json = nlohmann::json::parse(jsonStr);
        }
        catch (const std::exception& e) {
            LOG_ERROR("[TCP] Invalid JSON in control packet type {}: {}", rawType, e.what());
            return;
        }

        PacketType type = static_cast<PacketType>(rawType);
        auto it = m_packetHandlers.find(type);
        if (it != m_packetHandlers.end())
            it->second(json, conn);
        else
            LOG_WARN("[TCP] Unknown control packet type {}", rawType);
    }

    void Server::handleConnectionWithUserDown(network::tcp::ConnectionPtr conn) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_connToUser.find(conn);
        if (it == m_connToUser.end()) {
            LOG_DEBUG("[TCP] Disconnect: no user associated with connection (e.g. pre-auth or already reconnected)");
            return;
        }
        UserPtr user = std::move(it->second);
        m_connToUser.erase(it);
        std::string prefix = user->getNicknameHash().length() >= 5 ? user->getNicknameHash().substr(0, 5) : user->getNicknameHash();
        LOG_INFO("[TCP] Connection down with user {}", prefix);
        user->setConnectionDown(true);
        user->clearTcpConnection();
        processConnectionDown(user);
    }

    void Server::sendTcp(network::tcp::ConnectionPtr conn, uint32_t type, const std::vector<unsigned char>& body) {
        if (!conn) return;
        network::tcp::Packet packet;
        packet.type = type;
        packet.body = body;
        conn->send(packet);
    }

    void Server::sendTcpToUser(const std::string& receiverNicknameHash, uint32_t type, const std::string& jsonBody) {
        auto user = m_userRepository.findUserByNickname(receiverNicknameHash);
        if (!user) return;
        auto conn = user->getTcpConnection();
        if (!conn) return;
        sendTcp(conn, type, toBytes(jsonBody));
    }

    bool Server::sendTcpToUserIfConnected(const std::string& receiverNicknameHash, uint32_t type, const std::vector<unsigned char>& body) {
        auto user = m_userRepository.findUserByNickname(receiverNicknameHash);
        if (!user || user->isConnectionDown()) return false;
        auto conn = user->getTcpConnection();
        if (!conn) return false;
        sendTcp(conn, type, body);
        return true;
    }

    void Server::handleAuthorization(const nlohmann::json& json, network::tcp::ConnectionPtr conn) {
        std::lock_guard<std::mutex> lock(m_mutex);
        try {
            std::string uid = json[UID].get<std::string>();
            std::string nicknameHash = json[SENDER_NICKNAME_HASH].get<std::string>();
            CryptoPP::RSA::PublicKey publicKey = crypto::deserializePublicKey(json[PUBLIC_KEY]);

            uint16_t udpPort = 0;
            if (json.contains(UDP_PORT) && json[UDP_PORT].is_number_unsigned())
                udpPort = static_cast<uint16_t>(json[UDP_PORT].get<uint32_t>());

            asio::ip::udp::endpoint udpEndpoint;
            if (udpPort != 0) {
                auto tcpEp = conn->remoteEndpoint();
                if (!tcpEp.address().is_unspecified())
                    udpEndpoint = asio::ip::udp::endpoint(tcpEp.address(), udpPort);
            }

            bool authorized = false;
            std::string token;

            if (m_userRepository.containsUser(nicknameHash)) {
                std::string prefix = nicknameHash.length() >= 5 ? nicknameHash.substr(0, 5) : nicknameHash;
                LOG_WARN("Authorization failed - nickname already taken: {}", prefix);
            }
            else {
                token = crypto::generateUID();
                UserPtr user = std::make_shared<User>(nicknameHash, token, publicKey, udpEndpoint,
                    [this, nicknameHash]() {
                        std::lock_guard<std::mutex> lock(m_mutex);
                        auto u = m_userRepository.findUserByNickname(nicknameHash);
                        if (u) processUserLogout(u);
                    });
                user->setTcpConnection(conn);
                m_userRepository.addUser(user);
                m_connToUser[conn] = user;
                authorized = true;
                std::string prefix = nicknameHash.length() >= 5 ? nicknameHash.substr(0, 5) : nicknameHash;
                LOG_INFO("User authorized: {}", prefix);
            }

            std::optional<std::string> encryptedNickname;
            std::optional<std::string> packetKey;
            if (authorized && json.contains(ENCRYPTED_NICKNAME) && json[ENCRYPTED_NICKNAME].is_string())
                encryptedNickname = json[ENCRYPTED_NICKNAME].get<std::string>();
            if (authorized && json.contains(PACKET_KEY) && json[PACKET_KEY].is_string())
                packetKey = json[PACKET_KEY].get<std::string>();

            std::vector<unsigned char> packet;
            if (authorized)
                packet = PacketFactory::getAuthorizationResultPacket(true, uid, nicknameHash, token, encryptedNickname, packetKey);
            else
                packet = PacketFactory::getAuthorizationResultPacket(false, uid, nicknameHash);
            sendTcp(conn, static_cast<uint32_t>(PacketType::AUTHORIZATION_RESULT), packet);
        }
        catch (const std::exception& e) {
            LOG_ERROR("Authorization error: {}", e.what());
        }
    }

    void Server::handleReconnect(const nlohmann::json& json, network::tcp::ConnectionPtr conn) {
        std::lock_guard<std::mutex> lock(m_mutex);
        try {
            std::string uid = json[UID].get<std::string>();
            std::string token = json[TOKEN].get<std::string>();
            std::string senderNicknameHash = json[SENDER_NICKNAME_HASH].get<std::string>();
            std::string prefix = senderNicknameHash.length() >= 5 ? senderNicknameHash.substr(0, 5) : senderNicknameHash;

            uint16_t udpPort = 0;
            if (json.contains(UDP_PORT) && json[UDP_PORT].is_number_unsigned())
                udpPort = static_cast<uint16_t>(json[UDP_PORT].get<uint32_t>());

            auto user = m_userRepository.findUserByNickname(senderNicknameHash);
            if (!user) {
                LOG_INFO("[TCP] Reconnect: user {} not found (logged out or never authorized)", prefix);
                auto packet = PacketFactory::getReconnectionResultPacket(false, uid, senderNicknameHash, token);
                sendTcp(conn, static_cast<uint32_t>(PacketType::RECONNECT_RESULT), packet);
                return;
            }

            bool allowed = (user->getToken() == token);
            if (!allowed)
                LOG_WARN("[TCP] Reconnect: token mismatch for user {}", prefix);
            if (allowed) {
                auto oldConn = user->getTcpConnection();
                if (oldConn)
                    m_connToUser.erase(oldConn);
                user->setConnectionDown(false);
                user->setTcpConnection(conn);
                m_connToUser[conn] = user;
                if (udpPort != 0) {
                    auto tcpEp = conn->remoteEndpoint();
                    if (!tcpEp.address().is_unspecified()) {
                        asio::ip::udp::endpoint udpEp(tcpEp.address(), udpPort);
                        m_userRepository.updateUserUdpEndpoint(senderNicknameHash, udpEp);
                    }
                }
            }

            std::vector<unsigned char> packet = PacketFactory::getReconnectionResultPacket(
                allowed, uid, senderNicknameHash, token, allowed && user->isInCall());
            sendTcp(conn, static_cast<uint32_t>(PacketType::RECONNECT_RESULT), packet);

            if (allowed) {
                LOG_INFO("[TCP] Reconnect OK: user {}", prefix);
                auto [_, restoredPacket] = PacketFactory::getConnectionRestoredWithUserPacket(user->getNicknameHash());
                sendTcp(conn, static_cast<uint32_t>(PacketType::CONNECTION_RESTORED_WITH_USER), restoredPacket);
            }

            if (allowed && user->isInCall()) {
                auto partner = user->getCallPartner();
                if (partner && m_userRepository.containsUser(partner->getNicknameHash())) {
                    auto partnerInRepo = m_userRepository.findUserByNickname(partner->getNicknameHash());
                    if (!partnerInRepo->isConnectionDown()) {
                        auto [_, partnerPacket] = PacketFactory::getConnectionRestoredWithUserPacket(user->getNicknameHash());
                        sendTcpToUserIfConnected(partner->getNicknameHash(),
                            static_cast<uint32_t>(PacketType::CONNECTION_RESTORED_WITH_USER), partnerPacket);
                    }
                }
            }
        }
        catch (const std::exception& e) {
            LOG_ERROR("Reconnect error: {}", e.what());
        }
    }

    void Server::handleLogout(const nlohmann::json& json, network::tcp::ConnectionPtr conn) {
        std::lock_guard<std::mutex> lock(m_mutex);
        try {
            std::string senderNicknameHash = json[SENDER_NICKNAME_HASH].get<std::string>();
            auto user = m_userRepository.findUserByNickname(senderNicknameHash);
            if (user)
                processUserLogout(user);
        }
        catch (const std::exception& e) {
            LOG_ERROR("Logout error: {}", e.what());
        }
    }

    void Server::handleGetFriendInfo(const nlohmann::json& json, network::tcp::ConnectionPtr conn) {
        std::lock_guard<std::mutex> lock(m_mutex);
        try {
            std::string uid = json[UID].get<std::string>();
            std::string nicknameHash = json[NICKNAME_HASH].get<std::string>();
            std::string senderNicknameHash = json[SENDER_NICKNAME_HASH].get<std::string>();

            auto requested = m_userRepository.findUserByNickname(nicknameHash);
            bool senderExists = m_userRepository.containsUser(senderNicknameHash);
            std::optional<std::string> encryptedNickname;
            std::optional<std::string> packetKey;
            if (requested && senderExists && json.contains(ENCRYPTED_NICKNAME) && json[ENCRYPTED_NICKNAME].is_string())
                encryptedNickname = json[ENCRYPTED_NICKNAME].get<std::string>();
            if (requested && senderExists && json.contains(PACKET_KEY) && json[PACKET_KEY].is_string())
                packetKey = json[PACKET_KEY].get<std::string>();
            std::vector<unsigned char> packet;
            if (requested && senderExists)
                packet = PacketFactory::getUserInfoResultPacket(true, uid, requested->getNicknameHash(), requested->getPublicKey(), encryptedNickname, packetKey);
            else
                packet = PacketFactory::getUserInfoResultPacket(false, uid, nicknameHash);
            sendTcp(conn, static_cast<uint32_t>(PacketType::GET_USER_INFO_RESULT), packet);
        }
        catch (const std::exception& e) {
            LOG_ERROR("Get user info error: {}", e.what());
        }
    }

    void Server::handleStartOutgoingCall(const nlohmann::json& json, network::tcp::ConnectionPtr conn) {
        std::lock_guard<std::mutex> lock(m_mutex);
        try {
            std::string uid = json[UID].get<std::string>();
            std::string receiverNicknameHash = json[RECEIVER_NICKNAME_HASH].get<std::string>();
            std::string senderNicknameHash = json[SENDER_NICKNAME_HASH].get<std::string>();

            auto receiver = m_userRepository.findUserByNickname(receiverNicknameHash);
            auto sender = m_userRepository.findUserByNickname(senderNicknameHash);
            if (!receiver || !sender) return;
            if (sender->getTcpConnection() != conn) return;

            std::string jsonStr = json.dump();
            std::vector<unsigned char> body = toBytes(jsonStr);
            if (sendTcpToUserIfConnected(receiverNicknameHash, static_cast<uint32_t>(PacketType::CALLING_BEGIN), body)) {
                auto pending = std::make_shared<PendingCall>(sender, receiver, [this, receiver, sender]() {
                    auto out = sender->getOutgoingPendingCall();
                    if (out && out->getReceiver()->getNicknameHash() == receiver->getNicknameHash()) {
                        resetOutgoingPendingCall(sender);
                        removeIncomingPendingCall(receiver, out);
                    }
                });
                m_callManager.addPendingCall(pending);
                sender->setOutgoingPendingCall(pending);
                receiver->addIncomingPendingCall(pending);
                std::string sp = senderNicknameHash.length() >= 5 ? senderNicknameHash.substr(0, 5) : senderNicknameHash;
                std::string rp = receiverNicknameHash.length() >= 5 ? receiverNicknameHash.substr(0, 5) : receiverNicknameHash;
                LOG_INFO("Call initiated from {} to {}", sp, rp);
            }
        }
        catch (const std::exception& e) {
            LOG_ERROR("Start outgoing call error: {}", e.what());
        }
    }

    void Server::handleStopOutgoingCall(const nlohmann::json& json, network::tcp::ConnectionPtr conn) {
        std::lock_guard<std::mutex> lock(m_mutex);
        try {
            std::string receiverNicknameHash = json[RECEIVER_NICKNAME_HASH].get<std::string>();
            std::string senderNicknameHash = json[SENDER_NICKNAME_HASH].get<std::string>();

            auto receiver = m_userRepository.findUserByNickname(receiverNicknameHash);
            auto sender = m_userRepository.findUserByNickname(senderNicknameHash);
            if (!sender || sender->getTcpConnection() != conn) return;

            std::vector<unsigned char> body = toBytes(json.dump());
            if (receiver && !receiver->isConnectionDown())
                sendTcpToUserIfConnected(receiverNicknameHash, static_cast<uint32_t>(PacketType::CALLING_END), body);

            if (receiver && sender) {
                auto out = sender->getOutgoingPendingCall();
                if (out && out->getReceiver()->getNicknameHash() == receiverNicknameHash) {
                    resetOutgoingPendingCall(sender);
                    removeIncomingPendingCall(receiver, out);
                    std::string sp = senderNicknameHash.length() >= 5 ? senderNicknameHash.substr(0, 5) : senderNicknameHash;
                    std::string rp = receiverNicknameHash.length() >= 5 ? receiverNicknameHash.substr(0, 5) : receiverNicknameHash;
                    LOG_INFO("Calling stopped: {} -> {}", sp, rp);
                }
            }
        }
        catch (const std::exception& e) {
            LOG_ERROR("Stop outgoing call error: {}", e.what());
        }
    }

    void Server::handleAcceptCall(const nlohmann::json& json, network::tcp::ConnectionPtr conn) {
        std::lock_guard<std::mutex> lock(m_mutex);
        try {
            std::string senderNicknameHash = json[SENDER_NICKNAME_HASH].get<std::string>();
            std::string receiverNicknameHash = json[RECEIVER_NICKNAME_HASH].get<std::string>();

            auto sender = m_userRepository.findUserByNickname(senderNicknameHash);
            auto receiver = m_userRepository.findUserByNickname(receiverNicknameHash);
            if (!sender || !receiver || sender->getTcpConnection() != conn) return;

            auto incoming = sender->getIncomingPendingCalls();
            PendingCallPtr found;
            for (auto& pc : incoming) {
                if (pc->getInitiator()->getNicknameHash() == receiverNicknameHash) {
                    found = pc;
                    break;
                }
            }
            if (!found) return;

            std::vector<unsigned char> body = toBytes(json.dump());
            sendTcpToUserIfConnected(receiverNicknameHash, static_cast<uint32_t>(PacketType::CALL_ACCEPT), body);

            resetOutgoingPendingCall(receiver);
            removeIncomingPendingCall(sender, found);
            auto call = m_callManager.createCall(receiver, sender);
            receiver->setCall(call);
            sender->setCall(call);

            std::string sp = senderNicknameHash.length() >= 5 ? senderNicknameHash.substr(0, 5) : senderNicknameHash;
            std::string rp = receiverNicknameHash.length() >= 5 ? receiverNicknameHash.substr(0, 5) : receiverNicknameHash;
            LOG_INFO("Call accepted: {} -> {}", rp, sp);
        }
        catch (const std::exception& e) {
            LOG_ERROR("Accept call error: {}", e.what());
        }
    }

    void Server::handleDeclineCall(const nlohmann::json& json, network::tcp::ConnectionPtr conn) {
        std::lock_guard<std::mutex> lock(m_mutex);
        try {
            std::string receiverNicknameHash = json[RECEIVER_NICKNAME_HASH].get<std::string>();
            std::string senderNicknameHash = json[SENDER_NICKNAME_HASH].get<std::string>();

            auto receiver = m_userRepository.findUserByNickname(receiverNicknameHash);
            auto sender = m_userRepository.findUserByNickname(senderNicknameHash);
            if (!sender || sender->getTcpConnection() != conn) return;

            std::vector<unsigned char> body = toBytes(json.dump());
            if (receiver && !receiver->isConnectionDown())
                sendTcpToUserIfConnected(receiverNicknameHash, static_cast<uint32_t>(PacketType::CALL_DECLINE), body);

            if (receiver) {
                auto incoming = receiver->getIncomingPendingCalls();
                for (auto& pc : incoming) {
                    if (pc->getInitiator()->getNicknameHash() == senderNicknameHash) {
                        resetOutgoingPendingCall(sender);
                        removeIncomingPendingCall(receiver, pc);
                        std::string sp = senderNicknameHash.length() >= 5 ? senderNicknameHash.substr(0, 5) : senderNicknameHash;
                        std::string rp = receiverNicknameHash.length() >= 5 ? receiverNicknameHash.substr(0, 5) : receiverNicknameHash;
                        LOG_INFO("Call declined: {} -> {}", sp, rp);
                        break;
                    }
                }
            }
        }
        catch (const std::exception& e) {
            LOG_ERROR("Decline call error: {}", e.what());
        }
    }

    void Server::handleEndCall(const nlohmann::json& json, network::tcp::ConnectionPtr conn) {
        std::lock_guard<std::mutex> lock(m_mutex);
        try {
            std::string senderNicknameHash = json[SENDER_NICKNAME_HASH].get<std::string>();
            std::string receiverNicknameHash = json[RECEIVER_NICKNAME_HASH].get<std::string>();

            auto receiver = m_userRepository.findUserByNickname(receiverNicknameHash);
            auto sender = m_userRepository.findUserByNickname(senderNicknameHash);
            if (!sender || sender->getTcpConnection() != conn) return;

            std::vector<unsigned char> body = toBytes(json.dump());
            if (receiver && !receiver->isConnectionDown())
                sendTcpToUserIfConnected(receiverNicknameHash, static_cast<uint32_t>(PacketType::CALL_END), body);

            if (receiver && sender && sender->isInCall() && receiver->isInCall()) {
                std::string sp = senderNicknameHash.length() >= 5 ? senderNicknameHash.substr(0, 5) : senderNicknameHash;
                std::string rp = receiverNicknameHash.length() >= 5 ? receiverNicknameHash.substr(0, 5) : receiverNicknameHash;
                LOG_INFO("Call ended: {} ended call with {}", sp, rp);
                m_callManager.endCall(sender->getCall());
                sender->resetCall();
                receiver->resetCall();
            }
        }
        catch (const std::exception& e) {
            LOG_ERROR("End call error: {}", e.what());
        }
    }

    void Server::redirectPacket(const nlohmann::json& json, PacketType type, network::tcp::ConnectionPtr) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!json.contains(RECEIVER_NICKNAME_HASH)) return;
        std::string receiver = json[RECEIVER_NICKNAME_HASH].get<std::string>();
        sendTcpToUser(receiver, static_cast<uint32_t>(type), json.dump());
    }

    void Server::processConnectionDown(const UserPtr& user) {
        if (user->hasOutgoingPendingCall()) {
            auto out = user->getOutgoingPendingCall();
            auto receiver = out->getReceiver();
            if (m_userRepository.containsUser(receiver->getNicknameHash())) {
                auto rec = m_userRepository.findUserByNickname(receiver->getNicknameHash());
                if (rec && !rec->isConnectionDown()) {
                    auto [_, p] = PacketFactory::getConnectionDownWithUserPacket(user->getNicknameHash());
                    sendTcpToUserIfConnected(receiver->getNicknameHash(), static_cast<uint32_t>(PacketType::CONNECTION_DOWN_WITH_USER), p);
                }
                if (rec) removeIncomingPendingCall(rec, out);
            }
            resetOutgoingPendingCall(user);
        }

        auto incomingCopy = user->getIncomingPendingCalls();
        for (auto& pc : incomingCopy) {
            auto initiator = pc->getInitiator();
            if (m_userRepository.containsUser(initiator->getNicknameHash())) {
                auto init = m_userRepository.findUserByNickname(initiator->getNicknameHash());
                if (init && !init->isConnectionDown()) {
                    auto [_, p] = PacketFactory::getConnectionDownWithUserPacket(user->getNicknameHash());
                    sendTcpToUserIfConnected(initiator->getNicknameHash(), static_cast<uint32_t>(PacketType::CONNECTION_DOWN_WITH_USER), p);
                }
                if (init) resetOutgoingPendingCall(init);
            }
            removeIncomingPendingCall(user, pc);
        }

        if (user->isInCall()) {
            auto partner = user->getCallPartner();
            m_callManager.endCall(user->getCall());
            if (partner && m_userRepository.containsUser(partner->getNicknameHash())) {
                auto pInRepo = m_userRepository.findUserByNickname(partner->getNicknameHash());
                if (pInRepo && !pInRepo->isConnectionDown()) {
                    auto [_, p] = PacketFactory::getConnectionDownWithUserPacket(user->getNicknameHash());
                    sendTcpToUserIfConnected(partner->getNicknameHash(), static_cast<uint32_t>(PacketType::CONNECTION_DOWN_WITH_USER), p);
                }
                if (pInRepo) pInRepo->resetCall();
            }
            user->resetCall();
        }
    }

    void Server::processUserLogout(const UserPtr& user) {
        if (!user || !m_userRepository.containsUser(user->getNicknameHash())) return;
        std::string nicknameHash = user->getNicknameHash();
        std::string prefix = nicknameHash.length() >= 5 ? nicknameHash.substr(0, 5) : nicknameHash;
        LOG_INFO("User logout: {}", prefix);

        auto conn = user->getTcpConnection();
        if (conn)
            m_connToUser.erase(conn);

        if (user->isInCall()) {
            auto partner = user->getCallPartner();
            m_callManager.endCall(user->getCall());
            if (partner && m_userRepository.containsUser(partner->getNicknameHash())) {
                auto partnerInRepo = m_userRepository.findUserByNickname(partner->getNicknameHash());
                if (partnerInRepo) {
                    std::string pp = partnerInRepo->getNicknameHash().length() >= 5 ? partnerInRepo->getNicknameHash().substr(0, 5) : partnerInRepo->getNicknameHash();
                    LOG_INFO("Call ended due to logout: {} ended call with {}", prefix, pp);
                    if (!partnerInRepo->isConnectionDown()) {
                        auto [_, p] = PacketFactory::getUserLogoutPacket(nicknameHash);
                        sendTcpToUserIfConnected(partner->getNicknameHash(), static_cast<uint32_t>(PacketType::USER_LOGOUT), p);
                    }
                    partnerInRepo->resetCall();
                }
            }
            user->resetCall();
        }

        if (user->hasOutgoingPendingCall()) {
            auto out = user->getOutgoingPendingCall();
            auto pendingPartner = user->getOutgoingPendingCallPartner();
            if (pendingPartner && m_userRepository.containsUser(pendingPartner->getNicknameHash())) {
                auto pp = m_userRepository.findUserByNickname(pendingPartner->getNicknameHash());
                std::string ppPrefix = pp->getNicknameHash().length() >= 5 ? pp->getNicknameHash().substr(0, 5) : pp->getNicknameHash();
                LOG_INFO("Outgoing/Incoming call ended due to logout: {} <-> {}", prefix, ppPrefix);
                if (!pp->isConnectionDown()) {
                    auto [_, p] = PacketFactory::getUserLogoutPacket(nicknameHash);
                    sendTcpToUserIfConnected(pendingPartner->getNicknameHash(), static_cast<uint32_t>(PacketType::USER_LOGOUT), p);
                }
                removeIncomingPendingCall(pp, out);
            }
            resetOutgoingPendingCall(user);
        }

        auto incomingCopy = user->getIncomingPendingCalls();
        for (auto& pc : incomingCopy) {
            auto initiator = pc->getInitiator();
            if (initiator && m_userRepository.containsUser(initiator->getNicknameHash())) {
                auto init = m_userRepository.findUserByNickname(initiator->getNicknameHash());
                std::string ip = init->getNicknameHash().length() >= 5 ? init->getNicknameHash().substr(0, 5) : init->getNicknameHash();
                LOG_INFO("Incoming/Outgoing call ended due to logout: {} <-> {}", ip, prefix);
                if (!init->isConnectionDown()) {
                    auto [_, p] = PacketFactory::getUserLogoutPacket(nicknameHash);
                    sendTcpToUserIfConnected(initiator->getNicknameHash(), static_cast<uint32_t>(PacketType::USER_LOGOUT), p);
                }
                resetOutgoingPendingCall(init);
            }
            removeIncomingPendingCall(user, pc);
        }
        user->resetAllPendingCalls();
        m_userRepository.removeUser(nicknameHash);
    }

    bool Server::resetOutgoingPendingCall(const UserPtr& user) {
        if (!user->hasOutgoingPendingCall()) return false;
        auto pc = user->getOutgoingPendingCall();
        if (m_callManager.hasPendingCall(pc))
            m_callManager.removePendingCall(pc);
        user->resetOutgoingPendingCall();
        return true;
    }

    void Server::removeIncomingPendingCall(const UserPtr& user, const PendingCallPtr& pendingCall) {
        if (!user->hasIncomingPendingCall(pendingCall)) return;
        if (m_callManager.hasPendingCall(pendingCall))
            m_callManager.removePendingCall(pendingCall);
        user->removeIncomingPendingCall(pendingCall);
    }
}
