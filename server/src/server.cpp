#include "server.h"
#include "packetFactory.h"
#include "jsonType.h"
#include "utilities/crypto.h"
#include "utilities/logger.h"
#include "pendingCall.h"

using namespace std::chrono_literals;
using namespace std::placeholders;
using namespace server;
using namespace server::utilities;

namespace server
{
namespace
{
    std::vector<unsigned char> toBytes(const std::string& value) {
        return std::vector<unsigned char>(value.begin(), value.end());
    }

    struct DeferredSend {
        std::string uid;
        std::vector<unsigned char> packet;
        PacketType type;
        asio::ip::udp::endpoint endpoint;
        std::string failMessage;
    };
} 

Server::Server(const std::string& port)
{
    m_networkController.init(port,
        [this](const unsigned char* data, int size, uint32_t type, const asio::ip::udp::endpoint& endpointFrom) { onReceive(data, size, type, endpointFrom); },
        [this](const asio::ip::udp::endpoint& endpoint) {
            try {
                std::vector<DeferredSend> toSend;
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    if (!m_endpointToUser.contains(endpoint)) {
                        m_networkController.removePingMonitoring(endpoint);
                        return;
                    }
                    auto& user = m_endpointToUser.at(endpoint);
                    if (!user || user->isConnectionDown()) {
                        return;
                    }
                    const std::string userPrefix = user->getNicknameHash().length() >= 5 ? user->getNicknameHash().substr(0, 5) : user->getNicknameHash();
                    LOG_INFO("Connection down with user {}", userPrefix);
                    user->setConnectionDown(true);
                    clearPendingActionsForUser(user->getNicknameHash());

                    if (user->hasOutgoingPendingCall()) {
                        auto outgoingPendingCall = user->getOutgoingPendingCall();
                        auto receiver = outgoingPendingCall->getReceiver();
                        if (m_nicknameHashToUser.contains(receiver->getNicknameHash())) {
                            std::string rPrefix = receiver->getNicknameHash().length() >= 5 ? receiver->getNicknameHash().substr(0, 5) : receiver->getNicknameHash();
                            LOG_INFO("Outgoing call ended due to connection down: {} -> {}", userPrefix, rPrefix);
                            LOG_INFO("Incoming call ended due to connection down: {} -> {}", rPrefix, userPrefix);
                            if (!receiver->isConnectionDown()) {
                                auto [uid, pkt] = PacketFactory::getConnectionDownWithUserPacket(user->getNicknameHash());
                                toSend.push_back({uid, std::move(pkt), PacketType::CONNECTION_DOWN_WITH_USER, receiver->getEndpoint(), "Connection down with user task failed"});
                            }
                            removeIncomingPendingCall(receiver, outgoingPendingCall);
                        }
                        resetOutgoingPendingCall(user);
                    }

                    auto incomingCalls = user->getIncomingPendingCalls();
                    for (auto& pendingCall : incomingCalls) {
                        auto initiator = pendingCall->getInitiator();
                        if (m_nicknameHashToUser.contains(initiator->getNicknameHash())) {
                            std::string iPrefix = initiator->getNicknameHash().length() >= 5 ? initiator->getNicknameHash().substr(0, 5) : initiator->getNicknameHash();
                            LOG_INFO("Incoming call ended due to connection down: {} -> {}", iPrefix, userPrefix);
                            LOG_INFO("Outgoing call ended due to connection down: {} -> {}", iPrefix, userPrefix);
                            if (!initiator->isConnectionDown()) {
                                auto [uid, pkt] = PacketFactory::getConnectionDownWithUserPacket(user->getNicknameHash());
                                toSend.push_back({uid, std::move(pkt), PacketType::CONNECTION_DOWN_WITH_USER, initiator->getEndpoint(), "Connection down with user task failed"});
                            }
                            resetOutgoingPendingCall(initiator);
                            removeIncomingPendingCall(user, pendingCall);
                        }
                    }

                    if (user->isInCall()) {
                        auto callPartner = user->getCallPartner();
                        if (callPartner && m_nicknameHashToUser.contains(callPartner->getNicknameHash()) && !callPartner->isConnectionDown()) {
                            auto [uid, pkt] = PacketFactory::getConnectionDownWithUserPacket(user->getNicknameHash());
                            toSend.push_back({uid, std::move(pkt), PacketType::CONNECTION_DOWN_WITH_USER, callPartner->getEndpoint(), "Connection down with user task failed"});
                        }
                    }

                    auto [uidDown, connectionDownToSelf] = PacketFactory::getConnectionDownWithUserPacket(user->getNicknameHash());
                    toSend.push_back({uidDown, std::move(connectionDownToSelf), PacketType::CONNECTION_DOWN_WITH_USER, user->getEndpoint(), "Connection down notify self task failed"});
                }
                for (auto& s : toSend) {
                    m_taskManager.createTask(s.uid, 1500ms, 5,
                        std::bind(&Server::sendPacketTask, this, s.packet, s.type, s.endpoint),
                        std::bind(&Server::onTaskCompleted, this, _1),
                        std::bind(&Server::onTaskFailed, this, s.failMessage, _1));
                    m_taskManager.startTask(s.uid);
                }
            } catch (const std::exception& e) {
                LOG_ERROR("handleConnectionDown exception: {}", e.what());
            }
        },
        [this](const asio::ip::udp::endpoint& endpoint) {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_endpointToUser.contains(endpoint)) {
                auto& user = m_endpointToUser.at(endpoint);
                if (user) {
                    const std::string userPrefix = user->getNicknameHash().length() >= 5 ? user->getNicknameHash().substr(0, 5) : user->getNicknameHash();
                    LOG_INFO("Connection restored with user {}", userPrefix);
                } else {
                    LOG_INFO("Connection restored with unknown user object");
                }
            } else {
                LOG_INFO("Connection restored from endpoint {}:{}", endpoint.address().to_string(), endpoint.port());
            }
        }
    );

    m_packetHandlers.emplace(PacketType::AUTHORIZATION, [this](const nlohmann::json& json, const asio::ip::udp::endpoint& endpoint) {handleAuthorization(json, endpoint);});
    m_packetHandlers.emplace(PacketType::LOGOUT, [this](const nlohmann::json& json, const asio::ip::udp::endpoint& endpoint) {handleLogout(json, endpoint);});
    m_packetHandlers.emplace(PacketType::RECONNECT, [this](const nlohmann::json& json, const asio::ip::udp::endpoint& endpoint) {handleReconnect(json, endpoint);});
    m_packetHandlers.emplace(PacketType::GET_USER_INFO, [this](const nlohmann::json& json, const asio::ip::udp::endpoint& endpoint) {handleGetFriendInfo(json, endpoint);});
    m_packetHandlers.emplace(PacketType::CONFIRMATION, [this](const nlohmann::json& json, const asio::ip::udp::endpoint& endpoint) {handleConfirmation(json, endpoint);});
    m_packetHandlers.emplace(PacketType::CALLING_BEGIN, [this](const nlohmann::json& json, const asio::ip::udp::endpoint& endpoint) {handleStartOutgoingCall(json, endpoint);});
    m_packetHandlers.emplace(PacketType::CALLING_END, [this](const nlohmann::json& json, const asio::ip::udp::endpoint& endpoint) {handleStopOutgoingCall(json, endpoint);});
    m_packetHandlers.emplace(PacketType::CALL_ACCEPT, [this](const nlohmann::json& json, const asio::ip::udp::endpoint& endpoint) {handleAcceptCall(json, endpoint);});
    m_packetHandlers.emplace(PacketType::CALL_DECLINE, [this](const nlohmann::json& json, const asio::ip::udp::endpoint& endpoint) {handleDeclineCall(json, endpoint);});
    m_packetHandlers.emplace(PacketType::CALL_END, [this](const nlohmann::json& json, const asio::ip::udp::endpoint& endpoint) {handleEndCall(json, endpoint);});
}

