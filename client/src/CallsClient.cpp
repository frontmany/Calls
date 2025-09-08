#include "callsClient.h"
#include "packetsFabric.h"

#include "json.hpp"

CallsClient::CallsClient(std::function<void(AuthorizationResult)> authorizationResultCallback, std::function<void(CreateCallResult)> createCallResultCallback, std::function<void(const IncomingCallData&)> onIncomingCall, std::function<void()> onNetworkErrorCallback) :
    m_authorizationResultCallback(authorizationResultCallback),
    m_createCallResultCallback(createCallResultCallback),
    m_onNetworkErrorCallback(onNetworkErrorCallback),
    m_onIncomingCall(onIncomingCall),
    m_networkController("192.168.1.45", "8080",
        [this](const unsigned char* data, int length, PacketType type) {
            onReceiveCallback(data, length, type);
        },
        [this]() {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_callbacksQueue.push([this]() {m_onNetworkErrorCallback(); });
        }),
    m_audioEngine(
        [this](const unsigned char* data, int length) {
            onInputVoice(data, length);
        })
{
    crypto::generateRSAKeyPair(m_myPrivateKey, m_myPublicKey);
    m_queueProcessingThread = std::thread(&CallsClient::processQueue, this);
    m_audioEngine.initialize();
}

void CallsClient::onReceiveCallback(const unsigned char* data, int length, PacketType type) {
    if (type == PacketType::VOICE) {
        // TODO
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    m_timer.stop();

    switch (type) {
    case (PacketType::AUTHORIZATION_SUCCESS):
        m_state = State::FREE;
        m_callbacksQueue.push([this]() {m_authorizationResultCallback(AuthorizationResult::SUCCESS); });
        break;
    case (PacketType::AUTHORIZATION_FAIL):
        m_myNickname.clear();
        m_callbacksQueue.push([this]() {m_authorizationResultCallback(AuthorizationResult::FAIL); });
        break;
    case (PacketType::FRIEND_INFO_SUCCESS):
        m_timer.stop();
        if (bool parsed = parsePacket(data, length, type); parsed) {
            startCalling();
        }
        break;
    case (PacketType::FRIEND_INFO_FAIL):
        m_callbacksQueue.push([this]() {m_createCallResultCallback(CreateCallResult::WRONG_FRIEND_NICKNAME_RESULT); });
        break;
    case (PacketType::CALL_ACCEPTED):
        m_state = State::BUSY;
        m_callbacksQueue.push([this]() {m_createCallResultCallback(CreateCallResult::CALL_ACCEPTED); });
        break;
    case (PacketType::CALL_DECLINED):
        m_state = State::FREE;
        m_callbacksQueue.push([this]() {m_createCallResultCallback(CreateCallResult::CALL_DECLINED); });
        break;
    case (PacketType::INCOMING_CALL):
        if (bool parsed = parsePacket(data, length, type); parsed) {
            m_callbacksQueue.push([this]() {m_onIncomingCall(m_incomingCalls.back()); });
        }
        break;
    }
}

void CallsClient::authorize(const std::string& nickname) {
    m_myNickname = nickname;

    bool sent = m_networkController.send(
        PacketsFabric::getAuthorizationPacket(m_myPublicKey, nickname),
        PacketType::AUTHORIZE
    );

    using namespace std::chrono_literals;
    m_timer.start(4s, [this]() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_callbacksQueue.push([this]() {m_authorizationResultCallback(AuthorizationResult::TIMEOUT); });
    });
}

void CallsClient::endCall() {
    if (m_state == State::UNAUTHORIZED) return;
    m_state = State::FREE;
    bool sent = m_networkController.send(PacketType::END_CALL);
    m_call = std::nullopt;
}

void CallsClient::createCall(const std::string& friendNickname) {
    if (m_state == State::UNAUTHORIZED) return;
    requestFriendInfo(friendNickname);
}

void CallsClient::declineCall(const std::string& friendNickname) {
    if (m_state == State::UNAUTHORIZED) return;
    bool sent = m_networkController.send(PacketType::CALL_DECLINED);
}

void CallsClient::acceptCall(const std::string& friendNickname) {
    if (m_state == State::UNAUTHORIZED) return;

    if (m_state == State::BUSY && m_call.has_value()) {
        bool sent = m_networkController.send(PacketType::CALL_ACCEPTED);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

    }

    m_state = State::BUSY;
    bool sent = m_networkController.send(PacketType::CALL_ACCEPTED);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    m_audioEngine.startStream();
}

void CallsClient::processQueue() {
    while (true) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_callbacksQueue.empty()) {
            auto& callback = m_callbacksQueue.front();
            callback();
            m_callbacksQueue.pop();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void CallsClient::requestFriendInfo(const std::string& friendNickname) {
    bool sent = m_networkController.send(
        PacketsFabric::getRequestFriendInfoPacket(friendNickname),
        PacketType::GET_FRIEND_INFO
    );

    using namespace std::chrono_literals;
    m_timer.start(4s, [this]() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_callbacksQueue.push([this]() {m_createCallResultCallback(CreateCallResult::TIMEOUT); });
    });
}

bool CallsClient::parsePacket(const unsigned char* data, int length, PacketType type) {
    try {
        nlohmann::json jsonObject = nlohmann::json::parse(data, data + length);

        switch (type) {
        case PacketType::FRIEND_INFO_SUCCESS:
            m_call = Call(jsonObject[NICKNAME_HASH], crypto::deserializePublicKey(jsonObject[PUBLIC_KEY]));
            m_call.value().createCallKey();
            return true;
        case PacketType::INCOMING_CALL:
            m_incomingCalls.emplace_back(crypto::AESDecrypt(crypto::RSADecryptAESKey(m_myPrivateKey, jsonObject[PACKET_KEY]), jsonObject[NICKNAME]),
                crypto::deserializePublicKey(jsonObject[PUBLIC_KEY]),
                crypto::RSADecryptAESKey(m_myPrivateKey, jsonObject[CALL_KEY])
            );
            return true;
        default:
            std::cout << "Unhandled packet type: " << static_cast<int>(type) << std::endl;
            return false;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Packet parsing error: " << e.what() << std::endl;
        return false;
    }
}

void CallsClient::startCalling() {
    if (m_call) {
        m_state = State::CALLING;

        bool sent = m_networkController.send(
            PacketsFabric::getCreateCallPacket(m_myPublicKey, m_myNickname, *m_call),
            PacketType::CREATE_CALL
        );

        using namespace std::chrono_literals;
        m_timer.start(12s, [this]() {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_state = State::FREE;
            m_callbacksQueue.push([this]() {m_createCallResultCallback(CreateCallResult::TIMEOUT); });
        });
    }
}

void CallsClient::onInputVoice(const unsigned char* data, int length) {
    size_t cipherDataLength = static_cast<size_t>(length) + CryptoPP::AES::BLOCKSIZE;
    std::vector<CryptoPP::byte> cipherData(cipherDataLength);
    crypto::AESEncrypt(m_call.value().getCallKey().value(), data, length, cipherData.data(), cipherDataLength);

    m_networkController.send(cipherData.data(), cipherDataLength, PacketType::VOICE);
}