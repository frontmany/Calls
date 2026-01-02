#include "server.h"
#include "packetFactory.h"
#include "jsonType.h"
#include "utilities/crypto.h"
#include "utilities/logger.h"

CallsServer::CallsServer(const std::string& port)
{
    m_networkController.init(port,
        [this](const unsigned char* data, int size, uint32_t type, const asio::ip::udp::endpoint& endpointFrom) { onReceive(data, size, type, endpointFrom); },
        [this](const asio::ip::udp::endpoint& endpoint) {}, //TODO onConnectionDown
        [this](const asio::ip::udp::endpoint& endpoint) {} //TODO onConnectionRestored
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

void CallsServer::onReceive(const unsigned char* data, int size, uint32_t rawType, const asio::ip::udp::endpoint& endpointFrom) {
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

std::vector<unsigned char> CallsServer::toBytes(const std::string& value) {
    return std::vector<unsigned char>(value.begin(), value.end());
}

void CallsServer::run() {
    LOG_INFO("Server started");
    m_networkController.run();
}

void CallsServer::stop() {
    LOG_INFO("Server stopped");
}

void CallsServer::redirect(const nlohmann::json& jsonObject, PacketType type) {
    const std::string& receiverNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH].get<std::string>();

    if (m_nicknameHashToUser.contains(receiverNicknameHash)) {
        auto& user = m_nicknameHashToUser.at(receiverNicknameHash);

        auto packet = toBytes(jsonObject.dump());
        m_networkController.send(packet, static_cast<uint32_t>(type), user->getEndpoint());
    }
}

void CallsServer::handleConfirmation(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    std::string uid = jsonObject[UID].get<std::string>();
    m_taskManager.completeTask(uid);
}

void CallsServer::handleAuthorization(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string uid = jsonObject[UID].get<std::string>();
        std::string nicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();
        CryptoPP::RSA::PublicKey publicKey = crypto::deserializePublicKey(jsonObject[PUBLIC_KEY]);
        
        if (m_nicknameHashToUser.contains(nicknameHash)) {
            
            m_networkController.sendToClient(endpointFrom, getPacketWithUuid(uid),
                PacketType::AUTHORIZATION_RESULT
            );
            LOG_WARN("Authorization failed - nickname already taken: {}", nicknameHash);
            return;
        }

        m_networkController.sendToClient(endpointFrom, getPacketWithUuid(uid),
            PacketType::AUTHORIZE_SUCCESS
        );

        UserPtr user = std::make_shared<User>(nicknameHash, publicKey, endpointFrom, 
            [this, nicknameHash, endpointFrom]() {
                // Connection timeout callback - remove user after 2 minutes of connection down
                std::lock_guard<std::mutex> lock1(m_endpointToUserMutex);
                std::lock_guard<std::mutex> lock2(m_nicknameHashToUserAndCallsMutex);
                
                auto endpointIt = m_endpointToUser.find(endpointFrom);
                if (endpointIt != m_endpointToUser.end()) {
                    UserPtr userToErase = endpointIt->second;
                    
                    if (userToErase->isInCall()) {
                        const std::string& userInCallWithNicknameHash = userToErase->inCallWith();
                        
                        if (m_nicknameHashToUser.contains(userInCallWithNicknameHash)) {
                            auto& userInCallWith = m_nicknameHashToUser.at(userInCallWithNicknameHash);
                            
                            std::string packetData = getPacketEndCall(nicknameHash, false);
                            std::vector<unsigned char> data(packetData.begin(), packetData.end());
                            m_networkController.send(std::move(data), 
                                static_cast<uint32_t>(PacketType::END_CALL),
                                userInCallWith->getEndpoint()
                            );
                            
                            m_calls.erase(userToErase->getCall());
                            
                            userToErase->resetCall();
                            userInCallWith->resetCall();
                        }
                        else {
                            m_calls.erase(userToErase->getCall());
                        }
                    }
                    
                    m_endpointToUser.erase(endpointIt);
                    m_nicknameHashToUser.erase(nicknameHash);
                    
                    LOG_INFO("User removed due to connection timeout: {} from {}:{}", 
                        nicknameHash, endpointFrom.address().to_string(), endpointFrom.port());
                }
            });
        m_endpointToUser.emplace(endpointFrom, user);
        m_nicknameHashToUser.emplace(nicknameHash, user);
        LOG_INFO("User authorized: {} from {}:{}", nicknameHash, endpointFrom.address().to_string(), endpointFrom.port());
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in authorization packet: {}", e.what());
    }
}

void CallsServer::handleLogout(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom, bool logoutAndStop) {
    try {
        std::string uid = jsonObject[UID].get<std::string>();
        std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();
        bool needConfirmation = jsonObject[NEED_CONFIRMATION].get<bool>();

        std::lock_guard<std::mutex> lock1(m_endpointToUserMutex);
        std::lock_guard<std::mutex> lock2(m_pingResultsMutex);

        auto it = m_endpointToUser.find(endpointFrom);
        UserPtr userToErase = nullptr;

        if (it != m_endpointToUser.end()) {
            userToErase = it->second;
            m_endpointToUser.erase(it);
        }

        m_pingResults.erase(endpointFrom);

        if (userToErase) {
            if (userToErase->isInCall()) {
                const std::string& userInCallWithNicknameHash = userToErase->inCallWith();

                if (m_nicknameHashToUser.contains(userInCallWithNicknameHash)) {
                    auto& userInCallWith = m_nicknameHashToUser.at(userInCallWithNicknameHash);

                    m_networkController.sendToClient(userInCallWith->getEndpoint(), getPacketEndCall(senderNicknameHash, false),
                        PacketType::END_CALL
                    );

                    m_calls.erase(userToErase->getCall());

                    userToErase->resetCall();
                    userInCallWith->resetCall();
                }
                else {
                    m_calls.erase(userToErase->getCall());
                }
            }

            if (m_nicknameHashToUser.contains(senderNicknameHash)) {
                m_nicknameHashToUser.erase(senderNicknameHash);
            }

            if (needConfirmation) 
                m_networkController.sendToClient(endpointFrom, getPacketWithUuid(uid), PacketType::LOGOUT_OK);

            LOG_INFO("User successfully logged out: {}", senderNicknameHash);
        }
        else {
            LOG_WARN("Logout request from unknown endpoint: {}", endpointFrom.address().to_string());
        }

    }
    catch (const std::exception& e) {
        LOG_ERROR("Error during logout: {}", e.what());
    }
}

void CallsServer::handleGetFriendInfo(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string uid = jsonObject[UID].get<std::string>();
        std::string friendNicknameHash = jsonObject[NICKNAME_HASH].get<std::string>();
        std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();

        if (m_nicknameHashToUser.contains(friendNicknameHash)) {
            auto& friendUser = m_nicknameHashToUser.at(friendNicknameHash);

            if (m_nicknameHashToUser.contains(senderNicknameHash)) {
                auto& senderUser = m_nicknameHashToUser.at(senderNicknameHash);

                if (senderUser->getNicknameHash() == friendNicknameHash) {
                    m_networkController.sendToClient(endpointFrom, getPacketWithUuid(uid),
                        PacketType::GET_FRIEND_INFO_FAIL);
                    return;
                }
            }

            m_networkController.sendToClient(endpointFrom, getPacketFriendInfoSuccess(uid, friendUser->getPublicKey(), friendUser->getNicknameHash()),
                PacketType::GET_FRIEND_INFO_SUCCESS
            );
        }
        else {
            m_networkController.sendToClient(endpointFrom, getPacketWithUuid(uid),
                PacketType::GET_FRIEND_INFO_FAIL);
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in get friend info packet: {}", e.what());
    }
}

void CallsServer::handleStartScreenSharingPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    std::string uid = jsonObject[UID].get<std::string>();
    std::string receiverNicknameHash = jsonObject[NICKNAME_HASH_RECEIVER].get<std::string>();
    std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();
    
    LOG_INFO("Screen sharing initiated from {} to {}", senderNicknameHash, receiverNicknameHash);

    bool receiverOnline = false;
    if (m_nicknameHashToUser.contains(receiverNicknameHash)) {
        auto& userReceiver = m_nicknameHashToUser.at(receiverNicknameHash);
        m_networkController.sendToClient(userReceiver->getEndpoint(),
            jsonObject.dump(),
            PacketType::START_SCREEN_SHARING
        );

        receiverOnline = true;
    }

    if (m_nicknameHashToUser.contains(senderNicknameHash) && !receiverOnline) {
        auto& userSenser = m_nicknameHashToUser.at(senderNicknameHash);
        m_networkController.sendToClient(userSenser->getEndpoint(),
            getPacketWithUuid(uid),
            PacketType::START_SCREEN_SHARING_FAIL
        );

        LOG_WARN("Screen sharing failed: receiver {} is offline", receiverNicknameHash);
    }
}