void Server::onReceive(const unsigned char* data, int size, uint32_t rawType, const asio::ip::udp::endpoint& endpointFrom) {
    PacketType type = static_cast<PacketType>(rawType);
    
    if (type == PacketType::VOICE) {
        handleVoice(data, size, endpointFrom);
        return; 
    }
    else if (type == PacketType::SCREEN) {
        handleScreen(data, size, endpointFrom);
        return;
    }
    else if (type == PacketType::CAMERA) {
        handleCamera(data, size, endpointFrom);
        return;
    }

    try {
        std::string jsonStr = std::string(reinterpret_cast<const char*>(data), size);
        nlohmann::json jsonObject = nlohmann::json::parse(jsonStr);

        std::function<void(const nlohmann::json&, const asio::ip::udp::endpoint&)> handler;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_packetHandlers.contains(type)) {
                handler = m_packetHandlers[type];
            }
        }

        if (handler) {
            handler(jsonObject, endpointFrom);
        }
        else {
            redirect(jsonObject, type);
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error processing packet type {}: {}", static_cast<int>(type), e.what());
    }
}

void Server::run() {
    m_networkController.run();
}

void Server::stop() {
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_taskManager.cancelAllTasks();
    }
    
    m_networkController.stop();
    
}

void Server::redirect(const nlohmann::json& jsonObject, PacketType type) {
    const std::string& receiverNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH].get<std::string>();

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_nicknameHashToUser.contains(receiverNicknameHash)) {
        auto& user = m_nicknameHashToUser.at(receiverNicknameHash);

        auto packet = toBytes(jsonObject.dump());
        m_networkController.send(packet, static_cast<uint32_t>(type), user->getEndpoint());
    }
}

