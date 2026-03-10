#include "pendingMeetingJoinRequest.h"

#include <chrono>

using namespace std::chrono_literals;

namespace server
{
    PendingMeetingJoinRequest::PendingMeetingJoinRequest(const UserPtr& requester, const MeetingPtr& meeting, std::function<void()> onTimeout)
        : m_requester(requester)
        , m_meeting(meeting)
    {
        m_timer.start(65s, std::move(onTimeout));
    }

    const UserPtr& PendingMeetingJoinRequest::getRequester() const
    {
        return m_requester;
    }

    MeetingPtr PendingMeetingJoinRequest::getMeeting() const
    {
        return m_meeting.lock();
    }

    void PendingMeetingJoinRequest::stop()
    {
        m_timer.stop();
    }
}
