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

        case PacketType::PING:
            m_networkController.sendToClient(endpointFrom, PacketType::PING_SUCCESS);
            break;

        case PacketType::PING_SUCCESS:
            handlePingSuccess(endpointFrom);
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

    using namespace std::chrono_literals;
    std::chrono::duration gap(1s);
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    while (m_running) {
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        
        if (now - start >= gap) {
            checkPingResults();
            ping();
            start = now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    m_networkController.stop();
}

void CallsServer::ping() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& [endpoint, _] : m_endpointToUser) {
        m_networkController.sendToClient(endpoint, PacketType::PING);
        if (!m_pingResults.contains(endpoint)) {
            m_pingResults.emplace(endpoint, PingResult::PING_FAIL);
        }
    }
}

void CallsServer::checkPingResults() {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<asio::ip::udp::endpoint> endpointsToRemove;
    bool needToIterate = false;

    for (auto& [endpoint, pingResult] : m_pingResults) {
        if (pingResult == PingResult::PING_FAIL) {
            pingResult = PingResult::PING_LOSS_1;
        }
        else if (pingResult == PingResult::PING_LOSS_1) {
            pingResult = PingResult::PING_LOSS_2;
        }
        else if (pingResult == PingResult::PING_LOSS_2) {
            pingResult = PingResult::PING_LOSS_3;
        }
        else if (pingResult == PingResult::PING_LOSS_3) {
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
                    needToIterate = true;
                }

                const std::string& nicknameHash = user->getNicknameHash();
                m_endpointToUser.erase(endpoint);
                m_nicknameHashToUser.erase(nicknameHash);
                std::cout << "erased force disconnected client\n";
            }
        }
        else
            pingResult = PingResult::PING_FAIL;
            continue;
    }

    if (needToIterate) {
        for (auto& endpointToRemove : endpointsToRemove) {
            m_pingResults.erase(endpointToRemove);
        }
    }
}

void CallsServer::handlePingSuccess(const asio::ip::udp::endpoint& endpointFrom) {
    if (m_pingResults.contains(endpointFrom)) {
        auto& result = m_pingResults.at(endpointFrom);
        result = PingResult::PING_SUCCESS;
    }
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
        auto& [userEndpoint, user] = *it;

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

        const std::string& nicknameHash = user->getNicknameHash();

        m_endpointToUser.erase(userEndpoint);
        m_nicknameHashToUser.erase(nicknameHash);

        std::cout << "User logout" << std::endl;
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