void Server::handleConfirmation(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    redirect(jsonObject, PacketType::CONFIRMATION);
    
    std::string uid = jsonObject[UID].get<std::string>();
    std::string receiverNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH].get<std::string>();
    std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();

    m_taskManager.completeTask(uid);

    std::function<void()> actionToExecute;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_nicknameHashToUser.contains(senderNicknameHash)) {
            auto& sender = m_nicknameHashToUser.at(senderNicknameHash);

            ConfirmationKey key = ConfirmationKey(uid, sender->getNicknameHash(), sender->getToken());

            if (m_pendingActions.contains(key)) {
                actionToExecute = m_pendingActions.at(key);
                removePendingAction(key);
            }
        }
    }

    if (actionToExecute) {
        actionToExecute();
    }
}

void Server::handleAuthorization(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string uid = jsonObject[UID].get<std::string>();
        std::string nicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();
        CryptoPP::RSA::PublicKey publicKey = crypto::deserializePublicKey(jsonObject[PUBLIC_KEY]);
        
        bool authorized = false;
        std::string token = "";
        
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_nicknameHashToUser.contains(nicknameHash)) {
            std::string nicknameHashPrefix = nicknameHash.length() >= 5 ? nicknameHash.substr(0, 5) : nicknameHash;
            LOG_WARN("Authorization failed - nickname already taken: {}", nicknameHashPrefix);
        }
        else {
            token = crypto::generateUID();

            UserPtr user = std::make_shared<User>(nicknameHash, token, publicKey, endpointFrom,
                [this, nicknameHash]() {
                    UserPtr user;
                    {
                        std::lock_guard<std::mutex> lock(m_mutex);
                        if (m_nicknameHashToUser.contains(nicknameHash)) {
                            user = m_nicknameHashToUser.at(nicknameHash);
                        }
                    }
                    
                    if (user) {
                        processUserLogout(user);
                    }
                }
            );

            m_endpointToUser.emplace(endpointFrom, user);
            m_nicknameHashToUser.emplace(nicknameHash, user);
                
            authorized = true;
            
            std::string nicknameHashPrefix = nicknameHash.length() >= 5 ? nicknameHash.substr(0, 5) : nicknameHash;
            LOG_INFO("User authorized: {}", nicknameHashPrefix);
        }

        std::vector<unsigned char> packet;
        if (authorized)
            packet = PacketFactory::getAuthorizationResultPacket(true, uid, nicknameHash, token);
        else
            packet = PacketFactory::getAuthorizationResultPacket(false, uid, nicknameHash);

        m_networkController.send(packet, static_cast<uint32_t>(PacketType::AUTHORIZATION_RESULT), endpointFrom);
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in authorization packet: {}", e.what());
    }
}

void Server::handleLogout(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string uid = jsonObject[UID].get<std::string>();
        std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();

        auto packet = PacketFactory::getConfirmationPacket(uid, senderNicknameHash);
        m_networkController.send(packet, static_cast<uint32_t>(PacketType::CONFIRMATION), endpointFrom);

        UserPtr user;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_nicknameHashToUser.contains(senderNicknameHash)) {
                user = m_nicknameHashToUser.at(senderNicknameHash);
            }
        }
        
        if (user) {
            processUserLogout(user);
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error during logout: {}", e.what());
    }
}

void Server::handleReconnect(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    std::string uid = jsonObject[UID].get<std::string>();
    std::string token = jsonObject[TOKEN].get<std::string>();
    std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_nicknameHashToUser.contains(senderNicknameHash)) {
        auto& user = m_nicknameHashToUser.at(senderNicknameHash);

        bool reconnectionAllowed = false;
        if (user->getToken() == token) {
            user->setConnectionDown(false);
            reconnectionAllowed = true;
        }

        if (reconnectionAllowed) {
            asio::ip::udp::endpoint oldEndpoint = user->getEndpoint();
            if (oldEndpoint != endpointFrom) {
                m_endpointToUser.erase(oldEndpoint);
                m_networkController.removePingMonitoring(oldEndpoint);
                user->setEndpoint(endpointFrom);
                m_endpointToUser.emplace(endpointFrom, user);
            }
        }

        std::vector<unsigned char> packet;
        if (reconnectionAllowed)
            packet = PacketFactory::getReconnectionResultPacket(true, uid, senderNicknameHash, token, user->isInCall());
        else
            packet = PacketFactory::getReconnectionResultPacket(false, uid, senderNicknameHash, token);
            
        m_networkController.send(packet, static_cast<uint32_t>(PacketType::RECONNECT_RESULT), endpointFrom);
        
        if (reconnectionAllowed && user->isInCall()) {
            auto partner = user->getCallPartner();

            if (m_nicknameHashToUser.contains(partner->getNicknameHash())) { // here todo
                if (!partner->isConnectionDown()) {
                    auto [uid, connectionRestoredWithUserPacket] = PacketFactory::getConnectionRestoredWithUserPacket(user->getNicknameHash());

                    m_taskManager.createTask(uid, 1500ms, 5,
                        std::bind(&Server::sendPacketTask, this, connectionRestoredWithUserPacket, PacketType::CONNECTION_RESTORED_WITH_USER, partner->getEndpoint()),
                        std::bind(&Server::onTaskCompleted, this, _1),
                        std::bind(&Server::onTaskFailed, this, "Connection restored with user task failed", _1)
                    );

                    m_taskManager.startTask(uid);
                }
                else {
                    auto [uid, connectionDownWithUserPacket] = PacketFactory::getConnectionDownWithUserPacket(partner->getNicknameHash());

                    m_taskManager.createTask(uid, 1500ms, 5,
                        std::bind(&Server::sendPacketTask, this, connectionDownWithUserPacket, PacketType::CONNECTION_DOWN_WITH_USER, user->getEndpoint()),
                        std::bind(&Server::onTaskCompleted, this, _1),
                        std::bind(&Server::onTaskFailed, this, "Connection restored with user task failed", _1)
                    );

                    m_taskManager.startTask(uid);
                }
            }
        }
    }
    else {
        auto packet = PacketFactory::getReconnectionResultPacket(false, uid, senderNicknameHash, token);
        m_networkController.send(packet, static_cast<uint32_t>(PacketType::RECONNECT_RESULT), endpointFrom);
    }
}

