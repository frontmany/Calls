#include "callsServer.h"
#include "packetsFactory.h"
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

        case PacketType::GET_FRIEND_INFO:
            handleGetFriendInfoPacket(jsonObject, endpointFrom);
            break;

        case PacketType::CREATE_CALL:
            handleCreateCallPacket(jsonObject, endpointFrom);
            break;

        case PacketType::END_CALL:
            handleEndCallPacket(jsonObject, endpointFrom);
            break;

        case PacketType::CALL_ACCEPTED:
            handleCallAcceptedPacket(jsonObject, endpointFrom);
            break;

        case PacketType::CALL_DECLINED:
            handleCallDeclinedPacket(jsonObject, endpointFrom);
            break;

        case PacketType::CALLING_END:
            handleCallingEndPacket(jsonObject, endpointFrom);
            break;

        case PacketType::LOGOUT:
            handleLogout(jsonObject, endpointFrom);
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

        // Check ping takes priority over ping
        if (timeSinceLastCheck >= checkPingGap) {
            checkPing();
            lastCheck = now;
            // After check, also reset ping timer to avoid immediate ping
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
    std::vector<asio::ip::udp::endpoint> endpointsToLogout;

    {
        std::lock_guard<std::mutex> lock1(m_endpointToUserMutex);
        std::lock_guard<std::mutex> lock2(m_pingResultsMutex);

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
    }

    // Handle logouts after releasing all locks
    for (const auto& endpoint : endpointsToLogout) {
        handleLogout(endpoint);
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
        std::string nicknameHash = jsonObject[NICKNAME_HASH].get<std::string>();
        std::string publicKeyStr = jsonObject[PUBLIC_KEY].get<std::string>();
        CryptoPP::RSA::PublicKey publicKey = crypto::deserializePublicKey(publicKeyStr);

        std::lock_guard<std::mutex> lock(m_endpointToUserMutex);
        if (!m_nicknameHashToUser.contains(nicknameHash)) {
            m_networkController.sendToClient(endpointFrom,
                PacketType::AUTHORIZATION_SUCCESS
            );

            UserPtr user = std::make_shared<User>(nicknameHash, publicKey, endpointFrom);
            m_endpointToUser.emplace(endpointFrom, user);
            m_nicknameHashToUser.emplace(nicknameHash, user);
            DEBUG_LOG("User authorized: " + nicknameHash + " from " + endpointFrom.address().to_string());
        }
        else {
            m_networkController.sendToClient(endpointFrom,
                PacketType::AUTHORIZATION_FAIL
            );

            DEBUG_LOG("User not authorized (alreaady taken nickname)");
        }
    }
    catch (const std::exception& e) {
        DEBUG_LOG("Error in authorization packet: " + std::string(e.what()));
    }
}


void CallsServer::handleLogout(const asio::ip::udp::endpoint& endpointFrom) {
    std::lock_guard<std::mutex> lock(m_endpointToUserMutex);
    if (auto it = m_endpointToUser.find(endpointFrom); it != m_endpointToUser.end()) {
        auto& user = it->second;
        std::string nicknameHash = user->getNicknameHash();

        DEBUG_LOG("User logging out: "
            + nicknameHash + " from "
            + endpointFrom.address().to_string());

        // Handle call termination if user was in a call
        if (user->inCall()) {
            auto call = user->getCall();
            const std::string& nicknameHashInCallWith = user->inCallWith();

            if (m_nicknameHashToUser.contains(nicknameHashInCallWith)) {
                auto userInCallWith = m_nicknameHashToUser.at(nicknameHashInCallWith);

                // Notify the other user that the call ended due to logout
                m_networkController.sendToClient(userInCallWith->getEndpoint(),
                    PacketType::END_CALL
                );

                // Reset the other user's call state
                userInCallWith->resetCall();

                DEBUG_LOG("Notified user "
                    + nicknameHashInCallWith
                    + " about call termination due to logout");
            }

            // Remove the call from active calls
            if (m_calls.contains(call)) {
                m_calls.erase(call);
                DEBUG_LOG("Removed call from active calls due to logout");
            }
        }

        // Clean up user data structures
        m_endpointToUser.erase(it);
        m_nicknameHashToUser.erase(nicknameHash);

        // Also clean up ping results
        std::lock_guard<std::mutex> pingLock(m_pingResultsMutex);
        m_pingResults.erase(endpointFrom);

        DEBUG_LOG("User successfully logged out: " + nicknameHash);
    }
    else {
        // User wasn't found, but still clean up any residual data
        std::lock_guard<std::mutex> pingLock(m_pingResultsMutex);
        m_pingResults.erase(endpointFrom);

        DEBUG_LOG("Logout request from unknown endpoint: "
            + endpointFrom.address().to_string());
    }
}

