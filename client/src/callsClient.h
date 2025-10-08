#pragma once

#include <queue>
#include <iostream>
#include <mutex>
#include <optional>
#include <thread>
#include <chrono>

#include "networkController.h"
#include "audioEngine.h"
#include "pingManager.h"
#include "keysManager.h"
#include "packetTypes.h"
#include "incomingCallData.h"
#include "result.h"
#include "state.h"
#include "timer.h"
#include "call.h"

namespace calls {
class CallsClient {
public:
    static CallsClient& get();

    bool init(
        const std::string& host, 
        const std::string& port,
        std::function<void(bool)> logoutResult,
        std::function<void(bool)> authorizationResult,
        std::function<void(bool)> startCallingResult,
        std::function<void(bool)> sopCallingResult,
        std::function<void(bool)> declineAllIncomingCallsResult,
        std::function<void(bool)> declineIncomingCallResult,
        std::function<void(bool)> acceptIncomingCallResult,
        std::function<void(bool)> endCallResult,
        std::function<void(bool)> callingStoppedResult,
        std::function<void()> onCallingAccepted,
        std::function<void()> onCallingDeclined,
        std::function<void(const std::string&)> onIncomingCall,
        std::function<void(const std::string&)> onIncomingCallExpired,
        std::function<void(const std::string&)> onCallingSomeoneWhoAlreadyCallingYou,
        std::function<void()> onRemoteUserEndedCall,
        std::function<void()> onNetworkError
    );
    void run();
    void stop();

    // SET
    void refreshAudioDevices();
    void mute(bool isMute);
    void setInputVolume(int volume);
    void setOutputVolume(int volume);

    // GET
    bool isMuted();
    bool isRunning() const;
    const std::string& getNickname() const;
    State getState() const;
    int getInputVolume() const;
    int getOutputVolume() const;
    int getIncomingCallsCount() const;
    const std::string& getNicknameWhomCalling() const;
    const std::string& getNicknameInCallWith() const;

    // flow functions
    bool logout();
    bool authorize(const std::string& nickname);
    bool startCalling(const std::string& friendNickname);
    bool stopCalling();
    bool declineIncomingCall(const std::string& friendNickname);
    bool acceptIncomingCall(const std::string& friendNickname);
    bool endCall();

private:
    // essential functions
    CallsClient();
    ~CallsClient();
    void onReceive(const unsigned char* data, int length, PacketType type);
    void processQueue();
    void onInputVoice(const unsigned char* data, int length);
    bool validatePacket(const unsigned char* data, int length);
    void onPingFail();

    // received results 
    void onAuthorizationSuccess(const unsigned char* data, int length);
    void onAuthorizationFail(const unsigned char* data, int length);
    void onLogoutOk(const unsigned char* data, int length);
    void onFriendInfoSuccess(const unsigned char* data, int length);
    void onFriendInfoFail(const unsigned char* data, int length);
    void onCreateCallSuccess(const unsigned char* data, int length);
    void onCreateCallFail(const unsigned char* data, int length);
    void onEndCallOk(const unsigned char* data, int length);
    void onCallingEndOk(const unsigned char* data, int length);
    void onCallAcceptedOk(const unsigned char* data, int length);
    void onCallDeclinedOk(const unsigned char* data, int length);

    // received packets
    void onCallAccepted(const unsigned char* data, int length);
    void onCallDeclined(const unsigned char* data, int length);
    void onCallingEnd(const unsigned char* data, int length);
    void onEndCall(const unsigned char* data, int length);
    void onCreateCall(const unsigned char* data, int length);



private:
    std::atomic_bool m_running = false;
    State m_state = State::UNAUTHORIZED;

    std::optional<Call> m_call = std::nullopt;
    std::string m_nicknameWhomCalling;
    std::string m_myNickname;

    mutable std::mutex m_callbacksQueueMutex;
    mutable std::mutex m_incomingCallsMutex;

    std::thread m_callbacksQueueProcessingThread;
    std::queue<std::function<void()>> m_callbacksQueue;
    std::vector<std::pair<std::unique_ptr<Timer>, IncomingCallData>> m_incomingCalls;

    Timer m_timer;
    Timer m_callingTimer;
    std::string m_waitingForConfirmationOnPacketUUID;
    std::unordered_map<std::string, Timer> m_timers;

    std::shared_ptr<NetworkController> m_networkController;
    std::unique_ptr<AudioEngine> m_audioEngine;
    std::shared_ptr<PingManager> m_pingManager;
    std::unique_ptr<KeysManager> m_keysManager;


    std::function<void(bool)> m_logoutResult;
    std::function<void(bool)> m_authorizationResult;
    std::function<void(bool)> m_startCallingResult;
    std::function<void(bool)> m_stopCallingResult;
    std::function<void(bool)> m_declineAllIncomingCallsResult;
    std::function<void(bool)> m_declineIncomingCallResult;
    std::function<void(bool)> m_acceptIncomingCallResult;
    std::function<void(bool)> m_endCallResult;
    std::function<void(bool)> m_callingStoppedResult;
    std::function<void()> m_onCallingAccepted;
    std::function<void()> m_onCallingDeclined;
    std::function<void(const std::string&)> m_onIncomingCall;
    std::function<void(const std::string& friendNickname)> m_onIncomingCallExpired;
    std::function<void(const std::string& friendNickname)> m_onCallingSomeoneWhoAlreadyCallingYou;
    std::function<void()> m_onNetworkError;
    std::function<void()> m_onRemoteUserEndedCall;
};

}