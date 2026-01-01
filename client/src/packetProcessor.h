#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "taskManager.h"
#include "packetType.h"
#include "clientStateManager.h"
#include "json.hpp"

namespace audio
{
    class AudioEngine;
}

namespace network
{
    class NetworkController;
}

namespace calls
{
    class ClientStateManager;
    class KeyManager;

    class EventListener;

    class PacketProcessor {
    public:
        PacketProcessor(ClientStateManager& stateManager, 
            KeyManager& keysManager,
            TaskManager<long long, std::milli>& taskManager,
            network::NetworkController& networkController,
            audio::AudioEngine& audioEngine,
            std::shared_ptr<EventListener> eventListener
        );

        void processPacket(const unsigned char* data, int length, PacketType type);
        void onVoice(const unsigned char* data, int length);
        void onScreen(const unsigned char* data, int length);
        void onCamera(const unsigned char* data, int length);
        void onConfirmation(const nlohmann::json& jsonObject);
        void onAuthorizationResult(const nlohmann::json& jsonObject);
        void onReconnectResult(const nlohmann::json& jsonObject);
        void onUserInfoResult(const nlohmann::json& jsonObject);
        void onIncomingCallBegin(const nlohmann::json& jsonObject);
        void onIncomingCallEnded(const nlohmann::json& jsonObject);
        void onCallAccepted(const nlohmann::json& jsonObject);
        void onCallDeclined(const nlohmann::json& jsonObject);
        void onCallEnded(const nlohmann::json& jsonObject);
        void onScreenSharingBegin(const nlohmann::json& jsonObject);
        void onScreenSharingEnded(const nlohmann::json& jsonObject);
        void onCameraSharingBegin(const nlohmann::json& jsonObject);
        void onCameraSharingEnded(const nlohmann::json& jsonObject);
        void onConnectionDownWithUser(const nlohmann::json& jsonObject);
        void onConnectionRestoredWithUser(const nlohmann::json& jsonObject);


    private:
        void sendConfirmation(const std::string& userNickname, const std::string& uid);

    private:
        network::NetworkController& m_networkController;
        audio::AudioEngine& m_audioEngine;
        ClientStateManager& m_stateManager;
        TaskManager<long long, std::milli>& m_taskManager;
        KeyManager& m_keysManager;
        std::shared_ptr<EventListener> m_eventListener;
        std::unordered_map<PacketType, std::function<void(const nlohmann::json&)>> m_packetHandlers;
    };
}

