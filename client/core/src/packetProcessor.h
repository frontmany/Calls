#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <mutex>

#include "pendingRequests.h"
#include "packetType.h"
#include "clientStateManager.h"
#include "services/IMediaEncryptionService.h"
#include "network/tcp/controlController.h"
#include "json.hpp"
#include <memory>

namespace core { namespace audio { class AudioEngine; } }
namespace core { namespace network { namespace udp { class MediaController; } } }

namespace core
{
    class ClientStateManager;
    class KeyManager;
    class EventListener;

    class PacketProcessor {
    public:
        PacketProcessor(ClientStateManager& stateManager,
            KeyManager& keysManager,
            PendingRequests& pendingRequests,
            core::network::udp::MediaController& mediaController,
            std::unique_ptr<core::network::tcp::ControlController>& controlController,
            core::audio::AudioEngine& audioEngine,
            std::shared_ptr<EventListener> eventListener,
            core::services::IMediaEncryptionService& mediaEncryptionService);

        void processPacket(const unsigned char* data, int length, PacketType type);
        void onVoice(const unsigned char* data, int length);
        void onScreen(const unsigned char* data, int length);
        void onCamera(const unsigned char* data, int length);
        void onAuthorizationResult(const nlohmann::json& jsonObject);
        void onReconnectResult(const nlohmann::json& jsonObject);
        void setOrphanReconnectSuccessHandler(std::function<void(std::optional<nlohmann::json>)> f);
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
        void onUserLogout(const nlohmann::json& jsonObject);

    private:
        mutable std::mutex m_mutex;
        core::network::udp::MediaController& m_mediaController;
        core::audio::AudioEngine& m_audioEngine;
        ClientStateManager& m_stateManager;
        PendingRequests& m_pendingRequests;
        std::unique_ptr<core::network::tcp::ControlController>& m_controlController;
        KeyManager& m_keysManager;
        std::shared_ptr<EventListener> m_eventListener;
        core::services::IMediaEncryptionService& m_mediaEncryptionService;
        std::unordered_map<PacketType, std::function<void(const nlohmann::json&)>> m_packetHandlers;
        std::function<void(std::optional<nlohmann::json>)> m_onOrphanReconnectSuccess;
    };
}

