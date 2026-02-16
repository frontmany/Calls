#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include "constants/packetType.h"
#include "media/processing/mediaProcessingService.h"
#include "eventListener.h"

#include "json.hpp"

namespace core::media { class AudioEngine; }

namespace core::logic
{
    class AuthorizationPacketHandler;
    class CallPacketHandler;
    class MediaPacketHandler;
    class ReconnectionPacketHandler;
    class ClientStateManager;
    class KeyManager;

    class PacketHandleController {
    public:
        PacketHandleController(
            std::shared_ptr<ClientStateManager>& stateManager,
            std::shared_ptr<KeyManager>& keyManager,
            std::shared_ptr<media::AudioEngine> audioEngine,
            std::shared_ptr<media::MediaProcessingService> mediaProcessingService,
            std::shared_ptr<EventListener> eventListener,
            std::function<std::error_code(const std::vector<unsigned char>&, core::constant::PacketType)>&& sendPacket,
            std::function<void()> startAudioSharing = nullptr,
            std::function<void()> stopAudioSharing = nullptr
        );
        ~PacketHandleController();

        void processPacket(const unsigned char* data, int length, core::constant::PacketType type);

    private:
        void handleIncomingAudio(const unsigned char* data, int length);
        void handleIncomingScreen(const unsigned char* data, int length);
        void handleIncomingCamera(const unsigned char* data, int length);
        void handleAuthorizationResult(const nlohmann::json& jsonObject);
        void handleReconnectResult(const nlohmann::json& jsonObject);
        void handleUserInfoResult(const nlohmann::json& jsonObject);
        void handleIncomingCallBegin(const nlohmann::json& jsonObject);
        void handleIncomingCallEnded(const nlohmann::json& jsonObject);
        void handleCallAccepted(const nlohmann::json& jsonObject);
        void handleCallDeclined(const nlohmann::json& jsonObject);
        void handleCallEndedByRemote(const nlohmann::json& jsonObject);
        void handleScreenSharingStarted(const nlohmann::json& jsonObject);
        void handleScreenSharingStopped(const nlohmann::json& jsonObject);
        void handleCameraSharingStarted(const nlohmann::json& jsonObject);
        void handleCameraSharingStopped(const nlohmann::json& jsonObject);
        void handleRemoteUserConnectionDown(const nlohmann::json& jsonObject);
        void handleRemoteUserConnectionRestored(const nlohmann::json& jsonObject);
        void handleRemoteUserLogout(const nlohmann::json& jsonObject);

    private:
        std::function<std::error_code(const std::vector<unsigned char>&, core::constant::PacketType)> m_sendPacket;
        std::function<void()> m_startAudioSharing;
        std::function<void()> m_stopAudioSharing;
        std::unique_ptr<AuthorizationPacketHandler> m_authorizationPacketHandler;
        std::unique_ptr<CallPacketHandler> m_callPacketHandler;
        std::unique_ptr<MediaPacketHandler> m_mediaPacketHandler;
        std::unique_ptr<ReconnectionPacketHandler> m_reconnectionPacketHandler;
        std::unordered_map<core::constant::PacketType, std::function<void(const nlohmann::json&)>> m_packetHandlers;
    };
}

