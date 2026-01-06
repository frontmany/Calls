#include "server.h"
#include "packetFactory.h"
#include "jsonType.h"
#include "utilities/crypto.h"
#include "utilities/logger.h"
#include "pendingCall.h"

using namespace std::chrono_literals;
using namespace server;
using namespace server::utilities;

namespace server
{
namespace
{
    std::vector<unsigned char> toBytes(const std::string& value) {
        return std::vector<unsigned char>(value.begin(), value.end());
    }
} 

Server::Server(const std::string& port)
{
    m_networkController.init(port,
        [this](const unsigned char* data, int size, uint32_t type, const asio::ip::udp::endpoint& endpointFrom) { onReceive(data, size, type, endpointFrom); },
        [this](const asio::ip::udp::endpoint& endpoint) {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_endpointToUser.contains(endpoint)) {
                auto& user = m_endpointToUser.at(endpoint);

                if (user) {
                    user->setConnectionDown(true);
                    clearPendingActionsForUser(user->getNicknameHash());

                    if (user->hasOutgoingPendingCall()) {
                        auto outgoingPendingCall = user->getOutgoingPendingCall();
                        auto receiver = outgoingPendingCall->getReceiver();
                        
                        if (m_nicknameHashToUser.contains(receiver->getNicknameHash()) && !receiver->isConnectionDown()) {
                            auto [uid, connectionDownPacket] = PacketFactory::getConnectionDownWithUserPacket(user->getNicknameHash());
                            
                            m_taskManager.createTask(uid, 1500ms, 5,
                                [this, connectionDownPacket, receiver]() {
                                    m_networkController.send(connectionDownPacket, static_cast<uint32_t>(PacketType::CONNECTION_DOWN_WITH_USER), receiver->getEndpoint());
                                },
                                [this](std::optional<nlohmann::json> completionContext) {
                                    LOG_INFO("Connection down with user task completed successfully");
                                },
                                [this](std::optional<nlohmann::json> failureContext) {
                                    LOG_ERROR("Connection down with user task failed");
                                }
                            );
                            
                            m_taskManager.startTask(uid);
                        }
                    }

                    auto incomingCalls = user->getIncomingPendingCalls();
                    for (auto& pendingCall : incomingCalls) {
                        auto initiator = pendingCall->getInitiator();
                        
                        if (m_nicknameHashToUser.contains(initiator->getNicknameHash()) && !initiator->isConnectionDown()) {
                            auto [uid, connectionDownPacket] = PacketFactory::getConnectionDownWithUserPacket(user->getNicknameHash());
                            
                            m_taskManager.createTask(uid, 1500ms, 5,
                                [this, connectionDownPacket, initiator]() {
                                    m_networkController.send(connectionDownPacket, static_cast<uint32_t>(PacketType::CONNECTION_DOWN_WITH_USER), initiator->getEndpoint());
                                },
                                [this](std::optional<nlohmann::json> completionContext) {
                                    LOG_INFO("Connection down with user task completed successfully");
                                },
                                [this](std::optional<nlohmann::json> failureContext) {
                                    LOG_ERROR("Connection down with user task failed");
                                }
                            );
                            
                            m_taskManager.startTask(uid);
                        }
                    }

                    if (user->isInCall()) {
                        auto callPartner = user->getCallPartner();
                        
                        if (callPartner && m_nicknameHashToUser.contains(callPartner->getNicknameHash()) && !callPartner->isConnectionDown()) {
                            auto [uid, connectionDownPacket] = PacketFactory::getConnectionDownWithUserPacket(user->getNicknameHash());
                            
                            m_taskManager.createTask(uid, 1500ms, 5,
                                [this, connectionDownPacket, callPartner]() {
                                    m_networkController.send(connectionDownPacket, static_cast<uint32_t>(PacketType::CONNECTION_DOWN_WITH_USER), callPartner->getEndpoint());
                                },
                                [this](std::optional<nlohmann::json> completionContext) {
                                    LOG_INFO("Connection down with user task completed successfully");
                                },
                                [this](std::optional<nlohmann::json> failureContext) {
                                    LOG_ERROR("Connection down with user task failed");
                                }
                            );
                            
                            m_taskManager.startTask(uid);
                        }
                    }
                }
            }
        },
        [this](const asio::ip::udp::endpoint& endpoint) {
            LOG_INFO("Server detectecd restored ping");
        }
    );