void Server::handleGetFriendInfo(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string uid = jsonObject[UID];
        std::string nicknameHash = jsonObject[NICKNAME_HASH];
        std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH];

        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_nicknameHashToUser.contains(nicknameHash)) {
            auto& requestedUser = m_nicknameHashToUser.at(nicknameHash);

            if (m_nicknameHashToUser.contains(senderNicknameHash)) {
                auto packet = PacketFactory::getUserInfoResultPacket(true, uid, requestedUser->getNicknameHash(), requestedUser->getPublicKey());
                m_networkController.send(packet, static_cast<uint32_t>(PacketType::GET_USER_INFO_RESULT), endpointFrom);
            }
        }
        else {
            auto packet = PacketFactory::getUserInfoResultPacket(false, uid, nicknameHash);
            m_networkController.send(packet, static_cast<uint32_t>(PacketType::GET_USER_INFO_RESULT), endpointFrom);
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in get friend info packet: {}", e.what());
    }
}

void Server::handleStartOutgoingCall(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string uid = jsonObject[UID].get<std::string>();
        std::string receiverNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH].get<std::string>();
        std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();

        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_nicknameHashToUser.contains(receiverNicknameHash)) {
            auto& receiver = m_nicknameHashToUser.at(receiverNicknameHash);

            ConfirmationKey key(uid, receiver->getNicknameHash(), receiver->getToken());

            addPendingAction(key, [this, receiverNicknameHash, senderNicknameHash]() {
                    std::lock_guard<std::mutex> lock(m_mutex);

                    if (m_nicknameHashToUser.contains(receiverNicknameHash)) {
                        auto receiver = m_nicknameHashToUser.at(receiverNicknameHash);

                        if (m_nicknameHashToUser.contains(senderNicknameHash)) {
                            auto sender = m_nicknameHashToUser.at(senderNicknameHash);

                            std::shared_ptr<PendingCall> pendingCall = std::make_shared<PendingCall>(sender, receiver,
                                [this, receiver, sender]() {
                                    std::lock_guard<std::mutex> lock(m_mutex);
                                    auto outgoingPendingCall = sender->getOutgoingPendingCall();
                                    if (outgoingPendingCall && outgoingPendingCall->getReceiver()->getNicknameHash() == receiver->getNicknameHash()) {
                                        resetOutgoingPendingCall(sender);
                                        removeIncomingPendingCall(receiver, outgoingPendingCall);
                                    }
                                }
                            );

                            m_pendingCalls.emplace(pendingCall);
                            sender->setOutgoingPendingCall(pendingCall);
                            receiver->addIncomingPendingCall(pendingCall);

                            std::string senderPrefix = senderNicknameHash.length() >= 5 ? senderNicknameHash.substr(0, 5) : senderNicknameHash;
                            std::string receiverPrefix = receiverNicknameHash.length() >= 5 ? receiverNicknameHash.substr(0, 5) : receiverNicknameHash;
                            LOG_INFO("Call initiated from {} to {}", senderPrefix, receiverPrefix);
                        }
                    }
                }
            );

            if (!receiver->isConnectionDown()) {
                m_networkController.send(toBytes(jsonObject.dump()),
                    static_cast<uint32_t>(PacketType::CALLING_BEGIN),
                    receiver->getEndpoint()
                );
            }
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in start outgoing call packet: {}", e.what());
    }
}

