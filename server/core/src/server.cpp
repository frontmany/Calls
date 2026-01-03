#include "server.h"
#include "packetFactory.h"
#include "jsonType.h"
#include "utilities/crypto.h"
#include "utilities/logger.h"
#include "pendingCall.h"

using namespace std::chrono_literals;
using namespace callifornia;

namespace callifornia
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
            if (m_endpointToUser.contains(endpoint)) {
                auto& user = m_endpointToUser.at(endpoint);

                if (user) {
                    user->setConnectionDown(true);

                    if (user->hasOutgoingPendingCall()) {
                        auto outgoingPendingCall = user->getOutgoingPendingCall();
                        auto receiver = outgoingPendingCall->getReceiver();
                        
                        if (m_nicknameHashToUser.contains(receiver->getNicknameHash()) && !receiver->isConnectionDown()) {
                            auto [uid, connectionDownPacket] = PacketFactory::getConnectionDownWithUserPacket(user->getNicknameHash());
                            
                            m_taskManager.createTask(uid, 1500ms, 3,
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
                            
                            m_taskManager.createTask(uid, 1500ms, 3,
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

        if (m_packetHandlers.contains(type)) {
            m_packetHandlers[type](jsonObject, endpointFrom);
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
    LOG_INFO("Server stopped");
}

void Server::redirect(const nlohmann::json& jsonObject, PacketType type) {
    const std::string& receiverNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH].get<std::string>();

    if (m_nicknameHashToUser.contains(receiverNicknameHash)) {
        auto& user = m_nicknameHashToUser.at(receiverNicknameHash);

        auto packet = toBytes(jsonObject.dump());
        m_networkController.send(packet, static_cast<uint32_t>(type), user->getEndpoint());
    }
}

void Server::handleConfirmation(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    std::string uid = jsonObject[UID].get<std::string>();
    std::string receiverNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH].get<std::string>();
    
    m_taskManager.completeTask(uid);
}

void Server::handleAuthorization(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string uid = jsonObject[UID].get<std::string>();
        std::string nicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();
        CryptoPP::RSA::PublicKey publicKey = crypto::deserializePublicKey(jsonObject[PUBLIC_KEY]);
        
        bool authorized = false;
        std::string token = "";

        if (m_nicknameHashToUser.contains(nicknameHash)) {
            LOG_WARN("Authorization failed - nickname already taken: {}", nicknameHash);
        }
        else {
            token = crypto::generateToken();

            UserPtr user = std::make_shared<User>(nicknameHash, token, publicKey, endpointFrom,
                [this, nicknameHash]() {
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
        std::string receiverNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH].get<std::string>();
        std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();

        LOG_INFO("Call initiated from {} to {}", senderNicknameHash, receiverNicknameHash);

        if (m_nicknameHashToUser.contains(receiverNicknameHash)) {
            auto& userReceiver = m_nicknameHashToUser.at(receiverNicknameHash);
            
            if (m_nicknameHashToUser.contains(senderNicknameHash)) {
                auto& userSender = m_nicknameHashToUser.at(senderNicknameHash);
                
                std::shared_ptr<PendingCall> pendingCall = std::make_shared<PendingCall>(userSender, userReceiver, 
                    [this, userSender, userReceiver, pendingCall]() {
                        resetOutgoingPendingCall(userSender);
                        removeIncomingPendingCall(userReceiver, pendingCall);
                    });
                m_pendingCalls.emplace(pendingCall);
                
                userSender->setOutgoingPendingCall(pendingCall);
                userReceiver->addIncomingPendingCall(pendingCall);

                if (!userReceiver->isConnectionDown()) {
                    m_networkController.send(toBytes(jsonObject.dump()),
                        static_cast<uint32_t>(PacketType::CALLING_BEGIN),
                        userReceiver->getEndpoint()
                    );
                }
            }
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in start outgoing call packet: {}", e.what());
    }
}

void Server::handleStopOutgoingCall(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string receiverNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH].get<std::string>();
        std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();

        if (m_nicknameHashToUser.contains(receiverNicknameHash)) {
            auto& userReceiver = m_nicknameHashToUser.at(receiverNicknameHash);

            if (m_nicknameHashToUser.contains(senderNicknameHash)) {
                auto& userSender = m_nicknameHashToUser.at(senderNicknameHash);

                auto outgoingPendingCall = userSender->getOutgoingPendingCall();
                if (outgoingPendingCall && outgoingPendingCall->getReceiver()->getNicknameHash() == receiverNicknameHash) {
                    resetOutgoingPendingCall(userSender);
                    removeIncomingPendingCall(userReceiver, outgoingPendingCall);

                    if (!userReceiver->isConnectionDown()) {
                        m_networkController.send(toBytes(jsonObject.dump()),
                            static_cast<uint32_t>(PacketType::CALLING_END),
                            userReceiver->getEndpoint()
                        );
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in stop calling packet: {}", e.what());
    }
}

void Server::handleAcceptCall(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();
        std::string receiverNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH].get<std::string>();

        if (m_nicknameHashToUser.contains(receiverNicknameHash)) {
            auto& userReceiver = m_nicknameHashToUser.at(receiverNicknameHash);

            if (m_nicknameHashToUser.contains(senderNicknameHash)) {
                auto& userSender = m_nicknameHashToUser.at(senderNicknameHash);

                PendingCallPtr foundPendingCall = nullptr;
                auto incomingCalls = userReceiver->getIncomingPendingCalls();
                for (auto& pendingCall : incomingCalls) {
                    if (pendingCall->getInitiator()->getNicknameHash() == senderNicknameHash) {
                        foundPendingCall = pendingCall;
                        break;
                    }
                }

                if (foundPendingCall) {
                    resetOutgoingPendingCall(userSender);
                    removeIncomingPendingCall(userReceiver, foundPendingCall);

                    auto call = std::make_shared<Call>(userSender, userReceiver);
                    userSender->setCall(call);
                    userReceiver->setCall(call);

                    if (!userReceiver->isConnectionDown()) {
                        m_networkController.send(toBytes(jsonObject.dump()),
                            static_cast<uint32_t>(PacketType::CALL_ACCEPT),
                            userReceiver->getEndpoint()
                        );
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in call accepted packet: {}", e.what());
    }
}

void Server::handleDeclineCall(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string receiverNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH].get<std::string>();
        std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();

        if (m_nicknameHashToUser.contains(receiverNicknameHash)) {
            auto& userReceiver = m_nicknameHashToUser.at(receiverNicknameHash);

            if (m_nicknameHashToUser.contains(senderNicknameHash)) {
                auto& userSender = m_nicknameHashToUser.at(senderNicknameHash);

                PendingCallPtr foundPendingCall = nullptr;
                auto incomingCalls = userReceiver->getIncomingPendingCalls();
                for (auto& pendingCall : incomingCalls) {
                    if (pendingCall->getInitiator()->getNicknameHash() == senderNicknameHash) {
                        foundPendingCall = pendingCall;
                        break;
                    }
                }

                if (foundPendingCall) {
                    resetOutgoingPendingCall(userSender);
                    removeIncomingPendingCall(userReceiver, foundPendingCall);
                
                    if (!userReceiver->isConnectionDown()) {
                        m_networkController.send(
                            toBytes(jsonObject.dump()),
                            static_cast<uint32_t>(PacketType::CALL_DECLINE),
                            userReceiver->getEndpoint()
                        );
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in call declined packet: {}", e.what());
    }
}

void Server::handleEndCall(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();
        std::string receiverNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH].get<std::string>();

        if (m_nicknameHashToUser.contains(receiverNicknameHash)) {
            auto& userReceiver = m_nicknameHashToUser.at(receiverNicknameHash);

            if (m_nicknameHashToUser.contains(senderNicknameHash)) {
                auto& userSender = m_nicknameHashToUser.at(senderNicknameHash);

                if (userSender->isInCall() && userReceiver->isInCall()) {
                    m_calls.erase(userSender->getCall());
                    userSender->resetCall();
                    userReceiver->resetCall();

                    if (!userReceiver->isConnectionDown()) {
                        m_networkController.send(toBytes(jsonObject.dump()),
                            static_cast<uint32_t>(PacketType::CALLING_END),
                            userReceiver->getEndpoint()
                        );
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in end call packet: {}", e.what());
    }
}

void Server::handleVoice(const unsigned char* data, int size, const asio::ip::udp::endpoint& endpointFrom) {
    if (m_endpointToUser.contains(endpointFrom)) {
        UserPtr user = m_endpointToUser.at(endpointFrom);

        if (user->isInCall()) {
            auto callPartner = user->getCallPartner();

            if (!callPartner->isConnectionDown()) {
                m_networkController.send(data, size, static_cast<uint32_t>(PacketType::VOICE), callPartner->getEndpoint());
            }
        }
    }
}

void Server::handleScreen(const unsigned char* data, int size, const asio::ip::udp::endpoint& endpointFrom) {
    if (m_endpointToUser.contains(endpointFrom)) {
        UserPtr user = m_endpointToUser.at(endpointFrom);

        if (user->isInCall()) {
            auto callPartner = user->getCallPartner();

            if (!callPartner->isConnectionDown()) {
                m_networkController.send(data, size, static_cast<uint32_t>(PacketType::SCREEN), callPartner->getEndpoint());
            }
        }
    }
}

void Server::handleCamera(const unsigned char* data, int size, const asio::ip::udp::endpoint& endpointFrom) {
    if (m_endpointToUser.contains(endpointFrom)) {
        UserPtr user = m_endpointToUser.at(endpointFrom);

        if (user->isInCall()) {
            auto callPartner = user->getCallPartner();

            if (!callPartner->isConnectionDown()) {
                m_networkController.send(data, size, static_cast<uint32_t>(PacketType::CAMERA), callPartner->getEndpoint());
            }
        }
    }
}

void Server::processUserLogout(const UserPtr& user) {
    if (!user || !m_nicknameHashToUser.contains(user->getNicknameHash())) return;

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
} // namespace callifornia