    m_packetHandlers.emplace(PacketType::AUTHORIZATION, [this](const nlohmann::json& json, const asio::ip::udp::endpoint& endpoint) {handleAuthorization(json, endpoint);});
    m_packetHandlers.emplace(PacketType::LOGOUT, [this](const nlohmann::json& json, const asio::ip::udp::endpoint& endpoint) {handleLogout(json, endpoint);});
    m_packetHandlers.emplace(PacketType::RECONNECT, [this](const nlohmann::json& json, const asio::ip::udp::endpoint& endpoint) {handleReconnect(json, endpoint);});
    m_packetHandlers.emplace(PacketType::GET_USER_INFO, [this](const nlohmann::json& json, const asio::ip::udp::endpoint& endpoint) {handleGetFriendInfo(json, endpoint);});
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
    LOG_INFO("Server started");
    m_networkController.run();
}

void Server::stop() {
    LOG_INFO("Server stopping...");
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_taskManager.cancelAllTasks();
    }
    
    m_networkController.stop();
    
    LOG_INFO("Server stopped");
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
    std::string uid = jsonObject[UID].get<std::string>();
    std::string receiverNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH].get<std::string>();
    std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();

    m_taskManager.completeTask(uid);

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_nicknameHashToUser.contains(senderNicknameHash)) {
        auto& sender = m_nicknameHashToUser.at(senderNicknameHash);

        ConfirmationKey key = ConfirmationKey(uid, sender->getNicknameHash(), sender->getToken());

        if (m_pendingActions.contains(key)) {
            auto& action = m_pendingActions.at(key);
            action();
        }

        removePendingAction(key);
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
            LOG_WARN("Authorization failed - nickname already taken: {}", nicknameHash);
        }
        else {
            token = crypto::generateUID();

            UserPtr user = std::make_shared<User>(nicknameHash, token, publicKey, endpointFrom,
                [this, nicknameHash]() {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    if (m_nicknameHashToUser.contains(nicknameHash)) {
                        UserPtr user = m_nicknameHashToUser.at(nicknameHash);
                        processUserLogout(user);

                        LOG_INFO("User removed due to reconnection timeout");
                    }
                }
            );

            m_endpointToUser.emplace(endpointFrom, user);
            m_nicknameHashToUser.emplace(nicknameHash, user);
            LOG_INFO("User authorized: {} from {}:{}", nicknameHash, endpointFrom.address().to_string(), endpointFrom.port());
                
            authorized = true;
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

        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_nicknameHashToUser.contains(senderNicknameHash)) {
            UserPtr user = m_nicknameHashToUser.at(senderNicknameHash);
            processUserLogout(user);

            LOG_INFO("User successfully logged out: {}", senderNicknameHash);
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

        std::vector<unsigned char> packet;
        if (reconnectionAllowed)
            packet = PacketFactory::getReconnectionResultPacket(true, uid, senderNicknameHash, token, user->isInCall());
        else
            packet = PacketFactory::getReconnectionResultPacket(false, uid, senderNicknameHash, token);
            
        m_networkController.send(packet, static_cast<uint32_t>(PacketType::RECONNECT_RESULT), endpointFrom);
        
        if (reconnectionAllowed && user->isInCall()) {
            auto partner = user->getCallPartner();

            if (m_nicknameHashToUser.contains(partner->getNicknameHash()) && !partner->isConnectionDown()) {
            auto [uid, connectionRestoredWithUserPacket] = PacketFactory::getConnectionRestoredWithUserPacket(user->getNicknameHash());
                
            m_taskManager.createTask(uid, 1500ms, 5,
                [this, connectionRestoredWithUserPacket, partner]() { m_networkController.send(connectionRestoredWithUserPacket, static_cast<uint32_t>(PacketType::CONNECTION_RESTORED_WITH_USER), partner->getEndpoint()); },
                [this](std::optional<nlohmann::json> completionContext) {
                    LOG_INFO("Connection restored with user task completed successfully");
                },
                [this](std::optional<nlohmann::json> failureContext) {
                    LOG_ERROR("Connection restored with user task failed");
                }
            );

            m_taskManager.startTask(uid);
            }
        }
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

        LOG_INFO("Call initiated from {} to {}", senderNicknameHash, receiverNicknameHash);

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

                            std::shared_ptr<PendingCall> pendingCall = std::make_shared<PendingCall>(sender, receiver,
                                [this, receiver, sender, pendingCall]() {
                                    std::lock_guard<std::mutex> lock(m_mutex);
                                    resetOutgoingPendingCall(sender);
                                    removeIncomingPendingCall(receiver, pendingCall);
                                }
                            );

                            m_pendingCalls.emplace(pendingCall);
                            sender->setOutgoingPendingCall(pendingCall);
                            receiver->addIncomingPendingCall(pendingCall);
                        }
                    }
                });

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
                            }
                        }
                    }
                });

            if (!receiver->isConnectionDown()) {
                m_networkController.send(toBytes(jsonObject.dump()),
                    static_cast<uint32_t>(PacketType::CALLING_END),
                    receiver->getEndpoint()
                );
            }
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

                                auto call = std::make_shared<Call>(sender, receiver);
                                m_calls.emplace(call);
                                sender->setCall(call);
                                receiver->setCall(call);
                            }
                        }
                    }
                });

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
                            }
                        }
                    }
                });

            if (!receiver->isConnectionDown()) {
                m_networkController.send(
                    toBytes(jsonObject.dump()),
                    static_cast<uint32_t>(PacketType::CALL_DECLINE),
                    receiver->getEndpoint()
                );
            }
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

                            if (sender->isInCall() && receiver->isInCall()) {
                                m_calls.erase(sender->getCall());
                                sender->resetCall();
                                receiver->resetCall();
                            }
                        }
                    }
                });

            if (!receiver->isConnectionDown()) {
                m_networkController.send(toBytes(jsonObject.dump()),
                    static_cast<uint32_t>(PacketType::CALLING_END),
                    receiver->getEndpoint()
                );
            }
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
    
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_nicknameHashToUser.contains(user->getNicknameHash())) return;

    if (user->isInCall()) {
        UserPtr callPartner = user->getCallPartner();

        if (callPartner && m_nicknameHashToUser.contains(callPartner->getNicknameHash())) {
            if (!callPartner->isConnectionDown()) {
                auto [uid, packet] = PacketFactory::getUserLogoutPacket(user->getNicknameHash());
                m_taskManager.createTask(uid, 1500ms, 5,
                    [this, packet, callPartner]() {m_networkController.send(packet, static_cast<uint32_t>(PacketType::USER_LOGOUT), callPartner->getEndpoint()); },
                    [this](std::optional<nlohmann::json> completionContext) {
                        LOG_INFO("User logout task completed successfully");
                    },
                    [this](std::optional<nlohmann::json> failureContext) {
                        LOG_ERROR("User logout task failed");
                    }
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
            if (!pendingCallPartner->isConnectionDown()) {
                auto [uid, packet] = PacketFactory::getUserLogoutPacket(user->getNicknameHash());
                m_taskManager.createTask(uid, 1500ms, 5,
                    [this, packet, pendingCallPartner]() {m_networkController.send(packet, static_cast<uint32_t>(PacketType::USER_LOGOUT), pendingCallPartner->getEndpoint()); },
                    [this](std::optional<nlohmann::json> completionContext) {
                        LOG_INFO("User logout task completed successfully");
                    },
                    [this](std::optional<nlohmann::json> failureContext) {
                        LOG_ERROR("User logout task failed");
                    }
                );
            }

            pendingCallPartner->removeIncomingPendingCall(outgoingPendingCall);
        }

        if (outgoingPendingCall)
            m_pendingCalls.erase(outgoingPendingCall);

        user->resetOutgoingPendingCall();
    }

    auto incomingCalls = user->getIncomingPendingCalls();
    for (auto& pendingCall : incomingCalls) {
        auto pendingCallPartner = pendingCall->getInitiator();

        if (pendingCallPartner && m_nicknameHashToUser.contains(pendingCallPartner->getNicknameHash())) {
            if (!pendingCallPartner->isConnectionDown()) {
                auto [uid, packet] = PacketFactory::getUserLogoutPacket(user->getNicknameHash());
                m_taskManager.createTask(uid, 1500ms, 5,
                    [this, packet, pendingCallPartner]() {m_networkController.send(packet, static_cast<uint32_t>(PacketType::USER_LOGOUT), pendingCallPartner->getEndpoint()); },
                    [this](std::optional<nlohmann::json> completionContext) {
                        LOG_INFO("User logout task completed successfully");
                    },
                    [this](std::optional<nlohmann::json> failureContext) {
                        LOG_ERROR("User logout task failed");
                    }
                );
            }

            pendingCallPartner->resetOutgoingPendingCall();
        }

        m_pendingCalls.erase(pendingCall);
    }
    user->resetAllPendingCalls();

    clearPendingActionsForUser(user->getNicknameHash());

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
} // namespace server