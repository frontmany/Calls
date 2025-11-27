#include "callsServer.h"
#include "packetsFactory.h"
#include "jsonTypes.h"
#include "crypto.h"
#include "logger.h"

CallsServer::CallsServer(const std::string& port)
    :m_networkController(port,
        [this](const unsigned char* data, int size, PacketType type, const asio::ip::udp::endpoint& endpointFrom) { onReceive(data, size, type, endpointFrom); },
        [this]() {onNetworkError(); })
{
}

void CallsServer::onReceive(const unsigned char* data, int size, PacketType type, const asio::ip::udp::endpoint& endpointFrom) {
    if (type == PacketType::VOICE) {
        handleVoicePacket(data, size, endpointFrom);
        return;
    }
    else if (type == PacketType::SCREEN) {
        handleScreenPacket(data, size, endpointFrom);
        return;
    }

    std::lock_guard<std::mutex> lock(m_nicknameHashToUserAndCallsMutex);

    try {
        switch (type) {
        case PacketType::PING_SUCCESS:
            handlePingSuccess(endpointFrom);
            return;

        case PacketType::PING:
            handlePing(endpointFrom);
            return;
        }


        std::string jsonStr = std::string(reinterpret_cast<const char*>(data), size);
        nlohmann::json jsonObject = nlohmann::json::parse(jsonStr);

        switch (type) {
        case PacketType::AUTHORIZE:
            handleAuthorizationPacket(jsonObject, endpointFrom);
            break;

        case PacketType::LOGOUT:
            handleLogout(jsonObject, endpointFrom);
            break;

        case PacketType::GET_FRIEND_INFO:
            handleGetFriendInfoPacket(jsonObject, endpointFrom);
            break;

        case PacketType::START_CALLING:
            handleStartCallingPacket(jsonObject, endpointFrom);
            break;

        case PacketType::END_CALL:
            handleEndCallPacket(jsonObject, endpointFrom);
            break;

        case PacketType::STOP_CALLING:
            handleStopCallingPacket(jsonObject, endpointFrom);
            break;

        case PacketType::CALL_ACCEPTED:
            handleCallAcceptedPacket(jsonObject, endpointFrom);
            break;

        case PacketType::CALL_DECLINED:
            handleCallDeclinedPacket(jsonObject, endpointFrom);
            break;

        case PacketType::END_CALL_OK:
            redirectPacket(jsonObject, PacketType::END_CALL_OK);
            break;

        case PacketType::STOP_CALLING_OK:
            redirectPacket(jsonObject, PacketType::STOP_CALLING_OK);
            break;

        case PacketType::CALL_ACCEPTED_OK:
            handleCallAcceptedOkPacket(jsonObject, endpointFrom);
            break;

        case PacketType::CALL_DECLINED_OK:
            redirectPacket(jsonObject, PacketType::CALL_DECLINED_OK);
            break;

        case PacketType::START_CALLING_OK:
            redirectPacket(jsonObject, PacketType::START_CALLING_OK);
            break;

        case PacketType::START_SCREEN_SHARING:
            handleStartScreenSharingPacket(jsonObject, endpointFrom);
            break;

        case PacketType::STOP_SCREEN_SHARING:
            handleStopScreenSharingPacket(jsonObject, endpointFrom);
            break;

        case PacketType::START_SCREEN_SHARING_OK:
            redirectPacket(jsonObject, PacketType::START_SCREEN_SHARING_OK);
            break;

        case PacketType::STOP_SCREEN_SHARING_OK:
            redirectPacket(jsonObject, PacketType::STOP_SCREEN_SHARING_OK);
            break;


        case PacketType::START_CAMERA_SHARING:
            handleStartCameraSharingPacket(jsonObject, endpointFrom);
            break;

        case PacketType::STOP_CAMERA_SHARING:
            handleStopCameraSharingPacket(jsonObject, endpointFrom);
            break;

        case PacketType::START_CAMERA_SHARING_OK:
            redirectPacket(jsonObject, PacketType::START_CAMERA_SHARING_OK);
            break;

        case PacketType::STOP_CAMERA_SHARING_OK:
            redirectPacket(jsonObject, PacketType::STOP_CAMERA_SHARING_OK);
            break;

        default:
            LOG_WARN("Unknown packet type: {}", static_cast<int>(type));
            break;
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error processing packet type {}: {}", static_cast<int>(type), e.what());
    }
}


void CallsServer::run() {
    m_running = true;
    LOG_INFO("Starting calls server main loop");
    m_networkController.start();

    using namespace std::chrono_literals;

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    std::chrono::seconds pingGap = 2s;
    std::chrono::seconds checkPingGap = 6s;

    std::chrono::steady_clock::time_point lastPing = begin;
    std::chrono::steady_clock::time_point lastCheck = begin;

    while (m_running) {
        try {
            std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

            auto timeSinceLastCheck = now - lastCheck;
            auto timeSinceLastPing = now - lastPing;

            if (timeSinceLastCheck >= checkPingGap) {
                checkPing();
                lastCheck = now;
                lastPing = now;
            }
            else if (timeSinceLastPing >= pingGap) {
                broadcastPing();
                lastPing = now;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        catch (const std::exception& e) {
            LOG_ERROR("Error in server main loop: {}", e.what());
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        catch (...) {
            LOG_ERROR("Unknown error in server main loop");
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    m_networkController.stop();
}

void CallsServer::stop() {
    LOG_INFO("Server stopped");
    m_running = false;
}

void CallsServer::redirectPacket(const nlohmann::json& jsonObject, PacketType type) {
    try {
        const std::string& receiverNicknameHash = jsonObject[NICKNAME_HASH_RECEIVER].get<std::string>();

        if (m_nicknameHashToUser.contains(receiverNicknameHash)) {
            auto& user = m_nicknameHashToUser.at(receiverNicknameHash);
            try {
                m_networkController.sendToClient(user->getEndpoint(), jsonObject.dump(), type);
            }
            catch (const std::exception& e) {
                LOG_DEBUG("Failed to redirect packet to user {} (may have disconnected): {}", 
                    receiverNicknameHash, e.what());
            }
        }
    }
    catch (const std::exception& e) {
        LOG_WARN("Error in redirectPacket: {}", e.what());
    }
}

void CallsServer::broadcastPing() {
    std::lock_guard<std::mutex> lock1(m_endpointToUserMutex);
    std::lock_guard<std::mutex> lock2(m_pingResultsMutex);

    for (auto& [endpoint, _] : m_endpointToUser) {
        try {
            m_networkController.sendToClient(endpoint, PacketType::PING);
            if (!m_pingResults.contains(endpoint)) {
                m_pingResults.emplace(endpoint, false);
            }
        }
        catch (const std::exception& e) {
            LOG_DEBUG("Failed to send ping to {}:{} (may have disconnected): {}", 
                endpoint.address().to_string(), endpoint.port(), e.what());
        }
    }
}

void CallsServer::checkPing() {
    std::vector<asio::ip::udp::endpoint> endpointsToLogout;

    {
        std::lock_guard<std::mutex> lock1(m_endpointToUserMutex);
        std::lock_guard<std::mutex> lock2(m_pingResultsMutex);

        for (auto& [endpoint, result] : m_pingResults) {
            if (result) {
                m_pingResults.at(endpoint) = false;
                LOG_DEBUG("Ping confirmed from {}", endpoint.address().to_string());
            }
            else {
                endpointsToLogout.push_back(endpoint);
                LOG_INFO("User disconnected due to ping timeout: {}", endpoint.address().to_string());
            }
        }
    }

    std::lock_guard<std::mutex> lock(m_nicknameHashToUserAndCallsMutex);
    std::lock_guard<std::mutex> lockEndpoint(m_endpointToUserMutex);
    std::lock_guard<std::mutex> lockPing(m_pingResultsMutex);

    for (const auto& endpoint : endpointsToLogout) {
        try {
            auto it = m_endpointToUser.find(endpoint);

            if (it != m_endpointToUser.end()) {
                auto& userToErase = it->second;
                std::string nicknameHash = userToErase->getNicknameHash();

                if (userToErase->isInCall()) {
                    const std::string& userInCallWithNicknameHash = userToErase->inCallWith();

                    if (m_nicknameHashToUser.contains(userInCallWithNicknameHash)) {
                        auto& userInCallWith = m_nicknameHashToUser.at(userInCallWithNicknameHash);

                        try {
                            m_networkController.sendToClient(userInCallWith->getEndpoint(), getPacketEndCall(nicknameHash, false),
                                PacketType::END_CALL
                            );
                        }
                        catch (const std::exception& e) {
                            LOG_WARN("Failed to send END_CALL to user {} (may have disconnected): {}", 
                                userInCallWithNicknameHash, e.what());
                        }

                        try {
                            m_calls.erase(userToErase->getCall());
                            userToErase->resetCall();
                            userInCallWith->resetCall();
                        }
                        catch (const std::exception& e) {
                            LOG_WARN("Error cleaning up call for user {}: {}", nicknameHash, e.what());
                        }
                    }
                }

                m_endpointToUser.erase(it);
                m_nicknameHashToUser.erase(nicknameHash);
                m_pingResults.erase(endpoint);
            }
        }
        catch (const std::exception& e) {
            LOG_ERROR("Error processing logout for endpoint {}: {}", endpoint.address().to_string(), e.what());
            m_pingResults.erase(endpoint);
        }
    }
}

void CallsServer::handlePingSuccess(const asio::ip::udp::endpoint& endpointFrom) {
    std::lock_guard<std::mutex> lock(m_pingResultsMutex);
    if (m_pingResults.contains(endpointFrom)) {
        m_pingResults.at(endpointFrom) = true;
    }
}

void CallsServer::handlePing(const asio::ip::udp::endpoint& endpointFrom) {
    m_networkController.sendToClient(endpointFrom, PacketType::PING_SUCCESS);
}

void CallsServer::handleAuthorizationPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string uuid = jsonObject[UUID].get<std::string>();
        std::string nicknameHash = jsonObject[NICKNAME_HASH_SENDER].get<std::string>();
        std::string publicKeyStr = jsonObject[PUBLIC_KEY].get<std::string>();
        CryptoPP::RSA::PublicKey publicKey = crypto::deserializePublicKey(publicKeyStr);

        std::lock_guard<std::mutex> lock(m_endpointToUserMutex);
        if (!m_nicknameHashToUser.contains(nicknameHash)) {
            m_networkController.sendToClient(endpointFrom, getPacketWithUuid(uuid),
                PacketType::AUTHORIZE_SUCCESS
            );

            UserPtr user = std::make_shared<User>(nicknameHash, publicKey, endpointFrom);
            m_endpointToUser.emplace(endpointFrom, user);
            m_nicknameHashToUser.emplace(nicknameHash, user);
            LOG_INFO("User authorized: {} from {}:{}", nicknameHash, endpointFrom.address().to_string(), endpointFrom.port());
        }
        else {
            m_networkController.sendToClient(endpointFrom, getPacketWithUuid(uuid),
                PacketType::AUTHORIZE_FAIL
            );

            LOG_WARN("Authorization failed - nickname already taken: {}", nicknameHash);
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in authorization packet: {}", e.what());
    }
}

void CallsServer::handleLogout(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom, bool logoutAndStop) {
    try {
        std::string uuid = jsonObject[UUID].get<std::string>();
        std::string senderNicknameHash = jsonObject[NICKNAME_HASH_SENDER].get<std::string>();
        bool needConfirmation = jsonObject[NEED_CONFIRMATION].get<bool>();

        {
            std::lock_guard<std::mutex> lock(m_endpointToUserMutex);
            auto it = m_endpointToUser.find(endpointFrom);

            if (it != m_endpointToUser.end()) {
                m_endpointToUser.erase(it);
            }
        }

        {
            std::lock_guard<std::mutex> pingLock(m_pingResultsMutex);
            m_pingResults.erase(endpointFrom);
        }

        if (m_nicknameHashToUser.contains(senderNicknameHash)) {
            m_nicknameHashToUser.erase(senderNicknameHash);

            if (needConfirmation) 
                m_networkController.sendToClient(endpointFrom, getPacketWithUuid(uuid), PacketType::LOGOUT_OK);

            LOG_INFO("User successfully logged out: {}", senderNicknameHash);
        }

        else
            LOG_WARN("Logout request from unknown endpoint: {}", endpointFrom.address().to_string());

    }
    catch (const std::exception& e) {
        LOG_ERROR("Error during logout: {}", e.what());
    }
}

void CallsServer::handleGetFriendInfoPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string uuid = jsonObject[UUID].get<std::string>();
        std::string friendNicknameHash = jsonObject[NICKNAME_HASH].get<std::string>();
        std::string senderNicknameHash = jsonObject[NICKNAME_HASH_SENDER].get<std::string>();

        if (m_nicknameHashToUser.contains(friendNicknameHash)) {
            auto& friendUser = m_nicknameHashToUser.at(friendNicknameHash);

            if (m_nicknameHashToUser.contains(senderNicknameHash)) {
                auto& senderUser = m_nicknameHashToUser.at(senderNicknameHash);

                if (senderUser->getNicknameHash() == friendNicknameHash) {
                    m_networkController.sendToClient(endpointFrom, getPacketWithUuid(uuid),
                        PacketType::GET_FRIEND_INFO_FAIL);
                    return;
                }
            }

            m_networkController.sendToClient(endpointFrom, getPacketFriendInfoSuccess(uuid, friendUser->getPublicKey(), friendUser->getNicknameHash()),
                PacketType::GET_FRIEND_INFO_SUCCESS
            );
        }
        else {
            m_networkController.sendToClient(endpointFrom, getPacketWithUuid(uuid),
                PacketType::GET_FRIEND_INFO_FAIL);
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in get friend info packet: {}", e.what());
    }
}

void CallsServer::handleStartScreenSharingPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    std::string uuid = jsonObject[UUID].get<std::string>();
    std::string receiverNicknameHash = jsonObject[NICKNAME_HASH_RECEIVER].get<std::string>();
    std::string senderNicknameHash = jsonObject[NICKNAME_HASH_SENDER].get<std::string>();
    
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
            getPacketWithUuid(uuid),
            PacketType::START_SCREEN_SHARING_FAIL
        );

        LOG_WARN("Screen sharing failed: receiver {} is offline", receiverNicknameHash);
    }
}

void CallsServer::handleStopScreenSharingPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string uuid = jsonObject[UUID].get<std::string>();
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
    std::string uuid = jsonObject[UUID].get<std::string>();
    std::string receiverNicknameHash = jsonObject[NICKNAME_HASH_RECEIVER].get<std::string>();
    std::string senderNicknameHash = jsonObject[NICKNAME_HASH_SENDER].get<std::string>();

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
            getPacketWithUuid(uuid),
            PacketType::START_CAMERA_SHARING_FAIL
        );

        LOG_WARN("Camera sharing failed: receiver {} is offline", receiverNicknameHash);
    }
}

