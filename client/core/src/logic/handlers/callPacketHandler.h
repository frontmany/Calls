#pragma once

#include <memory>
#include "constants/packetType.h"
#include "eventListener.h"

#include "json.hpp"

namespace core::logic
{
    class ClientStateManager;
    class KeyManager;

    class CallPacketHandler {
    public:
        CallPacketHandler(
            std::shared_ptr<ClientStateManager> stateManager,
            std::shared_ptr<KeyManager> keyManager,
            std::shared_ptr<EventListener> eventListener,
            std::function<std::error_code(const std::vector<unsigned char>&, core::constant::PacketType)>&& sendPacket
        );

        void handleGetUserInfoResult(const nlohmann::json& jsonObject);
        void handleIncomingCallBegin(const nlohmann::json& jsonObject);
        void handleIncomingCallEnd(const nlohmann::json& jsonObject);
        void handleOutgoingCallAccepted(const nlohmann::json& jsonObject);
        void handleOutgoingCallDeclined(const nlohmann::json& jsonObject);
        void handleCallEndedByRemote(const nlohmann::json& jsonObject);
        void handleRemoteUserConnectionDown(const nlohmann::json& jsonObject);
        void handleRemoteUserConnectionRestored(const nlohmann::json& jsonObject);
        void handleRemoteUserLogout(const nlohmann::json& jsonObject);

    private:
        void changeStateOnEndCall();

    private:
        std::shared_ptr<ClientStateManager> m_stateManager;
        std::shared_ptr<KeyManager> m_keyManager;
        std::shared_ptr<EventListener> m_eventListener;
        std::function<std::error_code(const std::vector<unsigned char>&, core::constant::PacketType)> m_sendPacket;
    }
}