#pragma once

#include <queue>
#include <iostream>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <thread>
#include <chrono>

#include "networkController.h"
#include "audioEngine.h"
#include "pingManager.h"
#include "keysManager.h"
#include "packetTypes.h"
#include "incomingCallData.h"
#include "ErrorCode.h"
#include "handler.h"
#include "state.h"
#include "timer.h"
#include "call.h"
#include "task.h"

#include "json.hpp"

namespace calls {
class CallsClient {
public:
    static CallsClient& get();

    bool init(const std::string& host, const std::string& port, std::unique_ptr<Handler>&& handler);
    void run();
    void stop();

    // SET
    void refreshAudioDevices();
    void muteMicrophone(bool isMute);
    void muteSpeaker(bool isMute);
    void setInputVolume(int volume);
    void setOutputVolume(int volume);

    // GET
    bool isMicrophoneMuted();
    bool isSpeakerMuted();
    bool isRunning() const;
    bool isAuthorized() const;
    bool isCalling() const;
    bool isBusy() const;
    const std::string& getNickname() const;
    std::vector<std::string> getCallers();
    int getInputVolume() const;
    int getOutputVolume() const;
    int getIncomingCallsCount() const;
    const std::string& getNicknameWhomCalling() const;
    const std::string& getNicknameInCallWith() const;

    // flow functions
    bool logout();
    void reset();
    bool authorize(const std::string& nickname);
    bool startCalling(const std::string& friendNickname);
    bool stopCalling();
    bool declineCall(const std::string& friendNickname);
    bool acceptCall(const std::string& friendNickname);
    bool endCall();

private:
    // received results handlers
    void onAuthorizationSuccess(const nlohmann::json& jsonObject);
    void onAuthorizationFail(const nlohmann::json& jsonObject);

    void onLogoutOk(const nlohmann::json& jsonObject);

    void onFriendInfoSuccess(const nlohmann::json& jsonObject);
    void onFriendInfoFail(const nlohmann::json& jsonObject);

    void onStartCallingOk(const nlohmann::json& jsonObject);
    void onStartCallingFail(const nlohmann::json& jsonObject);
    void onEndCallOk(const nlohmann::json& jsonObject);
    void onStopCallingOk(const nlohmann::json& jsonObject);
    void onCallAcceptedOk(const nlohmann::json& jsonObject);
    void onCallAcceptedFail(const nlohmann::json& jsonObject);
    void onCallDeclinedOk(const nlohmann::json& jsonObject);

    // on received packets
    void onCallAccepted(const nlohmann::json& jsonObject);
    void onCallDeclined(const nlohmann::json& jsonObject);
    void onStopCalling(const nlohmann::json& jsonObject);
    void onEndCall(const nlohmann::json& jsonObject);
    void onIncomingCall(const nlohmann::json& jsonObject);

    // essential functions
    CallsClient() = default;
    ~CallsClient();
    void onReceive(const unsigned char* data, int length, PacketType type);
    void processQueue();
    void onInputVoice(const unsigned char* data, int length);
    bool validatePacket(const nlohmann::json& jsonObject);
    void onPingFail();
    void onConnectionRestored();

    // essential senders
    void sendAuthorizationPacket();
    void sendLogoutPacket(bool createTask);
    void sendRequestFriendInfoPacket(const std::string& friendNickname);
    void sendAcceptCallPacket(const std::string& friendNickname);
    void sendDeclineCallPacket(const std::string& friendNickname, bool createTask);
    void sendStartCallingPacket(const std::string& friendNickname, const CryptoPP::RSA::PublicKey& friendPublicKey, const CryptoPP::SecByteBlock& callKey);
    void sendStopCallingPacket(bool createTask);
    void sendEndCallPacket(bool createTask);
    void sendConfirmationPacket(const nlohmann::json& jsonObject, PacketType type);

private:
    bool m_running = false;
    State m_state = State::UNAUTHORIZED;
    mutable std::mutex m_dataMutex;

    std::optional<Call> m_call = std::nullopt;
    std::string m_nicknameWhomCalling;
    std::string m_myNickname;
    Timer m_callingTimer;

    std::thread m_callbacksQueueProcessingThread;
    std::queue<std::function<void()>> m_callbacksQueue;
    std::vector<std::pair<std::unique_ptr<Timer>, IncomingCallData>> m_incomingCalls;
    std::unordered_map<PacketType, std::function<void(const nlohmann::json&)>> m_handlers;
    std::unordered_map<std::string, std::shared_ptr<Task>> m_tasks;

    std::shared_ptr<NetworkController> m_networkController;
    std::unique_ptr<AudioEngine> m_audioEngine;
    std::shared_ptr<PingManager> m_pingManager;
    std::unique_ptr<KeysManager> m_keysManager;
    std::unique_ptr<Handler> m_callbackHandler;
};

}