void CallsServer::handleStopCameraSharingPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string uuid = jsonObject[UUID].get<std::string>();
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

void CallsServer::handleStartCallingPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    std::string uuid = jsonObject[UUID].get<std::string>();
    std::string receiverNicknameHash = jsonObject[NICKNAME_HASH_RECEIVER].get<std::string>();
    std::string senderNicknameHash = jsonObject[NICKNAME_HASH_SENDER].get<std::string>();

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
            getPacketWithUuid(uuid),
            PacketType::START_CALLING_FAIL
        );
        LOG_WARN("Call failed: receiver {} is offline", receiverNicknameHash);
    }
}

void CallsServer::handleEndCallPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string uuid = jsonObject[UUID].get<std::string>();
        std::string senderNicknameHash = jsonObject[NICKNAME_HASH_SENDER].get<std::string>();
        bool needConfirmation = jsonObject[NEED_CONFIRMATION].get<bool>();

        if (m_nicknameHashToUser.contains(senderNicknameHash)) {
            auto& userSender = m_endpointToUser.at(endpointFrom);

            bool error = false;
            if (userSender->isInCall()) {
                const std::string& userInCallWithNicknameHash = userSender->inCallWith();

                if (m_nicknameHashToUser.contains(userInCallWithNicknameHash)) {
                    auto& userInCallWith = m_nicknameHashToUser.at(userInCallWithNicknameHash);

                    try {
                        m_networkController.sendToClient(userInCallWith->getEndpoint(), jsonObject.dump(),
                            PacketType::END_CALL
                        );
                    }
                    catch (const std::exception& e) {
                        LOG_DEBUG("Failed to send END_CALL to user {} (may have disconnected): {}", 
                            userInCallWithNicknameHash, e.what());
                    }

                    try {
                        m_calls.erase(userSender->getCall());
                        userSender->resetCall();
                        userInCallWith->resetCall();
                    }
                    catch (const std::exception& e) {
                        LOG_WARN("Error cleaning up call: {}", e.what());
                    }
                }
            }

            if (error && needConfirmation) {
                m_networkController.sendToClient(endpointFrom, getPacketWithUuid(uuid),
                    PacketType::END_CALL_OK
                );
            }
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in end call packet: {}", e.what());
    }
}

