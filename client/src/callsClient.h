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
#include "clientStatus.h"
#include "timer.h"
#include "call.h"

namespace calls {
class CallsClient {
public:
    static CallsClient& get();

    void init(
        const std::string& host,
        std::function<void(Result)> authorizationResultCallback,
        std::function<void(Result)> createCallResultCallback,
        std::function<void(const std::string&)> onIncomingCall,
        std::function<void(const std::string&)> onIncomingCallExpired,
        std::function<void()> onCallHangUpCallback,
        std::function<void()> onNetworkErrorCallback
    );
    bool isRunning() const;
    const std::string& getNickname() const;
    ClientStatus getStatus() const;
    
    void run();
    void stop();

    void refreshAudioDevices();
    void mute(bool isMute);
    bool authorize(const std::string& nickname);
    bool createCall(const std::string& friendNickname);
    bool declineIncomingCall(const std::string& friendNickname);
    bool acceptIncomingCall(const std::string& friendNickname);
    bool endCall();
    void setInputVolume(int volume);
    void setOutputVolume(int volume);
    int getInputVolume() const;
    int getOutputVolume() const;

private:
    CallsClient();
    ~CallsClient();
    void onCallingEnd(const unsigned char* data, int length);
    void onReceiveCallback(const unsigned char* data, int length, PacketType type);
    bool onFriendInfoSuccess(const unsigned char* data, int length);
    bool onIncomingCall(const unsigned char* data, int length);
    void processQueue();
    void requestFriendInfo(const std::string& friendNickname);
    void startCalling();
    void onInputVoice(const unsigned char* data, int length);
    void logout();

private:
    std::atomic_bool m_mute = false;
    std::atomic_bool m_running = false;
    std::optional<Call> m_call = std::nullopt;
    std::string m_myNickname{};
    CryptoPP::RSA::PublicKey m_myPublicKey;
    CryptoPP::RSA::PrivateKey m_myPrivateKey;
    std::future<void> m_keysFuture;

    std::mutex m_queueCallbacksMutex;
    std::thread m_queueProcessingThread;
    std::queue<std::function<void()>> m_callbacksQueue;

    std::mutex m_incomingCallsMutex;
    std::vector<std::pair<std::unique_ptr<Timer>, IncomingCallData>> m_incomingCalls;


    std::unique_ptr<NetworkController> m_networkController;
    std::unique_ptr<AudioEngine> m_audioEngine;
    Timer m_timer;
    Timer m_checkConnectionTimer;
    ClientStatus m_status = ClientStatus::UNAUTHORIZED;

    std::function<void(Result)> m_authorizationResultCallback;
    std::function<void(Result)> m_createCallResultCallback;
    std::function<void(const std::string&)> m_onIncomingCall;
    std::function<void(const std::string& friendNickname)> m_onIncomingCallExpired;
    std::function<void()> m_onNetworkErrorCallback;
    std::function<void()> m_onCallHangUpCallback;

    static constexpr const char* PUBLIC_KEY = "publicKey";
    static constexpr const char* NICKNAME = "nickname";
    static constexpr const char* NICKNAME_HASH = "nicknameHash";
    static constexpr const char* CALL_KEY = "callKey";
    static constexpr const char* PACKET_KEY = "packetKey";
};

}