void Server::handleStopOutgoingCall(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string uid = jsonObject[UID].get<std::string>();
        std::string receiverNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH].get<std::string>();
        std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();

        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_nicknameHashToUser.contains(receiverNicknameHash)) {
            auto& receiver = m_nicknameHashToUser.at(receiverNicknameHash);

            ConfirmationKey key(uid, receiver->getNicknameHash(), receiver->getToken());

            addPendingAction(key, [this, receiverNicknameHash, senderNicknameHash]() {
                    std::lock_guard<std::mutex> lock(m_mutex);

                    if (m_nicknameHashToUser.contains(receiverNicknameHash)) {
                        auto& receiver = m_nicknameHashToUser.at(receiverNicknameHash);

                        if (m_nicknameHashToUser.contains(senderNicknameHash)) {
                            auto& sender = m_nicknameHashToUser.at(senderNicknameHash);

                            auto outgoingPendingCall = sender->getOutgoingPendingCall();
                            if (outgoingPendingCall && outgoingPendingCall->getReceiver()->getNicknameHash() == receiverNicknameHash) {
                                resetOutgoingPendingCall(sender);
                                removeIncomingPendingCall(receiver, outgoingPendingCall);

                                std::string senderPrefix = senderNicknameHash.length() >= 5 ? senderNicknameHash.substr(0, 5) : senderNicknameHash;
                                std::string receiverPrefix = receiverNicknameHash.length() >= 5 ? receiverNicknameHash.substr(0, 5) : receiverNicknameHash;
                                LOG_INFO("Calling stopped: {} -> {}", senderPrefix, receiverPrefix);
                            }
                        }
                    }
                }
            );

            if (receiver->isConnectionDown()) {
                auto packet = PacketFactory::getConfirmationPacket(uid, senderNicknameHash);

                m_networkController.send(packet,
                    static_cast<uint32_t>(PacketType::CONFIRMATION),
                    endpointFrom
                );
            }
            else {
                m_networkController.send(toBytes(jsonObject.dump()),
                    static_cast<uint32_t>(PacketType::CALLING_END),
                    receiver->getEndpoint()
                );
            }
        }
        else {
            auto packet = PacketFactory::getConfirmationPacket(uid, senderNicknameHash);

            m_networkController.send(packet,
                static_cast<uint32_t>(PacketType::CONFIRMATION),
                endpointFrom
            );
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in stop calling packet: {}", e.what());
    }
}

void Server::handleAcceptCall(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string uid = jsonObject[UID].get<std::string>();
        std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();
        std::string receiverNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH].get<std::string>();

        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_nicknameHashToUser.contains(receiverNicknameHash)) {
            auto& receiver = m_nicknameHashToUser.at(receiverNicknameHash);

            ConfirmationKey key(uid, receiver->getNicknameHash(), receiver->getToken());

            addPendingAction(key, [this, receiverNicknameHash, senderNicknameHash]() {
                    std::lock_guard<std::mutex> lock(m_mutex);

                    if (m_nicknameHashToUser.contains(receiverNicknameHash)) {
                        auto& receiver = m_nicknameHashToUser.at(receiverNicknameHash);

                        if (m_nicknameHashToUser.contains(senderNicknameHash)) {
                            auto& sender = m_nicknameHashToUser.at(senderNicknameHash);

                            auto foundPendingCall = receiver->getOutgoingPendingCall();

                            if (foundPendingCall && foundPendingCall->getReceiver()->getNicknameHash() == senderNicknameHash) {
                                resetOutgoingPendingCall(receiver);
                                removeIncomingPendingCall(sender, foundPendingCall);

                                auto call = std::make_shared<Call>(receiver, sender);
                                m_calls.emplace(call);
                                receiver->setCall(call);
                                sender->setCall(call);

                                auto declineIncomingPendingCalls = [this](const UserPtr& busyUser) {
                                    if (!busyUser) {
                                        return;
                                    }

                                    auto incomingCalls = busyUser->getIncomingPendingCalls();
                                    for (const auto& pendingCall : incomingCalls) {
                                        auto initiator = pendingCall->getInitiator();
                                        if (!initiator) {
                                            removeIncomingPendingCall(busyUser, pendingCall);
                                            continue;
                                        }

                                        resetOutgoingPendingCall(initiator);
                                        removeIncomingPendingCall(busyUser, pendingCall);

                                        if (m_nicknameHashToUser.contains(initiator->getNicknameHash()) && !initiator->isConnectionDown()) {
                                            auto [uid, declinePacket] = PacketFactory::getCallDeclinedPacket(busyUser->getNicknameHash(), initiator->getNicknameHash());
                                            m_taskManager.createTask(uid, 1500ms, 3,
                                                std::bind(&Server::sendPacketTask, this, declinePacket, PacketType::CALL_DECLINE, initiator->getEndpoint()),
                                                std::bind(&Server::onTaskCompleted, this, _1),
                                                std::bind(&Server::onTaskFailed, this, "Call decline notification task failed", _1)
                                            );
                                            m_taskManager.startTask(uid);
                                        }
                                    }
                                };

                                declineIncomingPendingCalls(receiver);
                                declineIncomingPendingCalls(sender);

                                std::string senderPrefix = senderNicknameHash.length() >= 5 ? senderNicknameHash.substr(0, 5) : senderNicknameHash;
                                std::string receiverPrefix = receiverNicknameHash.length() >= 5 ? receiverNicknameHash.substr(0, 5) : receiverNicknameHash;
                                LOG_INFO("Call accepted: {} -> {}", receiverPrefix, senderPrefix);
                            }
                        }
                    }
                }
            );

            if (!receiver->isConnectionDown()) {
                m_networkController.send(toBytes(jsonObject.dump()),
                    static_cast<uint32_t>(PacketType::CALL_ACCEPT),
                    receiver->getEndpoint()
                );
            }
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in call accepted packet: {}", e.what());
    }
}

