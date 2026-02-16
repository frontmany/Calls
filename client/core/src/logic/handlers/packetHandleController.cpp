#include "packetHandleController.h"

#include "constants/errorCode.h"
#include "constants/jsonType.h"
#include "logic/packetFactory.h"
#include "logic/clientStateManager.h"
#include "logic/keyManager.h"
#include "logic/handlers/authorizationPacketHandler.h"
#include "logic/handlers/callPacketHandler.h"
#include "logic/handlers/mediaPacketHandler.h"
#include "logic/handlers/reconnectionPacketHandler.h"
#include "utilities/logger.h"

using namespace core::constant;
using namespace core::utilities;
using namespace std::chrono_literals;

namespace core::logic
{
    PacketHandleController::PacketHandleController(
        std::shared_ptr<ClientStateManager>& stateManager,
        std::shared_ptr<KeyManager>& keyManager,
        std::shared_ptr<media::AudioEngine> audioEngine,
        std::shared_ptr<media::MediaProcessingService> mediaProcessingService,
        std::shared_ptr<EventListener> eventListener,
        std::function<std::error_code(const std::vector<unsigned char>&, core::constant::PacketType)>&& sendPacket,
        std::function<void()> startAudioSharing,
        std::function<void()> stopAudioSharing)
        : m_sendPacket(std::move(sendPacket))
        , m_startAudioSharing(std::move(startAudioSharing))
        , m_stopAudioSharing(std::move(stopAudioSharing))
    {
        m_authorizationPacketHandler = std::make_unique<AuthorizationPacketHandler>(stateManager, keyManager, eventListener);
        m_callPacketHandler = std::make_unique<CallPacketHandler>(stateManager, keyManager, eventListener,
            [this](const std::vector<unsigned char>& p, core::constant::PacketType t) { return m_sendPacket(p, t); });
        m_mediaPacketHandler = std::make_unique<MediaPacketHandler>(stateManager, audioEngine, mediaProcessingService, eventListener);
        m_reconnectionPacketHandler = std::make_unique<ReconnectionPacketHandler>(stateManager, eventListener,
            [this](const std::vector<unsigned char>& p, core::constant::PacketType t) { return m_sendPacket(p, t); },
            m_startAudioSharing);

        m_packetHandlers.emplace(PacketType::AUTHORIZATION_RESULT, [this](const nlohmann::json& json) { handleAuthorizationResult(json); });
        m_packetHandlers.emplace(PacketType::RECONNECT_RESULT, [this](const nlohmann::json& json) {handleReconnectResult(json); });
        m_packetHandlers.emplace(PacketType::GET_USER_INFO_RESULT, [this](const nlohmann::json& json) {handleUserInfoResult(json); });
        m_packetHandlers.emplace(PacketType::CALLING_BEGIN, [this](const nlohmann::json& json) {handleIncomingCallBegin(json); });
        m_packetHandlers.emplace(PacketType::CALLING_END, [this](const nlohmann::json& json) {handleIncomingCallEnded(json); });
        m_packetHandlers.emplace(PacketType::CALL_ACCEPT, [this](const nlohmann::json& json) {handleCallAccepted(json); });
        m_packetHandlers.emplace(PacketType::CALL_DECLINE, [this](const nlohmann::json& json) {handleCallDeclined(json); });
        m_packetHandlers.emplace(PacketType::CALL_END, [this](const nlohmann::json& json) {handleCallEndedByRemote(json); });
        m_packetHandlers.emplace(PacketType::SCREEN_SHARING_BEGIN, [this](const nlohmann::json& json) {handleScreenSharingStarted(json); });
        m_packetHandlers.emplace(PacketType::SCREEN_SHARING_END, [this](const nlohmann::json& json) {handleScreenSharingStopped(json); });
        m_packetHandlers.emplace(PacketType::CAMERA_SHARING_BEGIN, [this](const nlohmann::json& json) {handleCameraSharingStarted(json); });
        m_packetHandlers.emplace(PacketType::CAMERA_SHARING_END, [this](const nlohmann::json& json) {handleCameraSharingStopped(json); });
        m_packetHandlers.emplace(PacketType::CONNECTION_DOWN_WITH_USER, [this](const nlohmann::json& json) {handleRemoteUserConnectionDown(json); });
        m_packetHandlers.emplace(PacketType::CONNECTION_RESTORED_WITH_USER, [this](const nlohmann::json& json) {handleRemoteUserConnectionRestored(json); });
        m_packetHandlers.emplace(PacketType::USER_LOGOUT, [this](const nlohmann::json& json) {handleRemoteUserLogout(json); });
    }

