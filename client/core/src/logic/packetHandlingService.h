#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <mutex>

#include "objects/pendingRequests.h"
#include "protocol/packetType.h"
#include "media/processing/encryption/mediaEncryptionService.h"

#include "state/clientStateManager.h"
#include "state/userOperationManager.h"
#include "network/tcp/client.h"
#include "network/udp/client.h"
#include "json.hpp"
#include <memory>

namespace core::audio { class AudioEngine; }
namespace core::network::udp { class Client; } 

namespace core
{
    class ClientStateManager;
    class KeyManager;
    class EventListener;

    class PacketProcessingService {
    public:
        PacketProcesingService(ClientStateManager& stateManager,
            KeyManager& keysManager,
            PendingRequests& pendingRequests,
            core::network::udp::Client& mediaController,
            std::unique_ptr<core::network::tcp::Client>& controlController,
            core::audio::AudioEngine& audioEngine,
            std::shared_ptr<EventListener> eventListener,
            core::media::MediaEncryptionService& mediaEncryptionService);

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
        core::network::udp::Client& m_mediaController;
        core::audio::AudioEngine& m_audioEngine;
        ClientStateManager& m_stateManager;
        std::unique_ptr<core::network::tcp::Client>& m_controlController;
        KeyManager& m_keysManager;
        std::shared_ptr<EventListener> m_eventListener;
        std::function<void(std::optional<nlohmann::json>)> m_onOrphanReconnectSuccess;
    };
}

