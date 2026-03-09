#pragma once

#include "logic/clientStateManager.h"
#include "constants/packetType.h"
#include "eventListener.h"

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <system_error>

namespace core::logic
{
    class MeetingService {
    public:
        MeetingService(
            std::shared_ptr<ClientStateManager> stateManager,
            std::shared_ptr<EventListener> eventListener,
            std::function<std::error_code(const std::vector<unsigned char>&, core::constant::PacketType)>&& sendPacket
        );

        std::error_code createMeeting();
        std::error_code joinMeeting(const std::string& meetingId);
        std::error_code cancelMeetingJoin();
        std::error_code endMeeting();
        std::error_code leaveMeeting();
        std::error_code acceptJoinMeetingRequest(const std::string& friendNickname);
        std::error_code declineJoinMeetingRequest(const std::string& friendNickname);

    private:
        static constexpr std::chrono::seconds kJoinMeetingRequestTimeout{60};

        std::shared_ptr<ClientStateManager> m_stateManager;
        std::shared_ptr<EventListener> m_eventListener;
        std::function<std::error_code(const std::vector<unsigned char>&, core::constant::PacketType)> m_sendPacket;
    };
}
