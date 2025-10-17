#include "callsServer.h"
#include "packetsFactory.h"
#include "jsonTypes.h"
#include "crypto.h"
#include "logger.h"

CallsServer::CallsServer()
    :m_networkController("8081",
        [this](const unsigned char* data, int size, PacketType type, const asio::ip::udp::endpoint& endpointFrom) { onReceive(data, size, type, endpointFrom); },
        [this]() {onNetworkError(); },
        [this](asio::ip::udp::endpoint endpoint) {onUserDisconnected(endpoint); })
{
}

void CallsServer::onReceive(const unsigned char* data, int size, PacketType type, const asio::ip::udp::endpoint& endpointFrom) {
    if (type == PacketType::VOICE) {
        handleVoicePacket(data, size, endpointFrom);
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

        default:
            DEBUG_LOG("Unknown packet type: " + std::to_string(static_cast<int>(type)));
            break;
        }
    }
    catch (const std::exception& e) {
        DEBUG_LOG("Error processing packet type "
            + std::to_string(static_cast<int>(type))
            + ": " << e.what());
    }
}


void CallsServer::run() {
    m_running = true;
    m_networkController.start();

    using namespace std::chrono_literals;

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    std::chrono::seconds pingGap = 10s;
    std::chrono::seconds checkPingGap = 30s;

    std::chrono::steady_clock::time_point lastPing = begin;
    std::chrono::steady_clock::time_point lastCheck = begin;

    while (m_running) {
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

    m_networkController.stop();
}

void CallsServer::handleEndCallAssist(const UserPtr& user) {
    if (user->inCall()) {
        const std::string& userInCallWithNicknameHash = user->inCallWith();

        if (m_nicknameHashToUser.contains(userInCallWithNicknameHash)) {
            auto& userInCallWith = m_nicknameHashToUser.at(userInCallWithNicknameHash);

            m_networkController.sendToClient(userInCallWith->getEndpoint(),
                PacketType::END_CALL
            );

            m_calls.erase(user->getCall());

            user->resetCall();
            userInCallWith->resetCall();
        }
    }
}

void CallsServer::handleStopCallingAssist(const std::string& receiverNicknameHash, const nlohmann::json& jsonObject) {
    if (m_nicknameHashToUser.contains(receiverNicknameHash)) {
        auto& userReceiver = m_nicknameHashToUser.at(receiverNicknameHash);

        m_networkController.sendToClient(userReceiver->getEndpoint(),
            jsonObject.dump(),
            PacketType::STOP_CALLING
        );
    }
}

void CallsServer::redirectPacket(const nlohmann::json& jsonObject, PacketType type) {
    const std::string& receiverNicknameHash = jsonObject[NICKNAME_HASH_RECEIVER].get<std::string>();

    if (m_nicknameHashToUser.contains(receiverNicknameHash)) {
        auto& user = m_nicknameHashToUser.at(receiverNicknameHash);
        m_networkController.sendToClient(user->getEndpoint(), jsonObject.dump(), type);
    }
}

void CallsServer::broadcastPing() {
    std::lock_guard<std::mutex> lock1(m_endpointToUserMutex);
    std::lock_guard<std::mutex> lock2(m_pingResultsMutex);

    for (auto& [endpoint, _] : m_endpointToUser) {
        m_networkController.sendToClient(endpoint, PacketType::PING);
        if (!m_pingResults.contains(endpoint)) {
            m_pingResults.emplace(endpoint, false);
        }
    }
}

void CallsServer::checkPing() {
    std::lock_guard<std::mutex> lock1(m_endpointToUserMutex);
    std::lock_guard<std::mutex> lock2(m_pingResultsMutex);

    std::vector<asio::ip::udp::endpoint> endpointsToLogout;

    for (auto& [endpoint, result] : m_pingResults) {
        if (result) {
            m_pingResults.at(endpoint) = false;
            DEBUG_LOG("ping confirmed");
        }
        else {
            endpointsToLogout.push_back(endpoint);
            DEBUG_LOG("user disconnected due to ping fail");
        }
    }


    std::lock_guard<std::mutex> lock(m_nicknameHashToUserAndCallsMutex);
    for (const auto& endpoint : endpointsToLogout) {
        auto it = m_endpointToUser.find(endpoint);

        if (it != m_endpointToUser.end()) {
            auto& userToErase = it->second;
            std::string nicknameHash = userToErase->getNicknameHash();

            handleEndCallAssist(userToErase);

            m_endpointToUser.erase(it);
            m_nicknameHashToUser.erase(nicknameHash);
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
            DEBUG_LOG("User authorized: " + nicknameHash + " from " + endpointFrom.address().to_string());
        }
        else {
            m_networkController.sendToClient(endpointFrom, getPacketWithUuid(uuid),
                PacketType::AUTHORIZE_FAIL
            );

            DEBUG_LOG("User not authorized (already taken nickname)");
        }
    }
    catch (const std::exception& e) {
        DEBUG_LOG("Error in authorization packet: " + std::string(e.what()));
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

            DEBUG_LOG("User successfully logged out: " + senderNicknameHash);
        }

        else
            DEBUG_LOG("Logout request from unknown endpoint: " + endpointFrom.address().to_string());

    }
    catch (const std::exception& e) {
        DEBUG_LOG("Error during logout: " + std::string(e.what()));
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
        DEBUG_LOG("Error in get friend info packet: " + std::string(e.what()));
    }
}