void CallsServer::handleCallAcceptedPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string uuid = jsonObject[UUID].get<std::string>();
        std::string senderNicknameHash = jsonObject[NICKNAME_HASH_SENDER].get<std::string>();
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

void CallsServer::handleCallDeclinedPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
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
        std::string uuid = jsonObject[UUID].get<std::string>(); 
        std::string responderNicknameHash = jsonObject[NICKNAME_HASH_SENDER].get<std::string>();
        std::string initiatorNicknameHash = jsonObject[NICKNAME_HASH_RECEIVER].get<std::string>();

        if (m_nicknameHashToUser.contains(initiatorNicknameHash)) {
            auto& initiatorUser = m_nicknameHashToUser.at(initiatorNicknameHash);

            if (m_nicknameHashToUser.contains(responderNicknameHash)) {
                auto& responderSender = m_nicknameHashToUser.at(responderNicknameHash);

                std::shared_ptr<Call> call = std::make_shared<Call>(initiatorNicknameHash, responderNicknameHash);
                m_calls.emplace(call);

                initiatorUser->setCall(call, User::CallRole::INITIATOR);
                responderSender->setCall(call, User::CallRole::RESPONDER);
                
                LOG_INFO("Call established between {} and {}", initiatorNicknameHash, responderNicknameHash);
            }

            redirectPacket(jsonObject, PacketType::CALL_ACCEPTED_OK);
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in call accepted OK packet: {}", e.what());
    }
}

void CallsServer::handleStopCallingPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string uuid = jsonObject[UUID].get<std::string>();
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

void CallsServer::onNetworkError() {
    LOG_WARN("Critical network error occurred (socket issue), but continuing server operation");
    LOG_DEBUG("onNetworkError called - server will continue running");
    // DO NOT STOP SERVER - this is normal for UDP when clients crash
}