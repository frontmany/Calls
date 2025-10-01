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
        std::string jsonStr;
        nlohmann::json jsonObject;

        switch (type) {
        case PacketType::AUTHORIZE:
            jsonStr = std::string(reinterpret_cast<const char*>(data), size);
            jsonObject = nlohmann::json::parse(jsonStr);
            handleAuthorizationPacket(jsonObject, endpointFrom);
            break;

        case PacketType::GET_FRIEND_INFO:
            jsonStr = std::string(reinterpret_cast<const char*>(data), size);
            jsonObject = nlohmann::json::parse(jsonStr);
            handleGetFriendInfoPacket(jsonObject, endpointFrom);
            break;

        case PacketType::PING_SUCCESS:
            handlePingSuccess(endpointFrom);
            break;

        case PacketType::PING:
            m_networkController.sendToClient(endpointFrom, PacketType::PING_SUCCESS);
            break;

        case PacketType::CREATE_CALL:
            jsonStr = std::string(reinterpret_cast<const char*>(data), size);
            jsonObject = nlohmann::json::parse(jsonStr);
            handleCreateCallPacket(jsonObject, endpointFrom);
            break;

        case PacketType::CREATE_GROUP_CALL:
            jsonStr = std::string(reinterpret_cast<const char*>(data), size);
            jsonObject = nlohmann::json::parse(jsonStr);
            handleCreateGroupCallPacket(jsonObject, endpointFrom);
            break;

        case PacketType::JOIN_REQUEST:
            jsonStr = std::string(reinterpret_cast<const char*>(data), size);
            jsonObject = nlohmann::json::parse(jsonStr);
            handleJoinRequestPacket(jsonObject, endpointFrom);
            break;

        case PacketType::JOIN_ALLOWED:
            jsonStr = std::string(reinterpret_cast<const char*>(data), size);
            jsonObject = nlohmann::json::parse(jsonStr);
            handleJoinAllowedPacket(jsonObject, endpointFrom);
            break;

        case PacketType::JOIN_DECLINED:
            jsonStr = std::string(reinterpret_cast<const char*>(data), size);
            jsonObject = nlohmann::json::parse(jsonStr);
            handleJoinDeclinedPacket(jsonObject, endpointFrom);
            break;

        case PacketType::JOIN_REQUEST_CANCELED_BY_REQUESTER:
            jsonStr = std::string(reinterpret_cast<const char*>(data), size);
            jsonObject = nlohmann::json::parse(jsonStr);
            handleJoinRequestCanceledByRequesterSidePacket(jsonObject, endpointFrom);
            break;

        case PacketType::LEAVE_GROUP_CALL:
            jsonStr = std::string(reinterpret_cast<const char*>(data), size);
            jsonObject = nlohmann::json::parse(jsonStr);
            handleLeaveGroupCallPacket(jsonObject, endpointFrom);
            break;

        case PacketType::END_CALL:
            handleEndCallPacket(endpointFrom);
            break;

        case PacketType::END_GROUP_CALL:
            jsonStr = std::string(reinterpret_cast<const char*>(data), size);
            jsonObject = nlohmann::json::parse(jsonStr);
            handleEndGroupCallPacket(jsonObject, endpointFrom);
            break;

        case PacketType::CALL_ACCEPTED:
            jsonStr = std::string(reinterpret_cast<const char*>(data), size);
            jsonObject = nlohmann::json::parse(jsonStr);
            handleCallAcceptedPacket(jsonObject, endpointFrom);
            break;

        case PacketType::CALL_DECLINED:
            jsonStr = std::string(reinterpret_cast<const char*>(data), size);
            jsonObject = nlohmann::json::parse(jsonStr);
            handleCallDeclinedPacket(jsonObject, endpointFrom);
            break;

        case PacketType::CALLING_END:
            jsonStr = std::string(reinterpret_cast<const char*>(data), size);
            jsonObject = nlohmann::json::parse(jsonStr);
            handleCallingEndPacket(jsonObject, endpointFrom);
            break;

        case PacketType::LOGOUT:
            handleLogout(endpointFrom);
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
        if (!m_pingResults.contains(endpoint)) {
            m_pingResults.emplace(endpoint, false);
        }
        m_networkController.sendToClient(endpoint, PacketType::PING);
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
        auto user = it->second;
        std::string nicknameHash = user->getNicknameHash();

        DEBUG_LOG("User logging out: "
            + nicknameHash + " from "
            + endpointFrom.address().to_string());

        if (user->inCall()) {
            auto call = user->getCall();
            const std::string& nicknameHashInCallWith = user->inCallWith();

            if (m_nicknameHashToUser.contains(nicknameHashInCallWith)) {
                auto userInCallWith = m_nicknameHashToUser.at(nicknameHashInCallWith);

                m_networkController.sendToClient(userInCallWith->getEndpoint(),
                    PacketType::END_CALL
                );

                userInCallWith->resetCall();

                DEBUG_LOG("Notified user "
                    + nicknameHashInCallWith
                    + " about call termination due to logout");
            }

            if (m_calls.contains(call)) {
                m_calls.erase(call);
                DEBUG_LOG("Removed call from active calls due to logout");
            }
        }
        else if (user->inGroupCall()) {
            auto groupCall = user->getGroupCall();
            GroupCallRole groupCallRole = user->getGroupCallRole();

            if (groupCall != nullptr) {
                if (groupCallRole == GroupCallRole::INITIATOR) {
                    auto participants = user->inGroupCallWith();

                    for (auto& currentNicknameHash : participants) {
                        if (m_nicknameHashToUser.contains(currentNicknameHash)) {
                            auto currentUser = m_nicknameHashToUser.at(currentNicknameHash);
                            currentUser->resetGroupCall();
                            m_networkController.sendToClient(currentUser->getEndpoint(), PacketType::END_GROUP_CALL);
                        }
                    }

                    m_groupCalls.erase(groupCall->getGroupCallNameHash());
                }
                else {
                    auto participants = user->inGroupCallWith();

                    for (auto& currentNicknameHash : participants) {
                        if (m_nicknameHashToUser.contains(currentNicknameHash)) {
                            auto currentUser = m_nicknameHashToUser.at(currentNicknameHash);
                            m_networkController.sendToClient(currentUser->getEndpoint(), PacketsFactory::getParticipantLeavePacket(nicknameHash, groupCall->getGroupCallNameHash()), PacketType::GROUP_CALL_PARTICIPANT_QUIT);
                        }
                    }

                    groupCall->removeParticipant(nicknameHash);
                }
            }
        }

        m_endpointToUser.erase(it);
        m_nicknameHashToUser.erase(nicknameHash);

        std::lock_guard<std::mutex> pingLock(m_pingResultsMutex);
        m_pingResults.erase(endpointFrom);

        DEBUG_LOG("User successfully logged out: " + nicknameHash);
    }
    else {
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
    {
        std::lock_guard<std::mutex> lock(m_endpointToUserMutex);
        if (m_endpointToUser.contains(endpointFrom)) {
            auto userFrom = m_endpointToUser.at(endpointFrom);
            if (userFrom->inCall() || userFrom->inGroupCall()) {
                m_networkController.sendToClient(endpointFrom,
                    PacketType::CALL_DECLINED
                );
            }
        }
    }

    std::string responderNicknameHash = jsonObject[NICKNAME_HASH].get<std::string>();

    if (m_nicknameHashToUser.contains(responderNicknameHash)) {
        auto userResponder = m_nicknameHashToUser.at(responderNicknameHash);

        m_networkController.sendToClient(userResponder->getEndpoint(),
            jsonObject.dump(),
            PacketType::INCOMING_CALL
        );
    }
    else return;
}

