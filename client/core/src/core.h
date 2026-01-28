#pragma once

#include <queue>
#include <iostream>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <vector>
#include <cstdint>
#include <memory>
#include <atomic>

#include "keyManager.h"
#include "errorCode.h"
#include "pendingRequests.h"
#include "packetType.h"
#include "eventListener.h"
#include "clientStateManager.h"
#include "packetProcessor.h"
#include "network/networkController.h"
#include "network/tcp_control_client.h"
#include "audio/audioEngine.h"
#include "userOperationManager.h"
#include "services/MediaEncryptionService.h"
#include "services/CallService.h"
#include "services/AuthorizationService.h"
#include "services/MediaSharingService.h"
#include "json.hpp"

namespace core
{
    void initializeDiagnostics(const std::string& appDirectory,
        const std::string& logDirectory,
        const std::string& crashDumpDirectory,
        const std::string& appVersion);

    class Client {
    public:
        Client();
        ~Client();

        bool start(const std::string& host, const std::string& tcpPort, const std::string& udpPort, std::shared_ptr<EventListener> eventListener);
        void stop();

        void refreshAudioDevices();
        void muteMicrophone(bool isMute);
        void muteSpeaker(bool isMute);
        void setInputVolume(int volume);
        void setOutputVolume(int volume);
        bool setInputDevice(int deviceIndex);
        bool setOutputDevice(int deviceIndex);

        bool isScreenSharing() const;
        bool isViewingRemoteScreen() const;
        bool isCameraSharing() const;
        bool isViewingRemoteCamera() const;
        bool isMicrophoneMuted() const;
        bool isSpeakerMuted() const;
        bool isAuthorized() const;
        bool isOutgoingCall() const;
        bool isActiveCall() const;
        bool isConnectionDown() const;
        int getInputVolume() const;
        int getOutputVolume() const;
        int getCurrentInputDevice() const;
        int getCurrentOutputDevice() const;
        int getIncomingCallsCount() const;
        std::vector<std::string> getCallers() const;
        const std::string& getMyNickname() const;
        const std::string& getNicknameWhomCalling() const;
        const std::string& getNicknameInCallWith() const;

        std::error_code authorize(const std::string& nickname);
        std::error_code logout();
        std::error_code startOutgoingCall(const std::string& friendNickname);
        std::error_code stopOutgoingCall();
        std::error_code acceptCall(const std::string& friendNickname);
        std::error_code declineCall(const std::string& friendNickname);
        std::error_code endCall();
        std::error_code startScreenSharing();
        std::error_code stopScreenSharing();
        std::error_code sendScreen(const std::vector<unsigned char>& data);
        std::error_code startCameraSharing();
        std::error_code stopCameraSharing();
        std::error_code sendCamera(const std::vector<unsigned char>& data);

    private:
        void onReceive(const unsigned char* data, int length, PacketType type);
        void onInputVoice(const unsigned char* data, int length);
        void reset();

        void onNetworkReceive(const unsigned char* data, int length, uint32_t type);
        void onConnectionDown();
        void onConnectionRestored();

        void onReconnectCompleted(std::optional<nlohmann::json> completionContext);
        void onReconnectFailed(std::optional<nlohmann::json> failureContext);

        void onAuthorizeCompleted(const std::string& nickname, std::optional<nlohmann::json> completionContext);
        void onAuthorizeFailed(const std::string& nickname, std::optional<nlohmann::json> failureContext);

        void onLogoutCompleted(std::optional<nlohmann::json> completionContext);
        void onLogoutFailed(std::optional<nlohmann::json> failureContext);

        void onRequestUserInfoCompleted(const std::string& userNickname, std::optional<nlohmann::json> completionContext);
        void onRequestUserInfoFailed(const std::string& userNickname, std::optional<nlohmann::json> failureContext);

        void onStartOutgoingCallCompleted(const std::string& userNickname, std::optional<nlohmann::json> completionContext);
        void onStartOutgoingCallFailed(const std::string& userNickname, std::optional<nlohmann::json> failureContext);
        void onOutgoingCallTimeout();

        void onStopOutgoingCallCompleted(const std::string& nickname, std::optional<nlohmann::json> completionContext);
        void onStopOutgoingCallFailed(const std::string& nickname, std::optional<nlohmann::json> failureContext);

        void onDeclineIncomingCallCompleted(const std::string& nickname, std::optional<nlohmann::json> completionContext);
        void onDeclineIncomingCallFailed(std::optional<nlohmann::json> failureContext);

        void onAcceptCallStopOutgoingCompleted(const std::string& userNickname, std::optional<nlohmann::json> completionContext);
        void onAcceptCallStopOutgoingFailed(const std::string& userNickname, std::optional<nlohmann::json> failureContext);
        void onAcceptCallEndActiveCompleted(const std::string& userNickname, std::optional<nlohmann::json> completionContext);
        void onAcceptCallEndActiveFailed(const std::string& userNickname, std::optional<nlohmann::json> failureContext);

        void onAcceptCallCompleted(const std::string& userNickname, std::optional<nlohmann::json> completionContext);
        void onAcceptCallFailed(const std::string& userNickname, std::optional<nlohmann::json> failureContext);
        void onAcceptCallFailedAfterEndCall(const std::string& userNickname, std::optional<nlohmann::json> failureContext);

        void onDeclineCallCompleted(const std::string& userNickname, std::optional<nlohmann::json> completionContext);
        void onDeclineCallFailed(const std::string& userNickname, std::optional<nlohmann::json> failureContext);

        void onEndCallCompleted(const std::string& nickname, std::optional<nlohmann::json> completionContext);
        void onEndCallFailed(const std::string& nickname, std::optional<nlohmann::json> failureContext);

        void onStartScreenSharingCompleted(std::optional<nlohmann::json> completionContext);
        void onStartScreenSharingFailed(std::optional<nlohmann::json> failureContext);
        void onStopScreenSharingCompleted(std::optional<nlohmann::json> completionContext);
        void onStopScreenSharingFailed(std::optional<nlohmann::json> failureContext);

        void onStartCameraSharingCompleted(std::optional<nlohmann::json> completionContext);
        void onStartCameraSharingFailed(std::optional<nlohmann::json> failureContext);
        void onStopCameraSharingCompleted(std::optional<nlohmann::json> completionContext);
        void onStopCameraSharingFailed(std::optional<nlohmann::json> failureContext);

    private:
        PendingRequests m_pendingRequests;
        ClientStateManager m_stateManager;
        KeyManager m_keyManager;
        core::network::NetworkController m_networkController;
        std::unique_ptr<core::network::TcpControlClient> m_tcpControl;
        core::audio::AudioEngine m_audioEngine;
        std::shared_ptr<EventListener> m_eventListener;
        std::unique_ptr<PacketProcessor> m_packetProcessor;
        UserOperationManager m_operationManager;
        core::services::MediaEncryptionService m_mediaEncryptionService;
        std::unique_ptr<core::services::CallService> m_callService;
        std::unique_ptr<core::services::AuthorizationService> m_authorizationService;
        std::unique_ptr<core::services::MediaSharingService> m_mediaSharingService;
    };
}