void Server::handleDeclineCall(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string uid = jsonObject[UID].get<std::string>();
        std::string receiverNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH].get<std::string>();
        std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();

        std::unique_lock<std::mutex> lock(m_mutex);

        auto onConfirmationReceived = [this, receiverNicknameHash, senderNicknameHash]() {
            std::lock_guard<std::mutex> lock(m_mutex);

            if (m_nicknameHashToUser.contains(receiverNicknameHash)) {
                auto& receiver = m_nicknameHashToUser.at(receiverNicknameHash);

                if (m_nicknameHashToUser.contains(senderNicknameHash)) {
                    auto& sender = m_nicknameHashToUser.at(senderNicknameHash);

                    PendingCallPtr foundPendingCall = nullptr;
                    auto incomingCalls = receiver->getIncomingPendingCalls();
                    for (auto& pendingCall : incomingCalls) {
                        if (pendingCall->getInitiator()->getNicknameHash() == senderNicknameHash) {
                            foundPendingCall = pendingCall;
                            break;
                        }
                    }

                    if (foundPendingCall) {
                        resetOutgoingPendingCall(sender);
                        removeIncomingPendingCall(receiver, foundPendingCall);

                        std::string senderPrefix = senderNicknameHash.length() >= 5 ? senderNicknameHash.substr(0, 5) : senderNicknameHash;
                        std::string receiverPrefix = receiverNicknameHash.length() >= 5 ? receiverNicknameHash.substr(0, 5) : receiverNicknameHash;
                        LOG_INFO("Call declined: {} -> {}", senderPrefix, receiverPrefix);
                    }
                }
            }
        };

        if (m_nicknameHashToUser.contains(receiverNicknameHash)) {
            auto& receiver = m_nicknameHashToUser.at(receiverNicknameHash);

            if (receiver->isConnectionDown()) {
                lock.unlock();
                onConfirmationReceived();
                lock.lock();

                auto packet = PacketFactory::getConfirmationPacket(uid, senderNicknameHash);

                m_networkController.send(packet,
                    static_cast<uint32_t>(PacketType::CONFIRMATION),
                    endpointFrom
                );
            }
            else {
                ConfirmationKey key(uid, receiver->getNicknameHash(), receiver->getToken());
                addPendingAction(key, onConfirmationReceived);

                m_networkController.send(
                    toBytes(jsonObject.dump()),
                    static_cast<uint32_t>(PacketType::CALL_DECLINE),
                    receiver->getEndpoint()
                );
            }
        }
        else {
            lock.unlock();
            onConfirmationReceived();
            lock.lock();

            auto packet = PacketFactory::getConfirmationPacket(uid, senderNicknameHash);

            m_networkController.send(packet,
                static_cast<uint32_t>(PacketType::CONFIRMATION),
                endpointFrom
            );
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in call declined packet: {}", e.what());
    }
}

void Server::handleEndCall(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string uid = jsonObject[UID].get<std::string>();
        std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();
        std::string receiverNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH].get<std::string>();

        auto onConfirmationReceived = [this, receiverNicknameHash, senderNicknameHash]() {
            std::lock_guard<std::mutex> lock(m_mutex);

            if (m_nicknameHashToUser.contains(receiverNicknameHash)) {
                auto& receiver = m_nicknameHashToUser.at(receiverNicknameHash);

                if (m_nicknameHashToUser.contains(senderNicknameHash)) {
                    auto& sender = m_nicknameHashToUser.at(senderNicknameHash);

                    if (sender->isInCall() && receiver->isInCall()) {
                        std::string senderPrefix = senderNicknameHash.length() >= 5 ? senderNicknameHash.substr(0, 5) : senderNicknameHash;
                        std::string receiverPrefix = receiverNicknameHash.length() >= 5 ? receiverNicknameHash.substr(0, 5) : receiverNicknameHash;
                        LOG_INFO("Call ended: {} ended call with {}", senderPrefix, receiverPrefix);

                        m_calls.erase(sender->getCall());
                        sender->resetCall();
                        receiver->resetCall();
                    }
                }
            }
            };

        std::unique_lock<std::mutex> lock(m_mutex);

        if (m_nicknameHashToUser.contains(receiverNicknameHash)) {
            auto& receiver = m_nicknameHashToUser.at(receiverNicknameHash);

            if (receiver->isConnectionDown()) {
                lock.unlock();
                onConfirmationReceived();
                lock.lock();

                auto packet = PacketFactory::getConfirmationPacket(uid, senderNicknameHash);

                m_networkController.send(packet,
                    static_cast<uint32_t>(PacketType::CONFIRMATION),
                    endpointFrom
                );
            }
            else {
                ConfirmationKey key(uid, receiver->getNicknameHash(), receiver->getToken());

                addPendingAction(key, onConfirmationReceived);

                m_networkController.send(toBytes(jsonObject.dump()),
                    static_cast<uint32_t>(PacketType::CALL_END),
                    receiver->getEndpoint()
                );
            }
        }
        else {
            lock.unlock();
            onConfirmationReceived();
            lock.lock();

            auto packet = PacketFactory::getConfirmationPacket(uid, senderNicknameHash);

            m_networkController.send(packet,
                static_cast<uint32_t>(PacketType::CONFIRMATION),
                endpointFrom
            );
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in end call packet: {}", e.what());
    }
}

