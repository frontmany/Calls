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
#include "joinRequestData.h"
#include "result.h"
#include "state.h"
#include "timer.h"
#include "call.h"
#include "groupCall.h"

namespace calls {
class CallsClient {
public:
    static CallsClient& get();

    bool init(
        const std::string& host, 
        const std::string& port,
        std::function<void(Result)> authorizationResult,
        std::function<void(Result)> createCallResult,
        std::function<void(Result)> createGroupCallResult,
        std::function<void(const std::string&)> onJoinRequest,
        std::function<void(const std::string&)> onJoinRequestExpired,
        std::function<void(const std::string&)> onIncomingCall,
        std::function<void(const std::string&)> onIncomingCallExpired,
        std::function<void(const std::string&)> onCallingSomeoneWhoAlreadyCallingYou,
        std::function<void()> onRemoteUserEndedCall,
        std::function<void()> onNetworkError
    );

    void run();
    void stop();
    void logout();
    void refreshAudioDevices();
    void mute(bool isMute);
    bool isMuted();
    void setInputVolume(int volume);
    void setOutputVolume(int volume);
    bool isRunning() const;
    const std::string& getNickname() const;
    State getStatus() const;
    int getInputVolume() const;
    int getOutputVolume() const;
    int getIncomingCallsCount() const;
    const std::string& getNicknameWhomCalling() const;
    const std::string& getNicknameInCallWith() const;


    bool authorize(const std::string& nickname);
    bool createCall(const std::string& friendNickname);
    bool stopCalling();
    bool declineIncomingCall(const std::string& friendNickname);
    bool acceptIncomingCall(const std::string& friendNickname);
    bool endCall();
    
    void createGroupCall(const std::string& groupCallName);
    void allowJoin(const std::string& friendNickname);
    void declineJoin(const std::string& friendNickname);
    
private:
    CallsClient();
    ~CallsClient();

    void onIncomingCallingEnd(const unsigned char* data, int length);
    void onReceive(const unsigned char* data, int length, PacketType type);
    bool onFriendInfoSuccess(const unsigned char* data, int length);
    void onCreateGroupCallSuccess();
    void onJoinRequest(const unsigned char* data, int length);
    bool onIncomingCall(const unsigned char* data, int length);
    void processQueue();
    void requestFriendInfo(const std::string& friendNickname);
    void startCalling();
    void onInputVoice(const unsigned char* data, int length);

    void ping();
    void checkPing();

private:
    std::atomic_bool m_pingResult = false;
    std::atomic_bool m_running = false;
    std::optional<Call> m_call = std::nullopt;
    std::optional<GroupCall> m_groupCall = std::nullopt;

    std::string m_myNickname{};
    std::string m_nicknameWhomCalling{};
    std::string m_groupCallName{};

    CryptoPP::RSA::PublicKey m_myPublicKey;
    CryptoPP::RSA::PrivateKey m_myPrivateKey;
    std::future<void> m_keysFuture;

    mutable std::mutex m_mutex;
    std::thread m_thread;
    std::thread m_pingerThread;
    std::queue<std::function<void()>> m_queue;
    std::vector<std::pair<std::unique_ptr<Timer>, IncomingCallData>> m_incomingCalls;
    std::vector<std::pair<std::unique_ptr<Timer>, JoinRequestData>> m_joinRequests;

    std::unique_ptr<NetworkController> m_networkController;
    std::unique_ptr<AudioEngine> m_audioEngine;

    Timer m_timer;
    State m_state = State::UNAUTHORIZED;

    std::function<void(Result)> m_authorizationResult;
    std::function<void(Result)> m_createCallResult;
    std::function<void(Result)> m_createGroupCallResult;
    std::function<void(const std::string&)> m_onJoinRequest;
    std::function<void(const std::string&)> m_onJoinRequestExpired;
    std::function<void(const std::string&)> m_onIncomingCall;
    std::function<void(const std::string& friendNickname)> m_onIncomingCallExpired;
    std::function<void(const std::string& friendNickname)> m_onCallingSomeoneWhoAlreadyCallingYou;
    std::function<void()> m_onNetworkError;
    std::function<void()> m_onRemoteUserEndedCall;

    static constexpr const char* PUBLIC_KEY = "publicKey";
    static constexpr const char* NICKNAME = "nickname";
    static constexpr const char* NICKNAME_HASH = "nicknameHash";
    static constexpr const char* CALL_KEY = "callKey";
    static constexpr const char* PACKET_KEY = "packetKey";
};

}