void CallsServer::handleGetFriendInfoPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string friendNicknameHash = jsonObject[NICKNAME_HASH].get<std::string>();

        std::lock_guard<std::mutex> lock(m_endpointToUserMutex);
        if (m_nicknameHashToUser.contains(friendNicknameHash)) {
            auto user = m_nicknameHashToUser.at(friendNicknameHash);

            if (m_endpointToUser.contains(endpointFrom)) {
                auto userWhoRequested = m_endpointToUser.at(endpointFrom);
                if (userWhoRequested->getNicknameHash() == friendNicknameHash) {
                    m_networkController.sendToClient(endpointFrom, PacketType::FRIEND_INFO_FAIL);
                    return;
                }
            }

            m_networkController.sendToClient(endpointFrom,
                PacketsFactory::getFriendInfoSuccessPacket(user->getPublicKey(), user->getNicknameHash()),
                PacketType::FRIEND_INFO_SUCCESS
            );
        }
        else {
            m_networkController.sendToClient(endpointFrom, PacketType::FRIEND_INFO_FAIL);
        }
    }
    catch (const std::exception& e) {
        DEBUG_LOG("Error in get friend info packet: " + std::string(e.what()));
    }
}

void CallsServer::handleCreateCallPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    std::string responderNicknameHash = jsonObject[NICKNAME_HASH].get<std::string>();

    std::lock_guard<std::mutex> lock(m_endpointToUserMutex);
    if (m_nicknameHashToUser.contains(responderNicknameHash)) {
        auto userResponder = m_nicknameHashToUser.at(responderNicknameHash);
        m_networkController.sendToClient(userResponder->getEndpoint(),
            jsonObject.dump(),
            PacketType::INCOMING_CALL
        );
    }
    else return;
}

void CallsServer::handleEndCallPacket(const asio::ip::udp::endpoint& endpointFrom) {
    std::lock_guard<std::mutex> lock(m_endpointToUserMutex);
    if (m_endpointToUser.contains(endpointFrom)) {
        auto user1 = m_endpointToUser.at(endpointFrom);
        if (user1->inCall()) {
            const std::string& user2NicknameHash = user1->inCallWith();
            auto user2 = m_nicknameHashToUser.at(user2NicknameHash);

            m_networkController.sendToClient(user2->getEndpoint(),
                PacketType::END_CALL
            );

            m_calls.erase(user1->getCall());

            user1->resetCall();
            user2->resetCall();
        }
    }
}

void CallsServer::handleCallAcceptedPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string initiatorNicknameHash = jsonObject[NICKNAME_HASH].get<std::string>();

        std::lock_guard<std::mutex> lock(m_endpointToUserMutex);
        if (m_nicknameHashToUser.contains(initiatorNicknameHash)) {
            auto userInitiator = m_nicknameHashToUser.at(initiatorNicknameHash);

            if (m_endpointToUser.contains(endpointFrom)) {
                auto userResponder = m_endpointToUser.at(endpointFrom);
                std::shared_ptr<Call> call = std::make_shared<Call>(userInitiator->getNicknameHash(), userResponder->getNicknameHash());
                m_calls.emplace(call);

                userInitiator->setCall(call, User::CallRole::INITIATOR);
                userResponder->setCall(call, User::CallRole::RESPONDER);
            }
            else return;

            m_networkController.sendToClient(userInitiator->getEndpoint(),
                PacketType::CALL_ACCEPTED
            );
        }
        else return;
    }
    catch (const std::exception& e) {
        DEBUG_LOG("Error in create call packet: " + std::string(e.what()));
    }
}

void CallsServer::handleCallDeclinedPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string initiatorNicknameHash = jsonObject[NICKNAME_HASH].get<std::string>();

        std::lock_guard<std::mutex> lock(m_endpointToUserMutex);
        if (m_nicknameHashToUser.contains(initiatorNicknameHash)) {
            auto userInitiator = m_nicknameHashToUser.at(initiatorNicknameHash);

            m_networkController.sendToClient(userInitiator->getEndpoint(),
                PacketType::CALL_DECLINED
            );
        }
        else return;
    }
    catch (const std::exception& e) {
        DEBUG_LOG("Error in create call packet: " + std::string(e.what()));
    }
}

void CallsServer::handleCallingEndPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string responderNicknameHash = jsonObject[NICKNAME_HASH_TO].get<std::string>();

        std::lock_guard<std::mutex> lock(m_endpointToUserMutex);
        if (m_nicknameHashToUser.contains(responderNicknameHash)) {
            auto userResponder = m_nicknameHashToUser.at(responderNicknameHash);

            m_networkController.sendToClient(userResponder->getEndpoint(),
                jsonObject.dump(),
                PacketType::CALLING_END
            );
        }
        else return;
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