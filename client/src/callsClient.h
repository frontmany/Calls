#pragma once

#include <future>
#include <queue>
#include <iostream>
#include <mutex>
#include <optional>
#include <thread>
#include <chrono>

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
    CallsClient(const std::string& host, std::function<void(AuthorizationResult)> authorizationResultCallback,
        std::function<void(CreateCallResult)> createCallResultCallback,
        std::function<void(const IncomingCallData&)> onIncomingCall,
        std::function<void(const std::string& friendNickname)> onIncomingCallingExpired,
        std::function<void()> onCallHangUpCallback,
        std::function<void()> onNetworkErrorCallback
    );
    ~CallsClient();

    bool isAuthorized() const;
    bool isInCall() const;
    bool isRunning() const;
    const std::string& getNickname() const;
    void authorize(const std::string& nickname);
    void createCall(const std::string& friendNickname);
    void declineIncomingCall(const std::string& friendNickname);
    void acceptIncomingCall(const std::string& friendNickname);
    void endCall();
    void logout();

private:
    void onCallingEnd(const unsigned char* data, int length);
    void onReceiveCallback(const unsigned char* data, int length, PacketType type);
    bool onFriendInfoSuccess(const unsigned char* data, int length);
    bool onIncomingCall(const unsigned char* data, int length);
    void processQueue();
    void requestFriendInfo(const std::string& friendNickname);
    void startCalling();
    void onInputVoice(const unsigned char* data, int length);

private:
    bool m_running = true;
    std::optional<Call> m_call = std::nullopt;
    std::string m_myNickname{};
    CryptoPP::RSA::PublicKey m_myPublicKey;
    CryptoPP::RSA::PrivateKey m_myPrivateKey;


    std::mutex m_queueCallbacksMutex;
    std::thread m_queueProcessingThread;
    std::queue<std::function<void()>> m_callbacksQueue;

    std::mutex m_incomingCallsMutex;
    std::vector<std::pair<std::unique_ptr<Timer>, IncomingCallData>> m_incomingCalls;


    NetworkController m_networkController;
    AudioEngine m_audioEngine;
    Timer m_timer;
    State m_state = State::UNAUTHORIZED;

    std::function<void(AuthorizationResult)> m_authorizationResultCallback;
    std::function<void(CreateCallResult)> m_createCallResultCallback;
    std::function<void(const IncomingCallData&)> m_onIncomingCall;
    std::function<void(const std::string& friendNickname)> m_onIncomingCallingExpired;
    std::function<void()> m_onNetworkErrorCallback;
    std::function<void()> m_onCallHangUpCallback;

    static constexpr const char* PUBLIC_KEY = "publicKey";
    static constexpr const char* NICKNAME = "nickname";
    static constexpr const char* NICKNAME_HASH = "nicknameHash";
    static constexpr const char* CALL_KEY = "callKey";
    static constexpr const char* PACKET_KEY = "packetKey";
};
