#pragma once

#include <queue>
#include <iostream>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <thread>
#pragma once

#include <string>
#include <memory>

#include "media/audio/audioEngine.h"
#include "network/networkController.h"
#include "logic/services/authorizationService.h"
#include "logic/services/callService.h"
#include "logic/services/mediaService.h"
#include "logic/services/reconnectionService.h"
#include "logic/handlers/packetHandleController.h"
#include "logic/clientStateManager.h"

namespace core
{
    void initializeDiagnostics(const std::string& appDirectory,
        const std::string& logDirectory,
        const std::string& crashDumpDirectory,
        const std::string& appVersion
    );

    class Core {
    public:
        Core() = default;
        ~Core();

        bool start(
            const std::string& tcpHost,
            const std::string& udpHost,
            const std::string& tcpPort,
            const std::string& udpPort,
            std::shared_ptr<EventListener> eventListener
        );

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
        bool isConnectionDown() const;
        bool isOutgoingCall() const;
        bool isActiveCall() const;

        int getInputVolume() const;
        int getOutputVolume() const;
        int getCurrentInputDevice() const;
        int getCurrentOutputDevice() const;
        int getIncomingCallsCount() const;
        std::vector<std::string> getCallers() const;
        const std::string& getMyNickname() const;
        const std::string& getNicknameWhomOutgoingCall() const;
        const std::string& getNicknameInCallWith() const;

        std::error_code authorize(const std::string& nickname);
        std::error_code logout();
        std::error_code startOutgoingCall(const std::string& friendNickname);
        std::error_code stopOutgoingCall();
        std::error_code acceptCall(const std::string& friendNickname);
        std::error_code declineCall(const std::string& friendNickname);
        std::error_code endCall();
        std::error_code startScreenSharing(const media::ScreenCaptureTarget& target);
        std::error_code stopScreenSharing();
        std::error_code startCameraSharing(std::string deviceName);
        std::error_code stopCameraSharing();

    private:
        std::shared_ptr<logic::ClientStateManager> m_stateManager;
        std::shared_ptr<media::AudioEngine> m_audioEngine;
        std::unique_ptr<logic::AuthorizationService> m_authorizationService;
        std::unique_ptr<logic::CallService> m_callService;
        std::unique_ptr<logic::MediaService> m_mediaService;
        std::unique_ptr<logic::ReconnectionService> m_reconnectionService;
        std::unique_ptr<logic::PacketHandleController> m_packetHandleController;
        std::unique_ptr<network::NetworkController> m_networkController;
    };
}