void CallsServer::handleCreateGroupCallPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    bool isError = false;

    {
        std::lock_guard<std::mutex> lock(m_endpointToUserMutex);
        if (m_endpointToUser.contains(endpointFrom)) {
            auto user = m_endpointToUser.at(endpointFrom);
            if (user->inCall() || user->inGroupCall()) {
                isError = true;
            }
        }
        else {
            isError = true;
        }
    }
    
    std::string initiatorNicknameHash = jsonObject[NICKNAME_HASH].get<std::string>();
    std::string groupCallNameHash = jsonObject[GROUP_CALL_NAME_HASH].get<std::string>();

    if (!m_groupCalls.contains(groupCallNameHash)) {
        GroupCallPtr groupCall = std::make_shared<GroupCall>(initiatorNicknameHash, groupCallNameHash);
        m_groupCalls.emplace(groupCallNameHash, groupCall);
        m_networkController.sendToClient(endpointFrom, PacketType::CREATE_GROUP_CALL_SUCCESS);
    }
    else {
        isError = true;
    }

    if (isError) {
        m_networkController.sendToClient(endpointFrom, PacketType::CREATE_GROUP_CALL_FAIL);
    }
}

void CallsServer::handleJoinRequestPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    bool isError = false;

    {
        std::lock_guard<std::mutex> lock(m_endpointToUserMutex);
        if (m_endpointToUser.contains(endpointFrom)) {
            auto user = m_endpointToUser.at(endpointFrom);
            if (user->inCall() || user->inGroupCall()) {
                isError = true;
            }
        }
        else {
            isError = true;
        }
    }

    std::string groupCallNameHash = jsonObject[GROUP_CALL_NAME_HASH].get<std::string>();

    if (m_groupCalls.contains(groupCallNameHash)) {
        auto groupCall = m_groupCalls.at(groupCallNameHash);
        const std::string& initiatorNicknameHash = groupCall->getInitiatorNicknameHash();

        if (m_nicknameHashToUser.contains(initiatorNicknameHash)) {
            m_networkController.sendToClient(m_nicknameHashToUser.at(initiatorNicknameHash)->getEndpoint(),
                jsonObject.dump(),
                PacketType::JOIN_REQUEST);
        }
        else {
            isError = true;
        }
    }
    else {
        isError = true;
    }

    if (isError) {
        m_networkController.sendToClient(endpointFrom, PacketType::GROUP_CALL_NOT_EXIST);
    }
}

