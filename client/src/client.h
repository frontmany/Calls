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
#include "call.h"
#include "clientStateManager.h"
#include "packetProcessor.h"

#include "network/networkController.h"
#include "audio/audioEngine.h"
#include "utilities/timer.h"

#include "json.hpp"

namespace calls
{
    class CallsClient {
    public:
        static CallsClient& get();

        bool init(const std::string& host,
            const std::string& port,
            std::unique_ptr<EventListener>&& eventListener
        );

        void refreshAudioDevices();
        void muteMicrophone(bool isMute);
        void muteSpeaker(bool isMute);
        void setInputVolume(int volume);
        void setOutputVolume(int volume);

        bool isScreenSharing();
        bool isViewingRemoteScreen();
        bool isCameraSharing();
        bool isViewingRemoteCamera();
        bool isMicrophoneMuted();
        bool isSpeakerMuted();
        bool isRunning() const;
        bool isAuthorized() const;
        bool isOutgoingCall() const;
        bool isActiveCall() const;
        bool isConnectionDown() const;
        int getInputVolume() const;
        int getOutputVolume() const;
        int getIncomingCallsCount() const;
        std::vector<std::string> getCallers();
        const std::string& getMyNickname() const;
        const std::string& getNicknameWhomCalling() const;
        const std::string& getNicknameInCallWith() const;

        bool authorize(const std::string& nickname);
        bool logout();
        bool startCalling(const std::string& friendNickname);
        bool stopCalling();
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
        CallsClient() = default;
        ~CallsClient();
        void onReceive(const unsigned char* data, int length, PacketType type);
        void onVoice(const unsigned char* data, int length);
        void onScreen(const unsigned char* data, int length);
        void onCamera(const unsigned char* data, int length);
        void onInputVoice(const unsigned char* data, int length);
        void reset();

    private:
        TaskManager<long long, std::milli> m_taskManager;
        ClientStateManager m_stateManager;
        PacketProcessor m_packetProcessor;
        KeyManager m_keysManager;
        network::NetworkController m_networkController;
        audio::AudioEngine m_audioEngine;
        std::unique_ptr<EventListener> m_eventListener;
    };
}