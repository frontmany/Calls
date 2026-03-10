#pragma once

#include <memory>
#include <functional>
#include <vector>
#include "constants/packetType.h"
#include "eventListener.h"

#include "json.hpp"

namespace core::logic
{
    class ClientStateManager;
    class KeyManager;

    class MeetingPacketHandler {
    public:
        MeetingPacketHandler(
            std::shared_ptr<ClientStateManager> stateManager,
            std::shared_ptr<KeyManager> keyManager,
            std::shared_ptr<EventListener> eventListener,
            std::function<std::error_code(const std::vector<unsigned char>&, core::constant::PacketType)>&& sendPacket
        );

        void handleGetMeetingInfoResult(const nlohmann::json& jsonObject);
        void handleMeetingCreateResult(const nlohmann::json& jsonObject);
        void handleMeetingJoinRequest(const nlohmann::json& jsonObject);
        void handleMeetingJoinRequestCancelled(const nlohmann::json& jsonObject);
        void handleMeetingJoinAccept(const nlohmann::json& jsonObject);
        void handleMeetingJoinDecline(const nlohmann::json& jsonObject);
        void handleMeetingJoinRejected(const nlohmann::json& jsonObject);
        void handleMeetingEnded(const nlohmann::json& jsonObject);
        void handleMeetingParticipantJoined(const nlohmann::json& jsonObject);
        void handleMeetingParticipantLeft(const nlohmann::json& jsonObject);

    private:
        std::shared_ptr<ClientStateManager> m_stateManager;
        std::shared_ptr<KeyManager> m_keyManager;
        std::shared_ptr<EventListener> m_eventListener;
        std::function<std::error_code(const std::vector<unsigned char>&, core::constant::PacketType)> m_sendPacket;
    };
}
