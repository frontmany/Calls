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

#include "keysManager.h"
#include "tasksManager.h"
#include "packetTypes.h"
#include "incomingCallData.h"
#include "errorCode.h"
#include "callbacksInterface.h"
#include "clientState.h"
#include "call.h"
#include "task.h"
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
            std::unique_ptr<CallbacksInterface>&& callbacksHandler
        );
        void run();
        void stop();

        // SET
        void refreshAudioDevices();
        void muteMicrophone(bool isMute);
        void muteSpeaker(bool isMute);
        void setInputVolume(int volume);
        void setOutputVolume(int volume);

        // GET
        bool isScreenSharing();
        bool isViewingRemoteScreen();
        bool isCameraSharing();
        bool isViewingRemoteCamera();
        bool isMicrophoneMuted();
        bool isSpeakerMuted();
        bool isRunning() const;
        bool isAuthorized() const;
        bool isCalling() const;
        bool isBusy() const;
        bool isNetworkError() const;
        const std::string& getNickname() const;
        std::vector<std::string> getCallers();
        int getInputVolume() const;
        int getOutputVolume() const;
        int getIncomingCallsCount() const;
        const std::string& getNicknameWhomCalling() const;
        const std::string& getNicknameInCallWith() const;

        // flow functions
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
        // Media handlers
        void onVoice(const unsigned char* data, int length);
        void onScreen(const unsigned char* data, int length);
        void onCamera(const unsigned char* data, int length);

        // essential functions
        CallsClient() = default;
        ~CallsClient();
        void onReceive(const unsigned char* data, int length, PacketType type);
        void onInputVoice(const unsigned char* data, int length);
        void onConnectionDown();
        void onConnectionRestored();
        void reset();

        // essential senders
        void sendStartScreenSharingPacket();
        void sendStopScreenSharingPacket(bool createTask);
        void sendStartCameraSharingPacket();
        void sendStopCameraSharingPacket(bool createTask);
        void sendAuthorizationPacket();
        void sendLogoutPacket(bool createTask);
        void sendRequestFriendInfoPacket(const std::string& friendNickname);
        void sendAcceptCallPacket(const std::string& friendNickname);
        void sendDeclineCallPacket(const std::string& friendNickname, bool createTask);
        void sendStartCallingPacket(const std::string& friendNickname, const CryptoPP::RSA::PublicKey& friendPublicKey, const CryptoPP::SecByteBlock& callKey);
        void sendStopCallingPacket(bool createTask);
        void sendEndCallPacket(bool createTask);
        void sendConfirmation(const std::string& uid, const std::string& nicknameHashTo);
        void sendConfirmationPacket(const nlohmann::json& jsonObject, PacketType confirmationType);

        void sendPacketWithConfirmation(const std::string& uuid, std::vector<unsigned char> packet, 
            PacketType packetType, int maxRetries = 3);
        void onPacketConfirmed(const std::string& uuid);

    private:
        ClientStateManager m_stateManager;
        std::unique_ptr<PacketProcessor> m_packetProcessor;
        KeysManager m_keysManager;
        TasksManager m_tasksManager;
        network::NetworkController m_networkController;
        audio::AudioEngine m_audioEngine;
        std::unique_ptr<CallbacksInterface> m_callbackHandler;
        std::thread m_callbacksQueueProcessingThread;
    };

}