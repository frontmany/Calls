#pragma once

#include "constants/packetType.h"
#include <functional>
#include <memory>
#include <string>
#include <system_error>
#include <vector>
#include "eventListener.h"

#include "json.hpp"

namespace core::logic
{
    class ClientStateManager;

    class ReconnectionPacketHandler {
    public:
        using SendPacket = std::function<std::error_code(const std::vector<unsigned char>&, core::constant::PacketType)>;

        ReconnectionPacketHandler(
            std::shared_ptr<ClientStateManager> stateManager,
            std::shared_ptr<EventListener> eventListener,
            SendPacket sendPacket,
            std::function<void()> startAudioSharing = nullptr
        );

        void handleReconnectResult(const nlohmann::json& jsonObject);

    private:
        std::shared_ptr<ClientStateManager> m_stateManager;
        std::shared_ptr<EventListener> m_eventListener;
        SendPacket m_sendPacket;
        std::function<void()> m_startAudioSharing;
    };
}