void CallsServer::handleJoinAllowedPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    std::string nicknameHashTo = jsonObject[NICKNAME_HASH_TO].get<std::string>();
    std::string groupCallNameHash = jsonObject[GROUP_CALL_NAME_HASH].get<std::string>();

    if (m_groupCalls.contains(groupCallNameHash)) {
        if (m_nicknameHashToUser.contains(nicknameHashTo)) {
            auto userTo = m_nicknameHashToUser.at(nicknameHashTo);

            auto groupCall = m_groupCalls.at(groupCallNameHash);
            groupCall->addParticipant(nicknameHashTo);
            userTo->setGroupCall(groupCall, GroupCallRole::PARTICIPANT);
            m_networkController.sendToClient(userTo->getEndpoint(), jsonObject.dump(), PacketType::JOIN_ALLOWED);

            auto participants = userTo->inGroupCallWith();
            for (auto& nicknameHash : participants) {
                std::lock_guard<std::mutex> lock(m_endpointToUserMutex);
                
                if (m_endpointToUser.contains(endpointFrom)) {
                    auto& initiatorNicknameHash = m_endpointToUser.at(endpointFrom)->getNicknameHash();
                    
                    if (nicknameHash != initiatorNicknameHash) {
                        if (m_nicknameHashToUser.contains(nicknameHash)) {
                            auto user = m_nicknameHashToUser.at(nicknameHash);
                            m_networkController.sendToClient(user->getEndpoint(), jsonObject.dump(), PacketType::GROUP_CALL_NEW_PARTICIPANT);
                        }
                    }
                }
            }
        }
        else {
            m_networkController.sendToClient(endpointFrom, PacketsFactory::getParticipantLeavePacket(nicknameHashTo, groupCallNameHash), PacketType::GROUP_CALL_PARTICIPANT_QUIT);
        }
    }
    else {
        if (m_nicknameHashToUser.contains(nicknameHashTo)) {
            auto userTo = m_nicknameHashToUser.at(nicknameHashTo);
            m_networkController.sendToClient(userTo->getEndpoint(), jsonObject.dump(), PacketType::JOIN_DECLINED);
        }
    }
}

void CallsServer::handleJoinDeclinedPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    std::string nicknameHashTo = jsonObject[NICKNAME_HASH_TO].get<std::string>();
    if (m_nicknameHashToUser.contains(nicknameHashTo)) {
        auto userTo = m_nicknameHashToUser.at(nicknameHashTo);
        m_networkController.sendToClient(userTo->getEndpoint(), jsonObject.dump(), PacketType::JOIN_DECLINED);
    }
}