void CallsServer::handleStopScreenSharingPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string uid = jsonObject[UID].get<std::string>();
        std::string receiverNicknameHash = jsonObject[NICKNAME_HASH_RECEIVER].get<std::string>();

        if (m_nicknameHashToUser.contains(receiverNicknameHash)) {
            auto& userReceiver = m_nicknameHashToUser.at(receiverNicknameHash);

            m_networkController.sendToClient(userReceiver->getEndpoint(),
                jsonObject.dump(),
                PacketType::STOP_SCREEN_SHARING
            );
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in stop sharing screen packet: {}", e.what());
    }
}

void CallsServer::handleStartCameraSharingPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    std::string uid = jsonObject[UID].get<std::string>();
    std::string receiverNicknameHash = jsonObject[NICKNAME_HASH_RECEIVER].get<std::string>();
    std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();

    LOG_INFO("Camera sharing initiated from {} to {}", senderNicknameHash, receiverNicknameHash);

    bool receiverOnline = false;
    if (m_nicknameHashToUser.contains(receiverNicknameHash)) {
        auto& userReceiver = m_nicknameHashToUser.at(receiverNicknameHash);
        m_networkController.sendToClient(userReceiver->getEndpoint(),
            jsonObject.dump(),
            PacketType::START_CAMERA_SHARING
        );

        receiverOnline = true;
    }

    if (m_nicknameHashToUser.contains(senderNicknameHash) && !receiverOnline) {
        auto& userSenser = m_nicknameHashToUser.at(senderNicknameHash);
        m_networkController.sendToClient(userSenser->getEndpoint(),
            getPacketWithUuid(uid),
            PacketType::START_CAMERA_SHARING_FAIL
        );

        LOG_WARN("Camera sharing failed: receiver {} is offline", receiverNicknameHash);
    }
}

