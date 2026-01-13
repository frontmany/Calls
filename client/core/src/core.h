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
#include "taskManager.h"
#include "packetType.h"
#include "eventListener.h"
#include "clientStateManager.h"
#include "packetProcessor.h"
#include "network/networkController.h"
#include "audio/audioEngine.h"
#include "userOperationManager.h"
#include "json.hpp"

namespace core
{
    class Client {
    public:
        Client();
        ~Client();

        bool start(const std::string& host, const std::string& port, std::shared_ptr<EventListener> eventListener);
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

        void createAndStartTask(
            const std::string& uid,
            const std::vector<unsigned char>& packet,
            PacketType packetType,
            std::function<void(std::optional<nlohmann::json>)> onCompletion,
            std::function<void(std::optional<nlohmann::json>)> onFailure);

    private:
        TaskManager<long long, std::milli> m_taskManager;
        ClientStateManager m_stateManager;
        KeyManager m_keyManager;
        core::network::NetworkController m_networkController;
        core::audio::AudioEngine m_audioEngine;
        std::shared_ptr<EventListener> m_eventListener;
        std::unique_ptr<PacketProcessor> m_packetProcessor;
        UserOperationManager m_operationManager;
    };
}