void CallsServer::handleJoinRequestCanceledByRequesterSidePacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    std::string groupCallNameHash = jsonObject[GROUP_CALL_NAME_HASH].get<std::string>();
    if (m_groupCalls.contains(groupCallNameHash)) {
        auto groupCall = m_groupCalls.at(groupCallNameHash);

        const std::string initiatorNicknameHash = groupCall->getInitiatorNicknameHash();
        if (m_nicknameHashToUser.contains(initiatorNicknameHash)) {
            auto initiatorUser = m_nicknameHashToUser.at(initiatorNicknameHash);
            m_networkController.sendToClient(initiatorUser->getEndpoint(), jsonObject.dump(), PacketType::JOIN_REQUEST_CANCELED_BY_REQUESTER);
        }
    }
}

void CallsServer::handleLeaveGroupCallPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    std::string groupCallNameHash = jsonObject[GROUP_CALL_NAME_HASH].get<std::string>();
    std::string nicknameHash = jsonObject[NICKNAME_HASH].get<std::string>();

    if (m_groupCalls.contains(groupCallNameHash)) {
        if (m_nicknameHashToUser.contains(nicknameHash)) {
            auto groupCall = m_groupCalls.at(groupCallNameHash);
            groupCall->removeParticipant(nicknameHash);

            auto user = m_nicknameHashToUser.at(nicknameHash);

            auto participants = user->inGroupCallWith();
            for (auto& currentNicknameHash : participants) {
                if (m_nicknameHashToUser.contains(currentNicknameHash)) {
                    auto currentUser = m_nicknameHashToUser.at(currentNicknameHash);
                    m_networkController.sendToClient(currentUser->getEndpoint(), jsonObject.dump(), PacketType::GROUP_CALL_PARTICIPANT_QUIT);
                }
            }

            user->resetGroupCall();
        }
    }
}

void CallsServer::handleEndCallPacket(const asio::ip::udp::endpoint& endpointFrom) {
    std::lock_guard<std::mutex> lock(m_endpointToUserMutex);
    if (!m_endpointToUser.contains(endpointFrom)) return;

    auto user1 = m_endpointToUser.at(endpointFrom);
    if (!user1->inCall()) return;

    auto call = user1->getCall();
    const std::string& user2NicknameHash = user1->inCallWith();

    if (m_nicknameHashToUser.contains(user2NicknameHash)) {
        auto user2 = m_nicknameHashToUser.at(user2NicknameHash);
        m_networkController.sendToClient(user2->getEndpoint(), PacketType::END_CALL);
        user2->resetCall();
    }

    m_calls.erase(call);
    user1->resetCall();
}

void CallsServer::handleEndGroupCallPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    std::string groupCallNameHash = jsonObject[GROUP_CALL_NAME_HASH].get<std::string>();
    std::string nickNameHash = jsonObject[NICKNAME_HASH].get<std::string>();
    
    if (m_groupCalls.contains(groupCallNameHash)) {
        auto groupCall = m_groupCalls.at(groupCallNameHash);
        if (groupCall->getInitiatorNicknameHash() == nickNameHash) {
            auto& participants = groupCall->getParticipants();

            for (auto& currentNicknameHash : participants) {
                if (currentNicknameHash != nickNameHash) {
                    if (m_nicknameHashToUser.contains(currentNicknameHash)) {
                        auto currentUser = m_nicknameHashToUser.at(currentNicknameHash);
                        currentUser->resetGroupCall();
                        m_networkController.sendToClient(currentUser->getEndpoint(), PacketType::END_GROUP_CALL);
                    }
                }
            }
        }

        m_groupCalls.erase(groupCallNameHash);
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

                userInitiator->setCall(call, CallRole::INITIATOR);
                userResponder->setCall(call, CallRole::RESPONDER);

                m_networkController.sendToClient(userInitiator->getEndpoint(),
                    PacketType::CALL_ACCEPTED
                );
            }
            else {
                m_networkController.sendToClient(userInitiator->getEndpoint(), PacketType::CALL_DECLINED);
            }
        }
        else {
            m_networkController.sendToClient(endpointFrom, PacketType::END_CALL);
        }
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
        else if (user->inGroupCall()) {
            auto participants = user->inGroupCallWith();
            for (auto& userNicknameHash : participants) {
                if (m_nicknameHashToUser.contains(userNicknameHash)) {
                    UserPtr userTo = m_nicknameHashToUser.at(userNicknameHash);
                    m_networkController.sendVoiceToClient(userTo->getEndpoint(), data, size);
                }
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