void CallsServer::handleStopCameraSharingPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string uid = jsonObject[UID].get<std::string>();
        std::string receiverNicknameHash = jsonObject[NICKNAME_HASH_RECEIVER].get<std::string>();

        if (m_nicknameHashToUser.contains(receiverNicknameHash)) {
            auto& userReceiver = m_nicknameHashToUser.at(receiverNicknameHash);

            m_networkController.sendToClient(userReceiver->getEndpoint(),
                jsonObject.dump(),
                PacketType::STOP_CAMERA_SHARING
            );
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in stop sharing camera packet: {}", e.what());
    }
}

void CallsServer::handleStartOutgoingCall(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    std::string uid = jsonObject[UID].get<std::string>();
    std::string receiverNicknameHash = jsonObject[NICKNAME_HASH_RECEIVER].get<std::string>();
    std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();

    LOG_INFO("Call initiated from {} to {}", senderNicknameHash, receiverNicknameHash);

    bool receiverOnline = false;
    if (m_nicknameHashToUser.contains(receiverNicknameHash)) {
        auto& userReceiver = m_nicknameHashToUser.at(receiverNicknameHash);
        m_networkController.sendToClient(userReceiver->getEndpoint(),
            jsonObject.dump(),
            PacketType::START_CALLING
        );

        receiverOnline = true;
    }

    if (m_nicknameHashToUser.contains(senderNicknameHash) && !receiverOnline) {
        auto& userSenser = m_nicknameHashToUser.at(senderNicknameHash);
        m_networkController.sendToClient(userSenser->getEndpoint(),
            getPacketWithUuid(uid),
            PacketType::START_CALLING_FAIL
        );
        LOG_WARN("Call failed: receiver {} is offline", receiverNicknameHash);
    }
}

void CallsServer::handleEndCall(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string uid = jsonObject[UID].get<std::string>();
        std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();
        bool needConfirmation = jsonObject[NEED_CONFIRMATION].get<bool>();

        if (m_nicknameHashToUser.contains(senderNicknameHash)) {
            auto& userSender = m_endpointToUser.at(endpointFrom);

            bool success = false;
            if (userSender->isInCall()) {
                const std::string& userInCallWithNicknameHash = userSender->inCallWith();

                if (m_nicknameHashToUser.contains(userInCallWithNicknameHash)) {
                    auto& userInCallWith = m_nicknameHashToUser.at(userInCallWithNicknameHash);

                    m_networkController.sendToClient(userInCallWith->getEndpoint(), jsonObject.dump(),
                        PacketType::END_CALL
                    );

                    m_calls.erase(userSender->getCall());

                    userSender->resetCall();
                    userInCallWith->resetCall();
                    success = true;
                }
                else {
                    m_calls.erase(userSender->getCall());
                    userSender->resetCall();
                    success = true;
                }
            }

            if (success && needConfirmation) {
                m_networkController.sendToClient(endpointFrom, getPacketWithUuid(uid),
                    PacketType::END_CALL_OK
                );
            }
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in end call packet: {}", e.what());
    }
}

