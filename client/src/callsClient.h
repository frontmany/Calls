#pragma once

#include <future>;
#include <queue>;
#include <unordered_map>;
#include <mutex>;

#include "networkController.h"
#include "audioEngine.h"
#include "packetTypes.h"
#include "incomingCallData.h"
#include "result.h"
#include "timer.h"
#include "call.h"

class CallsClient {
private:
    enum class State : uint32_t 
    {
        UNAUTHORIZED,
        FREE,
        CALLING,
        BUSY
    };

public:
    CallsClient(std::function<void(AuthorizationResult)> authorizationResultCallback,
        std::function<void(CreateCallResult)> createCallResultCallback,
        std::function<void(const IncomingCallData&)> onIncomingCall,
        std::function<void()> onNetworkErrorCallback
    );
    void authorize(const std::string& nickname);
    void createCall(const std::string& friendNickname);
    void declineCall(const std::string& friendNickname);
    void acceptCall(const std::string& friendNickname);
    void endCall();

private:
    void onReceiveCallback(const unsigned char* data, int length, PacketType type);
    bool parsePacket(const unsigned char* data, int length, PacketType type);
    void processQueue();
    void requestFriendInfo(const std::string& friendNickname);
    void startCalling();
    void onInputVoice(const unsigned char* data, int length);

private:
    std::optional<Call> m_call = std::nullopt;
    std::string m_myNickname;
    CryptoPP::RSA::PublicKey m_myPublicKey;
    CryptoPP::RSA::PrivateKey m_myPrivateKey;
    std::thread m_queueProcessingThread;
    std::queue<std::function<void()>> m_callbacksQueue;
    std::unordered_map< IncomingCallData> m_incomingCalls;
    std::mutex m_mutex;
    NetworkController m_networkController;
    AudioEngine m_audioEngine;
    Timer m_timer;
    State m_state = State::UNAUTHORIZED;

    std::function<void(AuthorizationResult)> m_authorizationResultCallback;
    std::function<void(CreateCallResult)> m_createCallResultCallback;
    std::function<void(const IncomingCallData&)> m_onIncomingCall;
    std::function<void()> m_onNetworkErrorCallback;

    static constexpr const char* PUBLIC_KEY = "publicKey";
    static constexpr const char* NICKNAME = "nickname";
    static constexpr const char* NICKNAME_HASH = "nicknameHash";
    static constexpr const char* CALL_KEY = "callKey";
    static constexpr const char* PACKET_KEY = "packetKey";
};
