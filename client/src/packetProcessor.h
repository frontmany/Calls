#pragma once

#include <functional>
#include <string>
#include <vector>

#include "packetType.h"
#include "clientStateManager.h"
#include "json.hpp"

namespace calls
{
    class KeysManager;
    class ClientEventsListener;
}

namespace audio
{
    class AudioEngine;
}

namespace calls
{

    class PacketProcessor {
    public:
        PacketProcessor(KeysManager& keysManager,
            TaskManager& taskManager,
            std::function<void(nlohmann::json, PacketType)> sendPacket
        );

        void processPacket(const nlohmann::json& jsonObject, PacketType type);
        void onConfirmation(const nlohmann::json& jsonObject);
        void onAuthorizationSuccess(const nlohmann::json& jsonObject);
        void onAuthorizationFail(const nlohmann::json& jsonObject);
        void onReconnectSuccess(const nlohmann::json& jsonObject);
        void onReconnectFail(const nlohmann::json& jsonObject);
        void onUserInfoSuccess(const nlohmann::json& jsonObject);
        void onUserInfoFail(const nlohmann::json& jsonObject);
        void onCallAccepted(const nlohmann::json& jsonObject);
        void onCallDeclined(const nlohmann::json& jsonObject);
        void onIncomingCall(const nlohmann::json& jsonObject);
        void onIncomingCallEnded(const nlohmann::json& jsonObject);
        void onCallEnded(const nlohmann::json& jsonObject);
        void onIncomingScreenSharingStarted(const nlohmann::json& jsonObject);
        void onIncomingScreenSharingStopped(const nlohmann::json& jsonObject);
        void onIncomingCameraSharingStarted(const nlohmann::json& jsonObject);
        void onIncomingCameraSharingStopped(const nlohmann::json& jsonObject);
        void onCallParticipantConnectionDown(const nlohmann::json& jsonObject);
        void onCallParticipantConnectionRestored(const nlohmann::json& jsonObject);

    private:
        ClientStateManager& m_stateManager;
        TaskManager& m_taskManager;
        KeysManager& m_keysManager;
        std::function<void(const nlohmann::json&, PacketType)> m_sendPacket;
        std::unordered_map<PacketType, std::function<void(const nlohmann::json&)>> m_packetHandlers;
    };
}

