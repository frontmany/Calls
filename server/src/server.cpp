#include "server.h"
#include "logic/packetFactory.h"
#include "constants/jsonType.h"
#include "utilities/crypto.h"
#include "utilities/logger.h"
#include "utilities/metrics.h"
#include "models/pendingCall.h"
#include "models/meeting.h"
#include "models/pendingMeetingJoinRequest.h"
#include "network/tcp/packet.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
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
            [this](const unsigned char* data, int size, uint32_t type, const asio::ip::udp::endpoint& ep, const std::array<unsigned char, 32>& senderHash) {handleReceiveUdp(data, size, type, ep, senderHash);})
    {
        registerHandlers();
    }

    void Server::registerHandlers() {
        m_packetHandlers.emplace(PacketType::AUTHORIZATION, [this](const nlohmann::json& json, network::tcp::ConnectionPtr conn) { handleAuthorization(json, conn); });
        m_packetHandlers.emplace(PacketType::RECONNECT, [this](const nlohmann::json& json, network::tcp::ConnectionPtr conn) { handleReconnect(json, conn); });
        m_packetHandlers.emplace(PacketType::LOGOUT, [this](const nlohmann::json& json, network::tcp::ConnectionPtr conn) { handleLogout(json, conn); });
        m_packetHandlers.emplace(PacketType::GET_USER_INFO, [this](const nlohmann::json& json, network::tcp::ConnectionPtr conn) { handleGetFriendInfo(json, conn); });
        m_packetHandlers.emplace(PacketType::GET_METRICS, [this](const nlohmann::json& json, network::tcp::ConnectionPtr conn) { handleGetMetrics(json, conn); });
        m_packetHandlers.emplace(PacketType::CALLING_BEGIN, [this](const nlohmann::json& json, network::tcp::ConnectionPtr conn) { handleStartOutgoingCall(json, conn); });
        m_packetHandlers.emplace(PacketType::CALLING_END, [this](const nlohmann::json& json, network::tcp::ConnectionPtr conn) { handleStopOutgoingCall(json, conn); });
        m_packetHandlers.emplace(PacketType::CALL_ACCEPT, [this](const nlohmann::json& json, network::tcp::ConnectionPtr conn) { handleAcceptCall(json, conn); });
        m_packetHandlers.emplace(PacketType::CALL_DECLINE, [this](const nlohmann::json& json, network::tcp::ConnectionPtr conn) { handleDeclineCall(json, conn); });
        m_packetHandlers.emplace(PacketType::CALL_END, [this](const nlohmann::json& json, network::tcp::ConnectionPtr conn) { handleEndCall(json, conn); });
        m_packetHandlers.emplace(PacketType::SCREEN_SHARING_BEGIN, [this](const nlohmann::json& json, network::tcp::ConnectionPtr conn) { redirectPacket(json, PacketType::SCREEN_SHARING_BEGIN, conn); });
        m_packetHandlers.emplace(PacketType::SCREEN_SHARING_END, [this](const nlohmann::json& json, network::tcp::ConnectionPtr conn) { redirectPacket(json, PacketType::SCREEN_SHARING_END, conn); });
        m_packetHandlers.emplace(PacketType::CAMERA_SHARING_BEGIN, [this](const nlohmann::json& json, network::tcp::ConnectionPtr conn) { redirectPacket(json, PacketType::CAMERA_SHARING_BEGIN, conn); });
        m_packetHandlers.emplace(PacketType::CAMERA_SHARING_END, [this](const nlohmann::json& json, network::tcp::ConnectionPtr conn) { redirectPacket(json, PacketType::CAMERA_SHARING_END, conn); });
        m_packetHandlers.emplace(PacketType::MEETING_CREATE, [this](const nlohmann::json& json, network::tcp::ConnectionPtr conn) { handleMeetingCreate(json, conn); });
        m_packetHandlers.emplace(PacketType::GET_MEETING_INFO, [this](const nlohmann::json& json, network::tcp::ConnectionPtr conn) { handleGetMeetingInfo(json, conn); });
        m_packetHandlers.emplace(PacketType::MEETING_JOIN_REQUEST, [this](const nlohmann::json& json, network::tcp::ConnectionPtr conn) { handleMeetingJoinRequest(json, conn); });
        m_packetHandlers.emplace(PacketType::MEETING_JOIN_CANCEL, [this](const nlohmann::json& json, network::tcp::ConnectionPtr conn) { handleMeetingJoinCancel(json, conn); });
        m_packetHandlers.emplace(PacketType::MEETING_JOIN_ACCEPT, [this](const nlohmann::json& json, network::tcp::ConnectionPtr conn) { handleMeetingJoinAccept(json, conn); });
        m_packetHandlers.emplace(PacketType::MEETING_JOIN_DECLINE, [this](const nlohmann::json& json, network::tcp::ConnectionPtr conn) { handleMeetingJoinDecline(json, conn); });
        m_packetHandlers.emplace(PacketType::MEETING_LEAVE, [this](const nlohmann::json& json, network::tcp::ConnectionPtr conn) { handleMeetingLeave(json, conn); });
        m_packetHandlers.emplace(PacketType::MEETING_END, [this](const nlohmann::json& json, network::tcp::ConnectionPtr conn) { handleMeetingEnd(json, conn); });
    }

    void Server::run() {
        m_networkController.start();
    }

    void Server::stop() {
        m_networkController.stop();
    }

    void Server::handleReceiveUdp(const unsigned char* data, int size, uint32_t rawType, const asio::ip::udp::endpoint& endpointFrom,
        const std::array<unsigned char, 32>& senderNicknameHash) {
        PacketType type = static_cast<PacketType>(rawType);
        if (type != PacketType::VOICE && type != PacketType::SCREEN && type != PacketType::CAMERA)
            return;

        std::string senderHashHex = utilities::crypto::binaryToHex(senderNicknameHash.data(), senderNicknameHash.size());
        UserPtr sender = m_userRepository.findUserByNickname(senderHashHex);
        if (!sender) {
            LOG_DEBUG("[UDP] Media from unknown sender hash {}:{}", senderHashHex.substr(0, 10), endpointFrom.address().to_string());
            return;
        }

        m_userRepository.updateUserUdpEndpoint(senderHashHex, endpointFrom);

        if (sender->isInCall()) {
            UserPtr partner = sender->getCallPartner();
            if (!partner || !m_userRepository.containsUser(partner->getNicknameHash())) {
                return;
            }
            if (partner->isConnectionDown()) {
                return;
            }

            m_networkController.sendUdp(data, size, rawType, partner->getEndpoint());
            return;
        }

        if (!sender->isInMeeting()) {
            return;
        }

        auto meeting = sender->getMeeting();
        if (!meeting) {
            return;
        }

        const std::string senderHash = sender->getNicknameHash();
        for (const auto& participant : meeting->getParticipants()) {
            if (!participant.user) {
                continue;
            }
            if (participant.user->getNicknameHash() == senderHash) {
                continue;
            }
            if (participant.user->isConnectionDown()) {
                continue;
            }
            m_networkController.sendUdp(data, size, rawType, participant.user->getEndpoint());
        }
    }

    void Server::handleReceiveTcp(network::tcp::OwnedPacket&& owned) {
        network::tcp::ConnectionPtr conn = owned.connection;
        network::tcp::Packet& p = owned.packet;
        uint32_t rawType = p.type;

        nlohmann::json json;
        if (!p.body.empty()) {
            std::string jsonStr(p.body.begin(), p.body.end());
            try {
                json = nlohmann::json::parse(jsonStr);
            }
            catch (const std::exception& e) {
                LOG_ERROR("[TCP] Invalid JSON in control packet type {}: {}", rawType, e.what());
                return;
            }
        }

        PacketType type = static_cast<PacketType>(rawType);

        auto it = m_packetHandlers.find(type);
        if (it != m_packetHandlers.end()) {
            it->second(json, conn);
        }
        else {
            LOG_WARN("[TCP] Unknown control packet type {}", rawType);
        }
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
        try {
            std::string uid = json[UID].get<std::string>();
            std::string token = json[TOKEN].get<std::string>();
            std::string senderNicknameHash = json[SENDER_NICKNAME_HASH].get<std::string>();
            std::string prefix = senderNicknameHash.length() >= 5 ? senderNicknameHash.substr(0, 5) : senderNicknameHash;

            uint16_t udpPort = 0;
            if (json.contains(UDP_PORT) && json[UDP_PORT].is_number_unsigned())
                udpPort = static_cast<uint16_t>(json[UDP_PORT].get<uint32_t>());

            UserPtr user;
            bool allowed = false;
            network::tcp::ConnectionPtr oldConn;
            bool userInCall = false;
            bool userInMeeting = false;
            std::string callPartnerHash;
            std::optional<bool> isInMeetingOpt;
            std::optional<std::string> meetingRosterJson;
            MeetingPtr meeting;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                user = m_userRepository.findUserByNickname(senderNicknameHash);
                if (!user) {
                    LOG_INFO("[TCP] Reconnect: user {} not found (logged out or never authorized)", prefix);
                }
                else {
                    allowed = (user->getToken() == token);
                    if (!allowed) {
                        LOG_WARN("[TCP] Reconnect: token mismatch for user {}", prefix);
                    }
                    else {
                        oldConn = user->getTcpConnection();
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
                        userInCall = user->isInCall();
                        userInMeeting = user->isInMeeting();
                        if (userInCall) {
                            auto partner = user->getCallPartner();
                            if (partner) callPartnerHash = partner->getNicknameHash();
                        }
                        isInMeetingOpt = userInMeeting;
                        if (userInMeeting) {
                            meeting = user->getMeeting();
                            if (meeting) {
                                nlohmann::json roster = nlohmann::json::array();
                                const auto owner = meeting->getOwner();
                                const std::string ownerHash = owner ? owner->getNicknameHash() : "";

                                for (const auto& participant : meeting->getParticipants()) {
                                    if (!participant.user) {
                                        continue;
                                    }
                                    nlohmann::json item;
                                    item[ENCRYPTED_NICKNAME] = participant.encryptedNickname;
                                    item[PUBLIC_KEY] = crypto::serializePublicKey(participant.user->getPublicKey());
                                    item[IS_OWNER] = (!ownerHash.empty() && participant.user->getNicknameHash() == ownerHash);
                                    roster.push_back(std::move(item));
                                }
                                meetingRosterJson = roster.dump();
                            }
                        }
                    }
                }
            }

            if (!user) {
                auto packet = PacketFactory::getReconnectionResultPacket(false, uid, senderNicknameHash, token);
                sendTcp(conn, static_cast<uint32_t>(PacketType::RECONNECT_RESULT), packet);
                return;
            }

            std::vector<unsigned char> packet = PacketFactory::getReconnectionResultPacket(
                allowed, uid, senderNicknameHash, token,
                allowed && userInCall, callPartnerHash,
                isInMeetingOpt,
                meetingRosterJson);
            sendTcp(conn, static_cast<uint32_t>(PacketType::RECONNECT_RESULT), packet);
            if (allowed && oldConn && oldConn != conn) {
                oldConn->close();
            }

            if (allowed) {
                LOG_INFO("[TCP] Reconnect OK: user {}", prefix);
                auto [_, restoredPacket] = PacketFactory::getConnectionRestoredWithUserPacket(user->getNicknameHash());
                sendTcp(conn, static_cast<uint32_t>(PacketType::CONNECTION_RESTORED_WITH_USER), restoredPacket);
            }

            if (allowed && userInCall) {
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

            // If user is in a meeting after reconnect, replay current media sharing state to them.
            if (allowed && userInMeeting && meeting) {
                const std::string userHash = user->getNicknameHash();
                for (const auto& sharerHash : meeting->getScreenSharers()) {
                    if (sharerHash == userHash) {
                        continue;
                    }
                    auto beginPacket = PacketFactory::getMediaSharingBeginPacket(sharerHash);
                    sendTcp(conn, static_cast<uint32_t>(PacketType::SCREEN_SHARING_BEGIN), beginPacket);
                }
                for (const auto& sharerHash : meeting->getCameraSharers()) {
                    if (sharerHash == userHash) {
                        continue;
                    }
                    auto beginPacket = PacketFactory::getMediaSharingBeginPacket(sharerHash);
                    sendTcp(conn, static_cast<uint32_t>(PacketType::CAMERA_SHARING_BEGIN), beginPacket);
                }
                sendMeetingConnectionDownStateToUser(meeting, userHash);

                auto [_, restoredPacket] = PacketFactory::getConnectionRestoredWithUserPacket(user->getNicknameHash());
                broadcastToMeeting(meeting, user->getNicknameHash(), static_cast<uint32_t>(PacketType::CONNECTION_RESTORED_WITH_USER), restoredPacket);
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

    void Server::handleGetMetrics(const nlohmann::json&, network::tcp::ConnectionPtr conn) {
        try {
            double cpuUsage = getCpuUsagePercent();
            auto [memoryUsed, memoryAvailable] = getMemoryUsedAndAvailable();
            size_t activeUsers = m_userRepository.getActiveUsersCount();

            auto packet = PacketFactory::getMetricsResultPacket(
                cpuUsage, static_cast<uint64_t>(memoryUsed), static_cast<uint64_t>(memoryAvailable), activeUsers);
            
            sendTcp(conn, static_cast<uint32_t>(PacketType::GET_METRICS_RESULT), packet);
        }
        catch (const std::exception& e) {
            LOG_ERROR("Get metrics error: {}", e.what());
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
            if (!canStartCallLocked(sender, receiver)) return;

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
            std::vector<unsigned char> body = toBytes(json.dump());
            if (sendTcpToUserIfConnected(receiverNicknameHash, static_cast<uint32_t>(PacketType::CALLING_BEGIN), body)) {
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
            std::string senderNicknameHash = json[SENDER_NICKNAME_HASH].get<std::string>();

            auto sender = m_userRepository.findUserByNickname(senderNicknameHash);
            if (!sender || sender->getTcpConnection() != conn) return;
            if (!sender->hasOutgoingPendingCall()) return;

            auto out = sender->getOutgoingPendingCall();
            auto receiver = out->getReceiver();

            std::vector<unsigned char> body = toBytes(json.dump());
            if (receiver && !receiver->isConnectionDown())
                sendTcpToUserIfConnected(receiver->getNicknameHash(), static_cast<uint32_t>(PacketType::CALLING_END), body);

            std::string sp = senderNicknameHash.length() >= 5 ? senderNicknameHash.substr(0, 5) : senderNicknameHash;
            std::string rp = receiver->getNicknameHash().length() >= 5 ? receiver->getNicknameHash().substr(0, 5) : receiver->getNicknameHash();
            LOG_INFO("Outgoing call stopped: {} -> {}", sp, rp);

            resetOutgoingPendingCall(sender);
            removeIncomingPendingCall(receiver, out);
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
            if (!canAcceptCallLocked(sender, receiver)) return;

            auto incoming = sender->getIncomingPendingCalls();
            PendingCallPtr found;
            for (auto& pc : incoming) {
                auto initiator = pc->getInitiator();
                if (initiator && initiator->getNicknameHash() == receiverNicknameHash) {
                    found = pc;
                    break;
                }
            }
            if (!found) return;

            std::vector<unsigned char> body = toBytes(json.dump());
            bool delivered = sendTcpToUserIfConnected(receiverNicknameHash, static_cast<uint32_t>(PacketType::CALL_ACCEPT), body);

            if (!delivered) {
                resetOutgoingPendingCall(receiver);
                removeIncomingPendingCall(sender, found);
                std::vector<unsigned char> callEndBody = PacketFactory::getCallEndPacket(receiverNicknameHash);
                sendTcpToUserIfConnected(senderNicknameHash, static_cast<uint32_t>(PacketType::CALL_END), callEndBody);
                std::string rp = receiverNicknameHash.length() >= 5 ? receiverNicknameHash.substr(0, 5) : receiverNicknameHash;
                std::string sp = senderNicknameHash.length() >= 5 ? senderNicknameHash.substr(0, 5) : senderNicknameHash;
                LOG_INFO("Call accept not delivered (caller {} unreachable), callee {} notified", rp, sp);
                return;
            }

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

            auto sender = m_userRepository.findUserByNickname(senderNicknameHash);
            auto receiver = m_userRepository.findUserByNickname(receiverNicknameHash);
            if (!sender || sender->getTcpConnection() != conn) return;

            std::vector<unsigned char> body = toBytes(json.dump());
            if (receiver && !receiver->isConnectionDown())
                sendTcpToUserIfConnected(receiverNicknameHash, static_cast<uint32_t>(PacketType::CALL_DECLINE), body);

            auto incoming = sender->getIncomingPendingCalls();
            for (auto& pc : incoming) {
                if (pc->getInitiator()->getNicknameHash() == receiverNicknameHash) {
                    if (receiver) resetOutgoingPendingCall(receiver);
                    removeIncomingPendingCall(sender, pc);
                    std::string sp = senderNicknameHash.length() >= 5 ? senderNicknameHash.substr(0, 5) : senderNicknameHash;
                    std::string rp = receiverNicknameHash.length() >= 5 ? receiverNicknameHash.substr(0, 5) : receiverNicknameHash;
                    LOG_INFO("Call declined: {} declined call from {}", sp, rp);
                    break;
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

            auto sender = m_userRepository.findUserByNickname(senderNicknameHash);
            if (!sender || sender->getTcpConnection() != conn) return;
            if (!sender->isInCall()) return;

            auto receiver = sender->getCallPartner();

            std::vector<unsigned char> body = toBytes(json.dump());
            if (receiver && !receiver->isConnectionDown())
                sendTcpToUserIfConnected(receiver->getNicknameHash(), static_cast<uint32_t>(PacketType::CALL_END), body);

            std::string sp = senderNicknameHash.length() >= 5 ? senderNicknameHash.substr(0, 5) : senderNicknameHash;
            if (receiver) {
                std::string rp = receiver->getNicknameHash().length() >= 5 ? receiver->getNicknameHash().substr(0, 5) : receiver->getNicknameHash();
                LOG_INFO("Call ended: {} ended call with {}", sp, rp);
                receiver->resetCall();
            }
            m_callManager.endCall(sender->getCall());
            sender->resetCall();
        }
        catch (const std::exception& e) {
            LOG_ERROR("End call error: {}", e.what());
        }
    }

    void Server::handleMeetingCreate(const nlohmann::json& json, network::tcp::ConnectionPtr conn)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        try {
            std::string senderNicknameHash = json[SENDER_NICKNAME_HASH].get<std::string>();
            auto sender = m_userRepository.findUserByNickname(senderNicknameHash);
            if (!sender || sender->getTcpConnection() != conn) {
                return;
            }
            if (sender->isInCall() || sender->isInMeeting()) {
                auto packet = PacketFactory::getMeetingCreateResultPacket(false);
                sendTcp(conn, static_cast<uint32_t>(PacketType::MEETING_CREATE_RESULT), packet);
                return;
            }

            const std::string meetingId = crypto::generateUID();
            const std::string meetingIdHash = crypto::calculateHash(meetingId);
            auto meeting = m_meetingManager.createMeeting(meetingId, meetingIdHash, sender);
            if (!meeting) {
                auto packet = PacketFactory::getMeetingCreateResultPacket(false);
                sendTcp(conn, static_cast<uint32_t>(PacketType::MEETING_CREATE_RESULT), packet);
                return;
            }

            sender->setMeeting(meeting);
            std::string encryptedNickname;
            if (json.contains(ENCRYPTED_NICKNAME) && json[ENCRYPTED_NICKNAME].is_string()) {
                encryptedNickname = json[ENCRYPTED_NICKNAME].get<std::string>();
            }
            meeting->addParticipant(sender, encryptedNickname);

            std::optional<std::string> encryptedMeetingKey;
            if (json.contains(ENCRYPTED_MEETING_KEY) && json[ENCRYPTED_MEETING_KEY].is_string()) {
                encryptedMeetingKey = json[ENCRYPTED_MEETING_KEY].get<std::string>();
            }
            std::optional<std::string> packetKey;
            if (json.contains(PACKET_KEY) && json[PACKET_KEY].is_string()) {
                packetKey = json[PACKET_KEY].get<std::string>();
            }

            auto packet = PacketFactory::getMeetingCreateResultPacket(true, meetingId, encryptedMeetingKey, packetKey);
            sendTcp(conn, static_cast<uint32_t>(PacketType::MEETING_CREATE_RESULT), packet);
        } 
        catch (const std::exception& e) {
            LOG_ERROR("Meeting create error: {}", e.what());
        }
    }

    void Server::handleGetMeetingInfo(const nlohmann::json& json, network::tcp::ConnectionPtr conn)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        try {
            const std::string meetingIdHash = json[MEETING_ID_HASH].get<std::string>();
            auto meeting = m_meetingManager.findByIdHash(meetingIdHash);
            if (!meeting) {
                auto packet = PacketFactory::getMeetingInfoResultPacket(false);
                sendTcp(conn, static_cast<uint32_t>(PacketType::GET_MEETING_INFO_RESULT), packet);
                return;
            }

            auto owner = meeting->getOwner();
            if (!owner) {
                auto packet = PacketFactory::getMeetingInfoResultPacket(false);
                sendTcp(conn, static_cast<uint32_t>(PacketType::GET_MEETING_INFO_RESULT), packet);
                return;
            }

            auto packet = PacketFactory::getMeetingInfoResultPacket(true, crypto::serializePublicKey(owner->getPublicKey()));
            sendTcp(conn, static_cast<uint32_t>(PacketType::GET_MEETING_INFO_RESULT), packet);
        }
        catch (const std::exception& e) {
            LOG_ERROR("Get meeting info error: {}", e.what());
        }
    }

    void Server::handleMeetingJoinRequest(const nlohmann::json& json, network::tcp::ConnectionPtr conn)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        try {
            const std::string senderNicknameHash = json[SENDER_NICKNAME_HASH].get<std::string>();
            const std::string meetingIdHash = json[MEETING_ID_HASH].get<std::string>();
            auto sender = m_userRepository.findUserByNickname(senderNicknameHash);
            auto meeting = m_meetingManager.findByIdHash(meetingIdHash);
            if (!sender || !meeting || sender->getTcpConnection() != conn) {
                return;
            }
            if (!canJoinMeetingLocked(sender) || sender->hasPendingMeetingJoinRequest()) {
                auto packet = PacketFactory::getMeetingJoinRejectedPacket("invalid_state");
                sendTcp(conn, static_cast<uint32_t>(PacketType::MEETING_JOIN_REJECTED), packet);
                return;
            }

            auto owner = meeting->getOwner();
            if (!owner) {
                auto packet = PacketFactory::getMeetingJoinRejectedPacket("meeting_not_found");
                sendTcp(conn, static_cast<uint32_t>(PacketType::MEETING_JOIN_REJECTED), packet);
                return;
            }
            if (owner->isConnectionDown()) {
                auto packet = PacketFactory::getMeetingJoinRejectedPacket("owner_unavailable");
                sendTcp(conn, static_cast<uint32_t>(PacketType::MEETING_JOIN_REJECTED), packet);
                return;
            }
            if (meeting->getPendingJoinRequest(senderNicknameHash)) {
                return;
            }

            std::weak_ptr<User> weakSender = sender;
            std::weak_ptr<Meeting> weakMeeting = meeting;
            auto pending = std::make_shared<PendingMeetingJoinRequest>(sender, meeting, [this, weakSender, weakMeeting]() {
                std::lock_guard<std::mutex> innerLock(m_mutex);
                auto requester = weakSender.lock();
                auto meetingFromWeak = weakMeeting.lock();
                if (!requester || !meetingFromWeak) {
                    return;
                }

                auto existingPending = requester->getPendingMeetingJoinRequest();
                if (!existingPending) {
                    return;
                }

                meetingFromWeak->removePendingJoinRequest(requester->getNicknameHash());
                m_meetingManager.removePendingJoinRequest(existingPending);
                requester->resetPendingMeetingJoinRequest();

                auto rejectPacket = PacketFactory::getMeetingJoinRejectedPacket("request_timeout");
                sendTcpToUserIfConnected(requester->getNicknameHash(), static_cast<uint32_t>(PacketType::MEETING_JOIN_REJECTED), rejectPacket);
            });

            sender->setPendingMeetingJoinRequest(pending);
            meeting->addPendingJoinRequest(pending);
            m_meetingManager.addPendingJoinRequest(pending);

            bool delivered = sendTcpToUserIfConnected(owner->getNicknameHash(), static_cast<uint32_t>(PacketType::MEETING_JOIN_REQUEST), toBytes(json.dump()));
            if (!delivered) {
                auto removed = meeting->removePendingJoinRequest(senderNicknameHash);
                if (removed) {
                    removed->stop();
                    m_meetingManager.removePendingJoinRequest(removed);
                }
                sender->resetPendingMeetingJoinRequest();
                auto packet = PacketFactory::getMeetingJoinRejectedPacket("owner_unavailable");
                sendTcp(conn, static_cast<uint32_t>(PacketType::MEETING_JOIN_REJECTED), packet);
            }
        }
        catch (const std::exception& e) {
            LOG_ERROR("Meeting join request error: {}", e.what());
        }
    }

    void Server::handleMeetingJoinCancel(const nlohmann::json& json, network::tcp::ConnectionPtr conn)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        try {
            const std::string senderNicknameHash = json[SENDER_NICKNAME_HASH].get<std::string>();
            auto sender = m_userRepository.findUserByNickname(senderNicknameHash);
            if (!sender || sender->getTcpConnection() != conn || !sender->hasPendingMeetingJoinRequest()) {
                return;
            }

            auto pending = sender->getPendingMeetingJoinRequest();
            auto meeting = pending ? pending->getMeeting() : nullptr;
            if (!meeting) {
                sender->resetPendingMeetingJoinRequest();
                return;
            }

            auto owner = meeting->getOwner();
            if (owner && !owner->isConnectionDown()) {
                sendTcpToUserIfConnected(owner->getNicknameHash(), static_cast<uint32_t>(PacketType::MEETING_JOIN_CANCEL), toBytes(json.dump()));
            }

            auto removed = meeting->removePendingJoinRequest(senderNicknameHash);
            if (removed) {
                removed->stop();
                m_meetingManager.removePendingJoinRequest(removed);
            }
            sender->resetPendingMeetingJoinRequest();
        }
        catch (const std::exception& e) {
            LOG_ERROR("Meeting join cancel error: {}", e.what());
        }
    }

    void Server::handleMeetingJoinAccept(const nlohmann::json& json, network::tcp::ConnectionPtr conn)
    {
        try {
            const std::string ownerNicknameHash = json[SENDER_NICKNAME_HASH].get<std::string>();
            const std::string requesterNicknameHash = json[REQUESTER_NICKNAME_HASH].get<std::string>();
            const std::string encryptedNickname = json[ENCRYPTED_NICKNAME].get<std::string>();

            MeetingPtr meeting;
            UserPtr requester;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto owner = m_userRepository.findUserByNickname(ownerNicknameHash);
                requester = m_userRepository.findUserByNickname(requesterNicknameHash);
                if (!owner || !requester || owner->getTcpConnection() != conn) {
                    return;
                }

                meeting = owner->getMeeting();
                if (!meeting || !meeting->isOwner(ownerNicknameHash)) {
                    return;
                }

                auto pending = meeting->getPendingJoinRequest(requesterNicknameHash);
                if (!pending || requester->getPendingMeetingJoinRequest() != pending) {
                    return;
                }
                if (!canJoinMeetingLocked(requester)) {
                    auto removed = meeting->removePendingJoinRequest(requesterNicknameHash);
                    if (removed) {
                        removed->stop();
                        m_meetingManager.removePendingJoinRequest(removed);
                    }
                    requester->resetPendingMeetingJoinRequest();
                    auto rejectPacket = PacketFactory::getMeetingJoinRejectedPacket("invalid_state");
                    sendTcpToUserIfConnected(requesterNicknameHash, static_cast<uint32_t>(PacketType::MEETING_JOIN_REJECTED), rejectPacket);
                    return;
                }
            }

            const bool delivered = sendTcpToUserIfConnected(requesterNicknameHash, static_cast<uint32_t>(PacketType::MEETING_JOIN_ACCEPT), toBytes(json.dump()));
            if (!delivered) {
                return;
            }

            {
                std::lock_guard<std::mutex> lock(m_mutex);
                // Re-validate before committing state transition.
                if (!requester || !meeting || !canJoinMeetingLocked(requester)) {
                    return;
                }
                auto pending = meeting->getPendingJoinRequest(requesterNicknameHash);
                if (!pending || requester->getPendingMeetingJoinRequest() != pending) {
                    return;
                }

                auto removed = meeting->removePendingJoinRequest(requesterNicknameHash);
                if (removed) {
                    removed->stop();
                    m_meetingManager.removePendingJoinRequest(removed);
                }
                requester->resetPendingMeetingJoinRequest();
                requester->setMeeting(meeting);
                meeting->addParticipant(requester, encryptedNickname);
            }

            auto joinedPacket = PacketFactory::getMeetingParticipantJoinedPacket(
                encryptedNickname,
                crypto::serializePublicKey(requester->getPublicKey()));
            broadcastToMeeting(meeting, requesterNicknameHash, static_cast<uint32_t>(PacketType::MEETING_PARTICIPANT_JOINED), joinedPacket);

            // Send the current media sharing state to the newly joined participant.
            for (const auto& sharerHash : meeting->getScreenSharers()) {
                if (sharerHash == requesterNicknameHash) {
                    continue;
                }
                auto beginPacket = PacketFactory::getMediaSharingBeginPacket(sharerHash);
                sendTcpToUserIfConnected(requesterNicknameHash, static_cast<uint32_t>(PacketType::SCREEN_SHARING_BEGIN), beginPacket);
            }
            for (const auto& sharerHash : meeting->getCameraSharers()) {
                if (sharerHash == requesterNicknameHash) {
                    continue;
                }
                auto beginPacket = PacketFactory::getMediaSharingBeginPacket(sharerHash);
                sendTcpToUserIfConnected(requesterNicknameHash, static_cast<uint32_t>(PacketType::CAMERA_SHARING_BEGIN), beginPacket);
            }
            sendMeetingConnectionDownStateToUser(meeting, requesterNicknameHash);
        }
        catch (const std::exception& e) {
            LOG_ERROR("Meeting join accept error: {}", e.what());
        }
    }

    void Server::sendMeetingConnectionDownStateToUser(const MeetingPtr& meeting, const std::string& receiverNicknameHash)
    {
        if (!meeting) {
            return;
        }

        for (const auto& participant : meeting->getParticipants()) {
            if (!participant.user) {
                continue;
            }
            const std::string participantHash = participant.user->getNicknameHash();
            if (participantHash == receiverNicknameHash) {
                continue;
            }
            if (participant.user->isConnectionDown()) {
                auto [_, downPacket] = PacketFactory::getConnectionDownWithUserPacket(participantHash);
                sendTcpToUserIfConnected(receiverNicknameHash, static_cast<uint32_t>(PacketType::CONNECTION_DOWN_WITH_USER), downPacket);
            }
        }
    }

    void Server::handleMeetingJoinDecline(const nlohmann::json& json, network::tcp::ConnectionPtr conn)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        try {
            const std::string ownerNicknameHash = json[SENDER_NICKNAME_HASH].get<std::string>();
            const std::string requesterNicknameHash = json[REQUESTER_NICKNAME_HASH].get<std::string>();

            auto owner = m_userRepository.findUserByNickname(ownerNicknameHash);
            auto requester = m_userRepository.findUserByNickname(requesterNicknameHash);
            if (!owner || !requester || owner->getTcpConnection() != conn) {
                return;
            }

            auto meeting = owner->getMeeting();
            if (!meeting || !meeting->isOwner(ownerNicknameHash)) {
                return;
            }

            sendTcpToUserIfConnected(requesterNicknameHash, static_cast<uint32_t>(PacketType::MEETING_JOIN_DECLINE), toBytes(json.dump()));

            auto removed = meeting->removePendingJoinRequest(requesterNicknameHash);
            if (removed) {
                removed->stop();
                m_meetingManager.removePendingJoinRequest(removed);
            }
            requester->resetPendingMeetingJoinRequest();
        }
        catch (const std::exception& e) {
            LOG_ERROR("Meeting join decline error: {}", e.what());
        }
    }

    void Server::handleMeetingLeave(const nlohmann::json& json, network::tcp::ConnectionPtr conn)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        try {
            const std::string senderNicknameHash = json[SENDER_NICKNAME_HASH].get<std::string>();
            auto sender = m_userRepository.findUserByNickname(senderNicknameHash);
            if (!sender || sender->getTcpConnection() != conn || !sender->isInMeeting()) {
                return;
            }

            auto meeting = sender->getMeeting();
            if (!meeting || meeting->isOwner(senderNicknameHash)) {
                return;
            }
            removeMeetingParticipant(meeting, sender);
        }
        catch (const std::exception& e) {
            LOG_ERROR("Meeting leave error: {}", e.what());
        }
    }

    void Server::handleMeetingEnd(const nlohmann::json& json, network::tcp::ConnectionPtr conn)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        try {
            const std::string senderNicknameHash = json[SENDER_NICKNAME_HASH].get<std::string>();
            auto sender = m_userRepository.findUserByNickname(senderNicknameHash);
            if (!sender || sender->getTcpConnection() != conn || !sender->isInMeeting()) {
                return;
            }

            auto meeting = sender->getMeeting();
            if (!meeting || !meeting->isOwner(senderNicknameHash)) {
                return;
            }

            endMeetingCleanup(meeting);
        }
        catch (const std::exception& e) {
            LOG_ERROR("Meeting end error: {}", e.what());
        }
    }

    void Server::redirectPacket(const nlohmann::json& json, PacketType type, network::tcp::ConnectionPtr conn) {
        std::vector<network::tcp::ConnectionPtr> targets;
        std::vector<unsigned char> body;
        std::string partnerHash;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!json.contains(SENDER_NICKNAME_HASH)) return;
            std::string senderHash = json[SENDER_NICKNAME_HASH].get<std::string>();
            auto sender = m_userRepository.findUserByNickname(senderHash);
            if (!sender || sender->getTcpConnection() != conn) return;

            body = toBytes(json.dump());

            if (sender->isInMeeting()) {
                auto meeting = sender->getMeeting();
                if (!meeting) return;

                // Gather recipients while holding server lock; perform actual sends after unlock.
                for (const auto& participant : meeting->getParticipants()) {
                    if (!participant.user) continue;
                    if (participant.user->getNicknameHash() == senderHash) continue;
                    if (participant.user->isConnectionDown()) continue;
                    auto pConn = participant.user->getTcpConnection();
                    if (!pConn) continue;
                    targets.push_back(std::move(pConn));
                }

                // Track media sharing state for meetings so that late joiners and
                // reconnecting participants can be brought up to date.
                switch (type) {
                case PacketType::SCREEN_SHARING_BEGIN:
                    meeting->addScreenSharer(senderHash);
                    break;
                case PacketType::SCREEN_SHARING_END:
                    meeting->removeScreenSharer(senderHash);
                    break;
                case PacketType::CAMERA_SHARING_BEGIN:
                    meeting->addCameraSharer(senderHash);
                    break;
                case PacketType::CAMERA_SHARING_END:
                    meeting->removeCameraSharer(senderHash);
                    break;
                default:
                    break;
                }

                // meeting path only
            }
            else if (sender->isInCall()) {
                auto partner = sender->getCallPartner();
                if (!partner) return;
                if (!partner->isConnectionDown()) {
                    partnerHash = partner->getNicknameHash();
                }
            }
            else {
                return;
            }
        }

        // Perform I/O outside the global server mutex to avoid head-of-line blocking.
        if (!targets.empty()) {
            for (auto& c : targets) {
                sendTcp(c, static_cast<uint32_t>(type), body);
            }
            return;
        }

        if (!partnerHash.empty()) {
            sendTcpToUser(partnerHash, static_cast<uint32_t>(type), json.dump());
        }
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
            if (partner && m_userRepository.containsUser(partner->getNicknameHash())) {
                auto pInRepo = m_userRepository.findUserByNickname(partner->getNicknameHash());
                if (pInRepo && !pInRepo->isConnectionDown()) {
                    auto [_, p] = PacketFactory::getConnectionDownWithUserPacket(user->getNicknameHash());
                    sendTcpToUserIfConnected(partner->getNicknameHash(), static_cast<uint32_t>(PacketType::CONNECTION_DOWN_WITH_USER), p);
                }
            }
            // Keep call state while user is temporarily offline.
            // Call is terminated only when:
            // - user reconnects and peer ends call, or
            // - reconnection timeout triggers logout flow.
        }

        if (user->hasPendingMeetingJoinRequest()) {
            auto pending = user->getPendingMeetingJoinRequest();
            auto meeting = pending ? pending->getMeeting() : nullptr;
            if (meeting) {
                auto owner = meeting->getOwner();
                if (owner && !owner->isConnectionDown()) {
                    nlohmann::json cancelJson;
                    cancelJson[UID] = crypto::generateUID();
                    cancelJson[SENDER_NICKNAME_HASH] = user->getNicknameHash();
                    sendTcpToUserIfConnected(owner->getNicknameHash(), static_cast<uint32_t>(PacketType::MEETING_JOIN_CANCEL), toBytes(cancelJson.dump()));
                }
                auto removed = meeting->removePendingJoinRequest(user->getNicknameHash());
                if (removed) {
                    removed->stop();
                    m_meetingManager.removePendingJoinRequest(removed);
                }
            }
            user->resetPendingMeetingJoinRequest();
        }

        if (user->isInMeeting()) {
            auto meeting = user->getMeeting();
            if (!meeting) {
                return;
            }
            {
                auto [_, p] = PacketFactory::getConnectionDownWithUserPacket(user->getNicknameHash());
                broadcastToMeeting(meeting, user->getNicknameHash(), static_cast<uint32_t>(PacketType::CONNECTION_DOWN_WITH_USER), p);
            }
            const std::string disconnectedHash = user->getNicknameHash();
            {
                auto screenSharers = meeting->getScreenSharers();
                if (std::find(screenSharers.begin(), screenSharers.end(), disconnectedHash) != screenSharers.end()) {
                    auto endPacket = PacketFactory::getMediaSharingEndPacket(disconnectedHash);
                    broadcastToMeeting(meeting, disconnectedHash, static_cast<uint32_t>(PacketType::SCREEN_SHARING_END), endPacket);
                    meeting->removeScreenSharer(disconnectedHash);
                }
                auto cameraSharers = meeting->getCameraSharers();
                if (std::find(cameraSharers.begin(), cameraSharers.end(), disconnectedHash) != cameraSharers.end()) {
                    auto endPacket = PacketFactory::getMediaSharingEndPacket(disconnectedHash);
                    broadcastToMeeting(meeting, disconnectedHash, static_cast<uint32_t>(PacketType::CAMERA_SHARING_END), endPacket);
                    meeting->removeCameraSharer(disconnectedHash);
                }
            }

            if (meeting->isOwner(user->getNicknameHash())) {
                rejectAllPendingJoinRequests(meeting, "owner_unavailable");
                return;
            }

            auto owner = meeting->getOwner();
            if (owner && owner->isConnectionDown()) {
                return;
            }
            // Keep regular participant in meeting while reconnect timer is active.
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

        if (user->hasPendingMeetingJoinRequest()) {
            auto pending = user->getPendingMeetingJoinRequest();
            auto meeting = pending ? pending->getMeeting() : nullptr;
            if (meeting) {
                auto removed = meeting->removePendingJoinRequest(user->getNicknameHash());
                if (removed) {
                    removed->stop();
                    m_meetingManager.removePendingJoinRequest(removed);
                }
            }
            user->resetPendingMeetingJoinRequest();
        }

        if (user->isInMeeting()) {
            auto meeting = user->getMeeting();
            if (meeting) {
                if (meeting->isOwner(nicknameHash)) {
                    endMeetingCleanup(meeting);
                }
                else {
                    removeMeetingParticipant(meeting, user);
                }
            }
        }

        m_userRepository.removeUser(nicknameHash);
    }

    void Server::broadcastToMeeting(const MeetingPtr& meeting, const std::string& excludeNicknameHash, uint32_t type, const std::vector<unsigned char>& body)
    {
        if (!meeting) {
            return;
        }

        for (const auto& participant : meeting->getParticipants()) {
            if (!participant.user) {
                continue;
            }
            const std::string participantHash = participant.user->getNicknameHash();
            if (!excludeNicknameHash.empty() && participantHash == excludeNicknameHash) {
                continue;
            }
            sendTcpToUserIfConnected(participantHash, type, body);
        }
    }

    void Server::rejectAllPendingJoinRequests(const MeetingPtr& meeting, const std::string& reason)
    {
        if (!meeting) {
            return;
        }

        const auto rejectPacket = PacketFactory::getMeetingJoinRejectedPacket(reason);
        auto pendingRequests = meeting->getPendingJoinRequests();
        for (const auto& pending : pendingRequests) {
            if (!pending) {
                continue;
            }
            auto requester = pending->getRequester();
            if (!requester) {
                continue;
            }

            const std::string requesterHash = requester->getNicknameHash();
            auto removed = meeting->removePendingJoinRequest(requesterHash);
            if (removed) {
                removed->stop();
                m_meetingManager.removePendingJoinRequest(removed);
            }
            requester->resetPendingMeetingJoinRequest();
            sendTcpToUserIfConnected(requesterHash, static_cast<uint32_t>(PacketType::MEETING_JOIN_REJECTED), rejectPacket);
        }
    }

    void Server::removeMeetingParticipant(const MeetingPtr& meeting, const UserPtr& user)
    {
        if (!meeting || !user) {
            return;
        }

        const std::string senderHash = user->getNicknameHash();

        // If user was actively sharing media, notify others that sharing has ended.
        if (meeting->isParticipant(senderHash)) {
            auto screenSharers = meeting->getScreenSharers();
            if (std::find(screenSharers.begin(), screenSharers.end(), senderHash) != screenSharers.end()) {
                auto packet = PacketFactory::getMediaSharingEndPacket(senderHash);
                broadcastToMeeting(meeting, senderHash, static_cast<uint32_t>(PacketType::SCREEN_SHARING_END), packet);
                meeting->removeScreenSharer(senderHash);
            }
            auto cameraSharers = meeting->getCameraSharers();
            if (std::find(cameraSharers.begin(), cameraSharers.end(), senderHash) != cameraSharers.end()) {
                auto packet = PacketFactory::getMediaSharingEndPacket(senderHash);
                broadcastToMeeting(meeting, senderHash, static_cast<uint32_t>(PacketType::CAMERA_SHARING_END), packet);
                meeting->removeCameraSharer(senderHash);
            }
        }

        if (!meeting->removeParticipant(senderHash).has_value()) {
            user->resetMeeting();
            return;
        }

        user->resetMeeting();
        auto leftPacket = PacketFactory::getMeetingParticipantLeftPacket(senderHash);
        broadcastToMeeting(meeting, senderHash, static_cast<uint32_t>(PacketType::MEETING_PARTICIPANT_LEFT), leftPacket);
    }

    void Server::endMeetingCleanup(const MeetingPtr& meeting)
    {
        if (!meeting) {
            return;
        }

        rejectAllPendingJoinRequests(meeting, "meeting_ended");

        auto endedPacket = PacketFactory::getMeetingEndedPacket();
        auto participants = meeting->getParticipants();
        std::string ownerHash;
        auto owner = meeting->getOwner();
        if (owner) {
            ownerHash = owner->getNicknameHash();
        }

        for (const auto& participant : participants) {
            if (!participant.user) {
                continue;
            }
            const std::string participantHash = participant.user->getNicknameHash();
            if (!ownerHash.empty() && participantHash == ownerHash) {
                participant.user->resetMeeting();
                continue;
            }
            sendTcpToUserIfConnected(participantHash, static_cast<uint32_t>(PacketType::MEETING_ENDED), endedPacket);
            participant.user->resetMeeting();
        }

        m_meetingManager.endMeeting(meeting);
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

    bool Server::canStartCallLocked(const UserPtr& sender, const UserPtr& receiver) const {
        if (!sender || !receiver) return false;
        if (sender->getNicknameHash() == receiver->getNicknameHash()) return false;

        if (sender->isInCall() || sender->isInMeeting() || sender->isPendingCall() || sender->hasPendingMeetingJoinRequest()) {
            return false;
        }
        if (receiver->isInCall() || receiver->isInMeeting() || receiver->isPendingCall() || receiver->hasPendingMeetingJoinRequest()) {
            return false;
        }
        return true;
    }

    bool Server::canAcceptCallLocked(const UserPtr& callee, const UserPtr& caller) const {
        if (!callee || !caller) return false;
        if (callee->isInCall() || callee->isInMeeting() || callee->hasOutgoingPendingCall() || callee->hasPendingMeetingJoinRequest()) {
            return false;
        }
        if (caller->isInCall() || caller->isInMeeting() || caller->hasPendingMeetingJoinRequest()) {
            return false;
        }
        return true;
    }

    bool Server::canJoinMeetingLocked(const UserPtr& user) const {
        if (!user) return false;
        if (user->isInCall() || user->isInMeeting() || user->isPendingCall()) {
            return false;
        }
        return true;
    }
}
