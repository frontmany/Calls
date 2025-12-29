#pragma once

#include <functional>
#include <string>
#include <vector>

#include "packetTypes.h"
#include "clientStateManager.h"
#include "json.hpp"

namespace calls
{
    class KeysManager;
    class CallbacksInterface;
}

namespace audio
{
    class AudioEngine;
}

namespace calls
{

    class PacketProcessor
    {
    public:
        using SendPacketCallback = std::function<void(std::vector<unsigned char>&&, PacketType)>;
        using SendConfirmationCallback = std::function<void(const nlohmann::json&, PacketType)>;
        using SendDeclineCallCallback = std::function<void(const std::string&, bool)>;
        using SendStartCallingCallback = std::function<void(const std::string&, const CryptoPP::RSA::PublicKey&, const CryptoPP::SecByteBlock&)>;
        using OnPacketConfirmedCallback = std::function<void(const std::string&)>;

        PacketProcessor(
            ClientStateManager& stateManager,
            KeysManager& keysManager,
            CallbacksInterface* callbackHandler,
            audio::AudioEngine* audioEngine,
            SendPacketCallback sendPacket,
            SendConfirmationCallback sendConfirmation,
            SendDeclineCallCallback sendDeclineCall,
            SendStartCallingCallback sendStartCalling,
            OnPacketConfirmedCallback onPacketConfirmed
        );

        void processPacket(PacketType type, const nlohmann::json& jsonObject);

        // Packet handlers
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
        void onScreenSharingStartedOk(const nlohmann::json& jsonObject);
        void onScreenSharingStartedFail(const nlohmann::json& jsonObject);
        void onScreenSharingStoppedOk(const nlohmann::json& jsonObject);
        void onCameraSharingStartedOk(const nlohmann::json& jsonObject);
        void onCameraSharingStartedFail(const nlohmann::json& jsonObject);
        void onCameraSharingStoppedOk(const nlohmann::json& jsonObject);

        // Incoming packet handlers
        void onIncomingScreenSharingStarted(const nlohmann::json& jsonObject);
        void onIncomingScreenSharingStopped(const nlohmann::json& jsonObject);
        void onIncomingCameraSharingStarted(const nlohmann::json& jsonObject);
        void onIncomingCameraSharingStopped(const nlohmann::json& jsonObject);
        void onCallAccepted(const nlohmann::json& jsonObject);
        void onCallDeclined(const nlohmann::json& jsonObject);
        void onStopCalling(const nlohmann::json& jsonObject);
        void onEndCall(const nlohmann::json& jsonObject);
        void onIncomingCall(const nlohmann::json& jsonObject);

        bool validatePacket(const nlohmann::json& jsonObject);

    private:
        ClientStateManager& m_stateManager;
        KeysManager& m_keysManager;
        std::unique_ptr<CallbacksInterface> m_callbackHandler;
        audio::AudioEngine* m_audioEngine;
        SendPacketCallback m_sendPacket;
        SendConfirmationCallback m_sendConfirmation;
        SendDeclineCallCallback m_sendDeclineCall;
        SendStartCallingCallback m_sendStartCalling;
        OnPacketConfirmedCallback m_onPacketConfirmed;

        std::unordered_map<PacketType, std::function<void(const nlohmann::json&)>> m_packetHandlers;
    };
}

