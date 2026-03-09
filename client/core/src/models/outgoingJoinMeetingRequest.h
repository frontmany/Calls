#pragma once

#include <string>
#include <chrono>
#include <functional>

#include "ticTimer.h"

namespace core
{
    class OutgoingJoinMeetingRequest {
    public:
        template <typename Rep, typename Period>
        explicit OutgoingJoinMeetingRequest(std::string meetingId,
            const std::chrono::duration<Rep, Period>& timeout,
            std::function<void()> onTimeout)
            : m_meetingId(std::move(meetingId))
        {
            m_timer.start(timeout, std::move(onTimeout));
        }
        OutgoingJoinMeetingRequest(const OutgoingJoinMeetingRequest&) = delete;
        OutgoingJoinMeetingRequest(OutgoingJoinMeetingRequest&& other) noexcept;
        ~OutgoingJoinMeetingRequest();

        const std::string& getMeetingId() const;
        const tic::SingleShotTimer& getTimer() const;
        void stop();

    private:
        std::string m_meetingId;
        tic::SingleShotTimer m_timer;
    };
}