#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include "constants/packetType.h"
#include "media/audio/audioEngine.h"
#include "media/processing/mediaProcessingService.h"
#include "eventListener.h"

#include "json.hpp"

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
            std::function<std::error_code(const std::vector<unsigned char>&, core::constant::PacketType)>&& sendPacket
        );

        void processPacket(const unsigned char* data, int length, core::constant::PacketType type);

    private:
        std::unique_ptr<AuthorizationPacketHandler>& m_authorizationPacketHandler;
        std::unique_ptr<CallPacketHandler>& m_callPacketHandler;
        std::unique_ptr<MediaPacketHandler>& m_mediaPacketHandler;
        std::unique_ptr<ReconnectionPacketHandler>& m_reconnectionPacketHandler;
        std::unordered_map<core::constant::PacketType, std::function<void(const nlohmann::json&)>> m_packetHandlers;
    };
}