void CallsServer::handleAcceptCall(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string uid = jsonObject[UID].get<std::string>();
        std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();
        std::string nicknameHashToAccept = jsonObject[NICKNAME_HASH_RECEIVER].get<std::string>();

        if (m_nicknameHashToUser.contains(nicknameHashToAccept)) {
            auto& userToAccept = m_nicknameHashToUser.at(nicknameHashToAccept);

            m_networkController.sendToClient(userToAccept->getEndpoint(), jsonObject.dump(),
                PacketType::CALL_ACCEPTED
            );
        }
        else {
            m_networkController.sendToClient(endpointFrom, jsonObject.dump(),
                PacketType::CALL_ACCEPTED_FAIL
            );
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in call accepted packet: {}", e.what());
    }
}

void CallsServer::handleDeclineCall(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string nicknameHashTo = jsonObject[NICKNAME_HASH_RECEIVER].get<std::string>();

        if (m_nicknameHashToUser.contains(nicknameHashTo)) {
            auto& userTo = m_nicknameHashToUser.at(nicknameHashTo);

            m_networkController.sendToClient(userTo->getEndpoint(),
                jsonObject.dump(),
                PacketType::CALL_DECLINED
            );
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in call declined packet: {}", e.what());
    }
}

void CallsServer::handleCallAcceptedOkPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string uid = jsonObject[UID].get<std::string>(); 
        std::string responderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();
        std::string initiatorNicknameHash = jsonObject[NICKNAME_HASH_RECEIVER].get<std::string>();

        if (m_nicknameHashToUser.contains(initiatorNicknameHash)) {
            auto& initiatorUser = m_nicknameHashToUser.at(initiatorNicknameHash);

            if (m_nicknameHashToUser.contains(responderNicknameHash)) {
                auto& responderSender = m_nicknameHashToUser.at(responderNicknameHash);

                std::shared_ptr<Call> call = std::make_shared<Call>(initiatorUser, responderSender);
                m_calls.emplace(call);

                initiatorUser->setCall(call, User::CallRole::INITIATOR, responderNicknameHash);
                responderSender->setCall(call, User::CallRole::RESPONDER, initiatorNicknameHash);
                
                LOG_INFO("Call established between {} and {}", initiatorNicknameHash, responderNicknameHash);
            }

            redirect(jsonObject, PacketType::CALL_ACCEPTED_OK);
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in call accepted OK packet: {}", e.what());
    }
}

void CallsServer::handleStopOutgoingCall(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string uid = jsonObject[UID].get<std::string>();
        std::string receiverNicknameHash = jsonObject[NICKNAME_HASH_RECEIVER].get<std::string>();

        if (m_nicknameHashToUser.contains(receiverNicknameHash)) {
            auto& userReceiver = m_nicknameHashToUser.at(receiverNicknameHash);

            m_networkController.sendToClient(userReceiver->getEndpoint(),
                jsonObject.dump(),
                PacketType::STOP_CALLING
            );
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in stop calling packet: {}", e.what());
    }
}

void CallsServer::handleVoicePacket(const unsigned char* data, int size, const asio::ip::udp::endpoint& endpointFrom) {
    std::lock_guard<std::mutex> lock(m_endpointToUserMutex);
    if (m_endpointToUser.contains(endpointFrom)) {
        UserPtr user = m_endpointToUser.at(endpointFrom);

        if (user->isInCall()) {
            const std::string& userNicknameHash = user->inCallWith();

            if (m_nicknameHashToUser.contains(userNicknameHash)) {
                UserPtr userTo = m_nicknameHashToUser.at(userNicknameHash);
                m_networkController.sendVoiceToClient(userTo->getEndpoint(), data, size);
            }
        }
    }
}

void CallsServer::handleScreenPacket(const unsigned char* data, int size, const asio::ip::udp::endpoint& endpointFrom) {
    std::lock_guard<std::mutex> lock(m_endpointToUserMutex);
    if (m_endpointToUser.contains(endpointFrom)) {
        UserPtr user = m_endpointToUser.at(endpointFrom);

        if (user->isInCall()) {
            const std::string& userNicknameHash = user->inCallWith();

            if (m_nicknameHashToUser.contains(userNicknameHash)) {
                UserPtr userTo = m_nicknameHashToUser.at(userNicknameHash);
                m_networkController.sendScreenToClient(userTo->getEndpoint(), data, size);
            }
        }
    }
}

void CallsServer::handleCameraPacket(const unsigned char* data, int size, const asio::ip::udp::endpoint& endpointFrom) {
    std::lock_guard<std::mutex> lock(m_endpointToUserMutex);
    if (m_endpointToUser.contains(endpointFrom)) {
        UserPtr user = m_endpointToUser.at(endpointFrom);

        if (user->isInCall()) {
            const std::string& userNicknameHash = user->inCallWith();

            if (m_nicknameHashToUser.contains(userNicknameHash)) {
                UserPtr userTo = m_nicknameHashToUser.at(userNicknameHash);
                m_networkController.sendCameraToClient(userTo->getEndpoint(), data, size);
            }
        }
    }
}