void Server::handleVoice(const unsigned char* data, int size, const asio::ip::udp::endpoint& endpointFrom) {
    UserPtr user;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_endpointToUser.contains(endpointFrom)) {
            user = m_endpointToUser.at(endpointFrom);
        }
        else {
            return;
        }
    }

    if (user->isInCall()) {
        auto callPartner = user->getCallPartner();

        if (callPartner) {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_nicknameHashToUser.contains(callPartner->getNicknameHash()) && !callPartner->isConnectionDown()) {
                m_networkController.send(data, size, static_cast<uint32_t>(PacketType::VOICE), callPartner->getEndpoint());
            }
        }
    }
}

void Server::handleScreen(const unsigned char* data, int size, const asio::ip::udp::endpoint& endpointFrom) {
    UserPtr user;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_endpointToUser.contains(endpointFrom)) {
            user = m_endpointToUser.at(endpointFrom);
        }
        else {
            return;
        }
    }

    if (user->isInCall()) {
        auto callPartner = user->getCallPartner();

        if (callPartner) {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_nicknameHashToUser.contains(callPartner->getNicknameHash()) && !callPartner->isConnectionDown()) {
                m_networkController.send(data, size, static_cast<uint32_t>(PacketType::SCREEN), callPartner->getEndpoint());
            }
        }
    }
}

void Server::handleCamera(const unsigned char* data, int size, const asio::ip::udp::endpoint& endpointFrom) {
    UserPtr user;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_endpointToUser.contains(endpointFrom)) {
            user = m_endpointToUser.at(endpointFrom);
        }
        else {
            return;
        }
    }

    if (user->isInCall()) {
        auto callPartner = user->getCallPartner();

        if (callPartner) {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_nicknameHashToUser.contains(callPartner->getNicknameHash()) && !callPartner->isConnectionDown()) {
                m_networkController.send(data, size, static_cast<uint32_t>(PacketType::CAMERA), callPartner->getEndpoint());
            }
        }
    }
}