void CallsServer::handleStartCallingPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    std::string uuid = jsonObject[UUID].get<std::string>();
    std::string receiverNicknameHash = jsonObject[NICKNAME_HASH_RECEIVER].get<std::string>();
    std::string senderNicknameHash = jsonObject[NICKNAME_HASH_SENDER].get<std::string>();

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
    }
}

void CallsServer::handleEndCallPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string uuid = jsonObject[UUID].get<std::string>();
        std::string senderNicknameHash = jsonObject[NICKNAME_HASH_SENDER].get<std::string>();
        bool needConfirmation = jsonObject[NICKNAME_HASH_RECEIVER].get<bool>();

        if (m_nicknameHashToUser.contains(senderNicknameHash)) {
            auto& userSender = m_endpointToUser.at(endpointFrom);

            bool error = false;
            if (userSender->inCall()) {
                const std::string& userInCallWithNicknameHash = userSender->inCallWith();

                if (m_nicknameHashToUser.contains(userInCallWithNicknameHash)) {
                    auto& userInCallWith = m_nicknameHashToUser.at(userInCallWithNicknameHash);

                    m_networkController.sendToClient(userInCallWith->getEndpoint(), jsonObject.dump(),
                        PacketType::END_CALL
                    );

                    m_calls.erase(userSender->getCall());

                    userSender->resetCall();
                    userInCallWith->resetCall();
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
        DEBUG_LOG("Error in create call packet: " + std::string(e.what()));
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
        DEBUG_LOG("Error in create call packet: " + std::string(e.what()));
    }
}

void CallsServer::handleCallDeclinedPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string uuid = jsonObject[UUID].get<std::string>();
        std::string senderNicknameHash = jsonObject[NICKNAME_HASH_SENDER].get<std::string>();
        std::string nicknameHashTo = jsonObject[NICKNAME_HASH_RECEIVER].get<std::string>();

        if (m_nicknameHashToUser.contains(nicknameHashTo)) {
            auto& userTo = m_nicknameHashToUser.at(nicknameHashTo);

            m_networkController.sendToClient(userTo->getEndpoint(),
                PacketType::CALL_DECLINED
            );
        }
    }
    catch (const std::exception& e) {
        DEBUG_LOG("Error in create call packet: " + std::string(e.what()));
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
            }

            redirectPacket(jsonObject, PacketType::CALL_ACCEPTED_OK);
        }
    }
    catch (const std::exception& e) {
        DEBUG_LOG("Error in callAcceptedOk packet: " + std::string(e.what()));
    }
}

void CallsServer::handleStopCallingPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string uuid = jsonObject[UUID].get<std::string>();
        std::string receiverNicknameHash = jsonObject[NICKNAME_HASH_RECEIVER].get<std::string>();
        std::string senderNicknameHash = jsonObject[NICKNAME_HASH_SENDER].get<std::string>();
        bool needConfirmation = jsonObject[NICKNAME_HASH_RECEIVER].get<bool>();

        if (m_nicknameHashToUser.contains(receiverNicknameHash)) {
            auto& userReceiver = m_nicknameHashToUser.at(receiverNicknameHash);

            m_networkController.sendToClient(userReceiver->getEndpoint(),
                jsonObject.dump(),
                PacketType::STOP_CALLING
            );
        }

        else if (needConfirmation) {
            m_networkController.sendToClient(endpointFrom, getPacketWithUuid(uuid),
                PacketType::STOP_CALLING_OK
            );
        }
    }
    catch (const std::exception& e) {
        DEBUG_LOG("Error in create call packet: " + std::string(e.what()));
    }
}

void CallsServer::handleVoicePacket(const unsigned char* data, int size, const asio::ip::udp::endpoint& endpointFrom) {
    std::lock_guard<std::mutex> lock(m_endpointToUserMutex);
    if (m_endpointToUser.contains(endpointFrom)) {
        UserPtr user = m_endpointToUser.at(endpointFrom);

        if (user->inCall()) {
            const std::string& userNicknameHash = user->inCallWith();

            if (m_nicknameHashToUser.contains(userNicknameHash)) {
                UserPtr userTo = m_nicknameHashToUser.at(userNicknameHash);
                m_networkController.sendVoiceToClient(userTo->getEndpoint(), data, size);
            }
        }
    }
}

void CallsServer::onNetworkError() {
    m_running = false;
}

void CallsServer::onUserDisconnected(const asio::ip::udp::endpoint& endpoint) {
    std::lock_guard<std::mutex> lock1(m_endpointToUserMutex);
    if (m_endpointToUser.contains(endpoint)) {
        auto user = m_endpointToUser.at(endpoint);

        if (user->inCall()) {
            auto call = user->getCall();
            const std::string& nicknameHashInCallWith = user->inCallWith();

            if (m_nicknameHashToUser.contains(nicknameHashInCallWith)) {
                auto userInCallWith = m_nicknameHashToUser.at(nicknameHashInCallWith);
                m_networkController.sendToClient(userInCallWith->getEndpoint(),
                    PacketType::END_CALL
                );
            }

            m_calls.erase(call);
        }

        std::string nicknameHash = user->getNicknameHash();
        m_endpointToUser.erase(endpoint);
        m_nicknameHashToUser.erase(nicknameHash);

        std::lock_guard<std::mutex> lock2(m_pingResultsMutex);
        m_pingResults.erase(endpoint);

        DEBUG_LOG("erased force disconnected client");
    }
}