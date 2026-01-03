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

#include "keyManager.h"
#include "taskManager.h"
#include "packetType.h"
#include "eventListener.h"
#include "clientStateManager.h"
#include "packetProcessor.h"
#include "network/networkController.h"
#include "audio/audioEngine.h"
#include "json.hpp"

namespace callifornia
{
    class Client {
    public:
        Client();
        ~Client();

        bool init(const std::string& host,
            const std::string& port,
            std::shared_ptr<EventListener> eventListener
        );

        void refreshAudioDevices();
        void muteMicrophone(bool isMute);
        void muteSpeaker(bool isMute);
        void setInputVolume(int volume);
        void setOutputVolume(int volume);

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
        int getIncomingCallsCount() const;
        std::vector<std::string> getCallers() const;
        const std::string& getMyNickname() const;
        const std::string& getNicknameWhomCalling() const;
        const std::string& getNicknameInCallWith() const;

        bool authorize(const std::string& nickname);
        bool logout();
        bool startOutgoingCall(const std::string& friendNickname);
        bool stopOutgoingCall();
        bool acceptCall(const std::string& friendNickname);
        bool declineCall(const std::string& friendNickname);
        bool endCall();
        bool startScreenSharing();
        bool stopScreenSharing();
        bool sendScreen(const std::vector<unsigned char>& data);
        bool startCameraSharing();
        bool stopCameraSharing();
        bool sendCamera(const std::vector<unsigned char>& data);

    private:
        void onReceive(const unsigned char* data, int length, PacketType type);
        void onVoice(const unsigned char* data, int length);
        void onScreen(const unsigned char* data, int length);
        void onCamera(const unsigned char* data, int length);
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
        network::NetworkController m_networkController;
        audio::AudioEngine m_audioEngine;
        std::shared_ptr<EventListener> m_eventListener;
        PacketProcessor m_packetProcessor;
    };
}