#include "callsServer.h"
#include "packetsFactory.h"
#include "crypto.h"

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

        case PacketType::CREATE_CALL:
            jsonStr = std::string(reinterpret_cast<const char*>(data), size);
            jsonObject = nlohmann::json::parse(jsonStr);
            handleCreateCallPacket(jsonObject, endpointFrom);
            break;

        case PacketType::END_CALL:
            handleEndCallPacket(endpointFrom);
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
            std::cerr << "Unknown packet type: " << static_cast<int>(type) << std::endl;
            break;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error processing packet type " << static_cast<int>(type)
            << ": " << e.what() << std::endl;
    }
}
      

void CallsServer::run() {
    m_running = true;
    m_networkController.start();

    while (m_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Smaller sleep for better responsiveness
    }

    m_networkController.stop();
}

void CallsServer::handleAuthorizationPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::string nicknameHash = jsonObject[NICKNAME_HASH].get<std::string>();
        std::string publicKeyStr = jsonObject[PUBLIC_KEY].get<std::string>();
        CryptoPP::RSA::PublicKey publicKey = crypto::deserializePublicKey(publicKeyStr);

        if (!m_nicknameHashToUser.contains(nicknameHash)) {
            m_networkController.sendToClient(endpointFrom,
                PacketType::AUTHORIZATION_SUCCESS
            );

            UserPtr user = std::make_shared<User>(nicknameHash, publicKey, endpointFrom);
            m_endpointToUser.emplace(endpointFrom, user);
            m_nicknameHashToUser.emplace(nicknameHash, user);

            std::cout << "User authorized: " << nicknameHash << " from "
                << endpointFrom.address().to_string() << ":" << endpointFrom.port() << std::endl;
        }
        else {
            m_networkController.sendToClient(endpointFrom,
                PacketType::AUTHORIZATION_FAIL
            );

            std::cout << "User not authorized" << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error in authorization packet: " << e.what() << std::endl;
    }
}


void CallsServer::handleLogout(const asio::ip::udp::endpoint& endpointFrom) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (auto it = m_endpointToUser.find(endpointFrom); it != m_endpointToUser.end()) {
        auto& user = it->second;
        std::string nicknameHash = user->getNicknameHash();

        std::cout << "User logging out: " << nicknameHash << " from "
            << endpointFrom.address().to_string() << ":" << endpointFrom.port() << std::endl;

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

                std::cout << "Notified user " << nicknameHashInCallWith
                    << " about call termination due to logout" << std::endl;
            }

            // Remove the call from active calls
            if (m_calls.contains(call)) {
                m_calls.erase(call);
                std::cout << "Removed call from active calls due to logout" << std::endl;
            }
        }

        // Clean up user data structures
        m_endpointToUser.erase(it);
        m_nicknameHashToUser.erase(nicknameHash);

        std::cout << "User successfully logged out: " << nicknameHash << std::endl;
    }
    else {
        // User wasn't found, but still clean up any residual data
        std::cout << "Logout request from unknown endpoint: "
            << endpointFrom.address().to_string() << ":" << endpointFrom.port() << std::endl;
    }
}

void CallsServer::handleGetFriendInfoPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::string friendNicknameHash = jsonObject[NICKNAME_HASH].get<std::string>();

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
        std::cerr << "Error in get friend info packet: " << e.what() << std::endl;
    }
}

void CallsServer::handleCreateCallPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    std::lock_guard<std::mutex> lock(m_mutex);
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

void CallsServer::handleEndCallPacket(const asio::ip::udp::endpoint& endpointFrom) {
    std::lock_guard<std::mutex> lock(m_mutex);
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
        std::lock_guard<std::mutex> lock(m_mutex);
        std::string initiatorNicknameHash = jsonObject[NICKNAME_HASH].get<std::string>();

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
        std::cerr << "Error in create call packet: " << e.what() << std::endl;
    }
}

void CallsServer::handleCallDeclinedPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::string initiatorNicknameHash = jsonObject[NICKNAME_HASH].get<std::string>();

        if (m_nicknameHashToUser.contains(initiatorNicknameHash)) {
            auto userInitiator = m_nicknameHashToUser.at(initiatorNicknameHash);

            m_networkController.sendToClient(userInitiator->getEndpoint(),
                PacketType::CALL_DECLINED
            );
        }
        else return;
    }
    catch (const std::exception& e) {
        std::cerr << "Error in create call packet: " << e.what() << std::endl;
    }
}

void CallsServer::handleCallingEndPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::string responderNicknameHash = jsonObject[NICKNAME_HASH_TO].get<std::string>();

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
        std::cerr << "Error in create call packet: " << e.what() << std::endl;
    }
}

void CallsServer::handleVoicePacket(const unsigned char* data, int size, const asio::ip::udp::endpoint& endpointFrom) {
    std::lock_guard<std::mutex> lock(m_mutex);
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
    std::lock_guard<std::mutex> lock(m_mutex);

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
        std::cout << "erased force disconnected client\n";
    }
}