    PacketHandleController::~PacketHandleController() = default;

    void PacketHandleController::processPacket(const unsigned char* data, int length, PacketType type)
    {
        if (type == PacketType::VOICE) {
            handleIncomingAudio(data, length);
        }
        else if (type == PacketType::SCREEN) {
            handleIncomingScreen(data, length);
        }
        else if (type == PacketType::CAMERA) {
            handleIncomingCamera(data, length);
        }
        else {
            if (m_packetHandlers.contains(type)) {
                auto& handlePacket = m_packetHandlers[type];

                try {
                    nlohmann::json jsonObject = nlohmann::json::parse(data, data + length);
                    handlePacket(jsonObject);
                }
                catch (const nlohmann::json::exception& e) {
                    LOG_ERROR("Failed to parse JSON packet: {}", e.what());
                }
            }
        }
    }

    void PacketHandleController::PacketHandleController::handleIncomingAudio(const unsigned char* data, int length) {
        m_mediaPacketHandler->handleIncomingAudio(data, length);
    }

    void PacketHandleController::handleIncomingScreen(const unsigned char* data, int length) {
        m_mediaPacketHandler->handleIncomingScreen(data, length);
    }

    void PacketHandleController::handleIncomingCamera(const unsigned char* data, int length) {
        m_mediaPacketHandler->handleIncomingCamera(data, length);
    }

    void PacketHandleController::handleAuthorizationResult(const nlohmann::json& jsonObject) {
        m_authorizationPacketHandler->handleAuthorizationResult(jsonObject);
    }

    void PacketHandleController::handleReconnectResult(const nlohmann::json& jsonObject) {
        m_reconnectionPacketHandler->handleReconnectResult(jsonObject);
    }

    void PacketHandleController::handleUserInfoResult(const nlohmann::json& jsonObject) {
        m_callPacketHandler->handleGetUserInfoResult(jsonObject);
    }

    void PacketHandleController::handleIncomingCallBegin(const nlohmann::json& jsonObject){
        m_callPacketHandler->handleIncomingCallBegin(jsonObject);
    }

    void PacketHandleController::handleIncomingCallEnded(const nlohmann::json& jsonObject){
        m_callPacketHandler->handleIncomingCallEnd(jsonObject);
    }

    void PacketHandleController::handleCallAccepted(const nlohmann::json& jsonObject) {
        m_callPacketHandler->handleOutgoingCallAccepted(jsonObject);
        if (m_startAudioSharing) {
            m_startAudioSharing();
        }
    }

    void PacketHandleController::handleCallDeclined(const nlohmann::json& jsonObject) {
        m_callPacketHandler->handleOutgoingCallDeclined(jsonObject);
    }

    void PacketHandleController::handleCallEndedByRemote(const nlohmann::json& jsonObject) {
        m_callPacketHandler->handleCallEndedByRemote(jsonObject);
        if (m_stopAudioSharing) {
            m_stopAudioSharing();
        }
    }

    void PacketHandleController::handleScreenSharingStarted(const nlohmann::json& jsonObject) {
        m_mediaPacketHandler->handleIncomingScreenSharingStarted(jsonObject);
    }

    void PacketHandleController::handleScreenSharingStopped(const nlohmann::json& jsonObject) {
        m_mediaPacketHandler->handleIncomingScreenSharingStopped(jsonObject);
    }

    void PacketHandleController::handleCameraSharingStarted(const nlohmann::json& jsonObject) {
        m_mediaPacketHandler->handleIncomingCameraSharingStarted(jsonObject);
    }

    void PacketHandleController::handleCameraSharingStopped(const nlohmann::json& jsonObject) {
        m_mediaPacketHandler->handleIncomingCameraSharingStopped(jsonObject);
    }

    void PacketHandleController::handleRemoteUserConnectionDown(const nlohmann::json& jsonObject) {
        m_callPacketHandler->handleRemoteUserConnectionDown(jsonObject);
        if (m_stopAudioSharing) {
            m_stopAudioSharing();
        }
    }

    void PacketHandleController::handleRemoteUserConnectionRestored(const nlohmann::json& jsonObject) {
        m_callPacketHandler->handleRemoteUserConnectionRestored(jsonObject);
        if (m_startAudioSharing) {
            m_startAudioSharing();
        }
    }

    void PacketHandleController::handleRemoteUserLogout(const nlohmann::json& jsonObject) {
        m_callPacketHandler->handleRemoteUserLogout(jsonObject);
        if (m_stopAudioSharing) {
            m_stopAudioSharing();
        }
    }
}