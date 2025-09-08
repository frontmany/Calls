#include "callsClient.h"
#include "packetsFactory.h"

#include "json.hpp"

CallsClient::CallsClient(std::function<void(AuthorizationResult)> authorizationResultCallback, std::function<void(CreateCallResult)> createCallResultCallback, std::function<void(const IncomingCallData&)> onIncomingCall, std::function<void()> onCallHangUpCallback, std::function<void()> onNetworkErrorCallback) :
    m_authorizationResultCallback(authorizationResultCallback),
    m_createCallResultCallback(createCallResultCallback),
    m_onNetworkErrorCallback(onNetworkErrorCallback),
    m_onIncomingCall(onIncomingCall),
    m_onCallHangUpCallback(onCallHangUpCallback),
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
    m_networkController.connect();
}

CallsClient::~CallsClient() {
    if (m_queueProcessingThread.joinable()) {
        m_queueProcessingThread.join();
    }

    m_networkController.disconnect();

    if (m_audioEngine.isInitialized() && m_audioEngine.isStream()) {
        m_audioEngine.stopStream();
    }
}

void CallsClient::onReceiveCallback(const unsigned char* data, int length, PacketType type) {
    if (type == PacketType::VOICE) {
        // TODO
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);

    switch (type) {
    case (PacketType::AUTHORIZATION_SUCCESS):
        m_timer.stop();
        m_state = State::FREE;
        m_callbacksQueue.push([this]() {m_authorizationResultCallback(AuthorizationResult::SUCCESS); });
        break;
    case (PacketType::AUTHORIZATION_FAIL):
        m_timer.stop();
        m_myNickname.clear();
        m_callbacksQueue.push([this]() {m_authorizationResultCallback(AuthorizationResult::FAIL); });
        break;
    case (PacketType::FRIEND_INFO_SUCCESS):
        m_timer.stop();
        if (bool parsed = parsePacket(data, length, type); parsed) {
            m_call.value().createCallKey();
            startCalling();
        }
        break;
    case (PacketType::FRIEND_INFO_FAIL):
        m_timer.stop();
        m_callbacksQueue.push([this]() {m_createCallResultCallback(CreateCallResult::WRONG_FRIEND_NICKNAME_RESULT); });
        break;
    case (PacketType::CALL_ACCEPTED):
        m_timer.stop();
        m_state = State::BUSY;
        m_callbacksQueue.push([this]() {m_createCallResultCallback(CreateCallResult::CALL_ACCEPTED); });
        break;
    case (PacketType::CALL_DECLINED):
        m_timer.stop();
        m_state = State::FREE;
        m_callbacksQueue.push([this]() {m_createCallResultCallback(CreateCallResult::CALL_DECLINED); });
        break;
    case (PacketType::END_CALL):
        if (m_state == State::BUSY) {
            m_audioEngine.stopStream();
            m_callbacksQueue.push([this]() {m_onCallHangUpCallback(); });
            m_state = State::FREE;
        }
        break;
    case (PacketType::INCOMING_CALL):
        if (bool parsed = parsePacket(data, length, type); parsed && m_state == State::FREE) {
            m_callbacksQueue.push([this]() {m_onIncomingCall(m_incomingCalls.back()); });
        }
        else {
            declineIncomingCall(m_incomingCalls.back().friendNickname);
        }
        break;
    }
}

void CallsClient::authorize(const std::string& nickname) {
    m_myNickname = nickname;

    m_networkController.send(
        PacketsFactory::getAuthorizationPacket(m_myPublicKey, nickname),
        PacketType::AUTHORIZE
    );

    using namespace std::chrono_literals;
    m_timer.start(4s, [this]() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_callbacksQueue.push([this]() {m_authorizationResultCallback(AuthorizationResult::TIMEOUT); });
    });
}

void CallsClient::endCall() {
    if (m_state == State::UNAUTHORIZED || !m_call) return;
    m_state = State::FREE;
    m_audioEngine.stopStream();
    m_networkController.send(PacketType::END_CALL);
    m_call = std::nullopt;
}

void CallsClient::createCall(const std::string& friendNickname) {
    if (m_state == State::UNAUTHORIZED || m_call) return;
    m_state = State::CALLING;
    requestFriendInfo(friendNickname);
}

void CallsClient::declineIncomingCall(const std::string& friendNickname) {
    if (m_state == State::UNAUTHORIZED || m_incomingCalls.empty()) return;
    
    m_networkController.send(
        PacketsFactory::getDeclineCallPacket(friendNickname), 
        PacketType::CALL_DECLINED
    );

    auto it = std::find_if(m_incomingCalls.begin(), m_incomingCalls.end(), [&friendNickname](IncomingCallData data) {
        return data.friendNickname == friendNickname;
    });
    m_incomingCalls.erase(it);
}

void CallsClient::acceptIncomingCall(const std::string& friendNickname) {
    if (m_state == State::UNAUTHORIZED || m_incomingCalls.empty()) return;

    m_state = State::BUSY;
    auto it = std::find_if(m_incomingCalls.begin(), m_incomingCalls.end(), [&friendNickname](IncomingCallData data) {
        return data.friendNickname == friendNickname;
    });
    m_call = Call(*it);
    m_incomingCalls.erase(it);

    for (auto& incomingCall : m_incomingCalls) {
        m_networkController.send(
            PacketsFactory::getDeclineCallPacket(incomingCall.friendNickname),
            PacketType::CALL_DECLINED
        );
    }
    m_incomingCalls.clear();
    
    m_networkController.send(
        PacketsFactory::getAcceptCallPacket(friendNickname),
        PacketType::CALL_ACCEPTED
    );

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
    m_networkController.send(
        PacketsFactory::getRequestFriendInfoPacket(friendNickname),
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
        std::cout << "Packet parsing error: " << e.what() << std::endl;
        return false;
    }
}

void CallsClient::startCalling() {
    if (m_call) {
        m_networkController.send(
            PacketsFactory::getCreateCallPacket(m_myPublicKey, m_myNickname, *m_call),
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
    if (m_call) {
        size_t cipherDataLength = static_cast<size_t>(length) + CryptoPP::AES::BLOCKSIZE;
        std::vector<CryptoPP::byte> cipherData(cipherDataLength);
        crypto::AESEncrypt(m_call.value().getCallKey(), data, length, cipherData.data(), cipherDataLength);

        m_networkController.send(std::move(cipherData), PacketType::VOICE);
    }
}