void Server::processUserLogout(const UserPtr& user) {
    if (!user) return;
    
    std::string nicknameHash = user->getNicknameHash();
    std::string nicknameHashPrefix = nicknameHash.length() >= 5 ? nicknameHash.substr(0, 5) : nicknameHash;
    LOG_INFO("User logout: {}", nicknameHashPrefix);
    
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_nicknameHashToUser.contains(nicknameHash)) return;

    if (user->isInCall()) {
        UserPtr callPartner = user->getCallPartner();

        if (callPartner && m_nicknameHashToUser.contains(callPartner->getNicknameHash())) {
            std::string callPartnerPrefix = callPartner->getNicknameHash().length() >= 5 ? callPartner->getNicknameHash().substr(0, 5) : callPartner->getNicknameHash();
            LOG_INFO("Call ended due to logout: {} ended call with {}", nicknameHashPrefix, callPartnerPrefix);

            if (!callPartner->isConnectionDown()) {
                auto [uid, packet] = PacketFactory::getUserLogoutPacket(user->getNicknameHash());
                m_taskManager.createTask(uid, 1500ms, 5,
                    std::bind(&Server::sendPacketTask, this, packet, PacketType::USER_LOGOUT, callPartner->getEndpoint()),
                    std::bind(&Server::onTaskCompleted, this, _1),
                    std::bind(&Server::onTaskFailed, this, "User logout task failed", _1)
                );

                m_taskManager.startTask(uid);
            }

            callPartner->resetCall();
        }

        if (auto call = user->getCall(); call)
            m_calls.erase(call);

        user->resetCall();
    }

    if (user->hasOutgoingPendingCall()) {
        auto outgoingPendingCall = user->getOutgoingPendingCall();
        auto pendingCallPartner = user->getOutgoingPendingCallPartner();

        if (pendingCallPartner && m_nicknameHashToUser.contains(pendingCallPartner->getNicknameHash())) {
            std::string pendingCallPartnerPrefix = pendingCallPartner->getNicknameHash().length() >= 5 ? pendingCallPartner->getNicknameHash().substr(0, 5) : pendingCallPartner->getNicknameHash();
            LOG_INFO("Outgoing call ended due to logout: {} -> {}", nicknameHashPrefix, pendingCallPartnerPrefix);
            LOG_INFO("Incoming call ended due to logout: {} -> {}", pendingCallPartnerPrefix, nicknameHashPrefix);

            if (!pendingCallPartner->isConnectionDown()) {
                auto [uid, packet] = PacketFactory::getUserLogoutPacket(user->getNicknameHash());
                m_taskManager.createTask(uid, 1500ms, 5,
                    std::bind(&Server::sendPacketTask, this, packet, PacketType::USER_LOGOUT, pendingCallPartner->getEndpoint()),
                    std::bind(&Server::onTaskCompleted, this, _1),
                    std::bind(&Server::onTaskFailed, this, "User logout task failed", _1)
                );

                m_taskManager.startTask(uid);
            }

            removeIncomingPendingCall(pendingCallPartner, outgoingPendingCall);
        }

        resetOutgoingPendingCall(user);
    }

    auto incomingCalls = user->getIncomingPendingCalls();
    for (auto& pendingCall : incomingCalls) {
        auto pendingCallPartner = pendingCall->getInitiator();

        if (pendingCallPartner && m_nicknameHashToUser.contains(pendingCallPartner->getNicknameHash())) {
            std::string pendingCallPartnerPrefix = pendingCallPartner->getNicknameHash().length() >= 5 ? pendingCallPartner->getNicknameHash().substr(0, 5) : pendingCallPartner->getNicknameHash();
            LOG_INFO("Incoming call ended due to logout: {} -> {}", pendingCallPartnerPrefix, nicknameHashPrefix);
            LOG_INFO("Outgoing call ended due to logout: {} -> {}", pendingCallPartnerPrefix, nicknameHashPrefix);

            if (!pendingCallPartner->isConnectionDown()) {
                auto [uid, packet] = PacketFactory::getUserLogoutPacket(user->getNicknameHash());
                m_taskManager.createTask(uid, 1500ms, 5,
                    std::bind(&Server::sendPacketTask, this, packet, PacketType::USER_LOGOUT, pendingCallPartner->getEndpoint()),
                    std::bind(&Server::onTaskCompleted, this, _1),
                    std::bind(&Server::onTaskFailed, this, "User logout task failed", _1)
                );

                m_taskManager.startTask(uid);
            }

            resetOutgoingPendingCall(pendingCallPartner);
        }

        removeIncomingPendingCall(user, pendingCall);
    }
    user->resetAllPendingCalls();

    clearPendingActionsForUser(user->getNicknameHash());

    m_networkController.removePingMonitoring(user->getEndpoint());
    m_endpointToUser.erase(user->getEndpoint());
    m_nicknameHashToUser.erase(user->getNicknameHash());
}

bool Server::resetOutgoingPendingCall(const UserPtr& user) {
    if (user->hasOutgoingPendingCall()) {
        auto pendingCall = user->getOutgoingPendingCall();

        if (m_pendingCalls.contains(pendingCall)) 
            m_pendingCalls.erase(pendingCall);
        
        user->resetOutgoingPendingCall();

        return true;
    }

    return false;
}

void Server::removeIncomingPendingCall(const UserPtr& user, const PendingCallPtr& pendingCall) {
    if (user->hasIncomingPendingCall(pendingCall)) {
        if (m_pendingCalls.contains(pendingCall))
            m_pendingCalls.erase(pendingCall);
        
        user->removeIncomingPendingCall(pendingCall);
    }
}

void Server::addPendingAction(const ConfirmationKey& key, std::function<void()> action)
{
    if (!m_pendingActions.contains(key)) {
        m_pendingActions.emplace(key, std::move(action));
        m_nicknameHashToConfirmationKeys[key.nicknameHash].insert(key);
    }
}

void Server::removePendingAction(const ConfirmationKey& key)
{
    if (m_pendingActions.contains(key)) {
        m_pendingActions.erase(key);
        auto it = m_nicknameHashToConfirmationKeys.find(key.nicknameHash);
        if (it != m_nicknameHashToConfirmationKeys.end()) {
            it->second.erase(key);
            if (it->second.empty()) {
                m_nicknameHashToConfirmationKeys.erase(it);
            }
        }
    }
}

void Server::clearPendingActionsForUser(const std::string& nicknameHash)
{
    auto it = m_nicknameHashToConfirmationKeys.find(nicknameHash);
    if (it != m_nicknameHashToConfirmationKeys.end()) {
        for (const auto& key : it->second) {
            m_pendingActions.erase(key);
        }
        m_nicknameHashToConfirmationKeys.erase(it);
    }
}

void Server::sendPacketTask(const std::vector<unsigned char>& packet, PacketType type, const asio::ip::udp::endpoint& endpoint) {
    m_networkController.send(packet, static_cast<uint32_t>(type), endpoint);
}

void Server::onTaskCompleted(std::optional<nlohmann::json> completionContext) {
}

void Server::onTaskFailed(const std::string& message, std::optional<nlohmann::json> failureContext) {
    LOG_ERROR(message);
}
} // namespace server