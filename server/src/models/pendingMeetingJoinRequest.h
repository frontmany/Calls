#pragma once

#include <functional>
#include <memory>

#include "ticTimer.h"

namespace server
{
    class User;
    class Meeting;

    typedef std::shared_ptr<User> UserPtr;
    typedef std::shared_ptr<Meeting> MeetingPtr;

    class PendingMeetingJoinRequest {
    public:
        PendingMeetingJoinRequest(const UserPtr& requester, const MeetingPtr& meeting, std::function<void()> onTimeout);

        const UserPtr& getRequester() const;
        MeetingPtr getMeeting() const;
        void stop();

    private:
        UserPtr m_requester;
        std::weak_ptr<Meeting> m_meeting;
        tic::SingleShotTimer m_timer;
    };
}
