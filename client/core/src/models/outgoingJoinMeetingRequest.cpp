#include "outgoingJoinMeetingRequest.h"

namespace core
{
    OutgoingJoinMeetingRequest::OutgoingJoinMeetingRequest(OutgoingJoinMeetingRequest&& other) noexcept
        : m_meetingId(std::move(other.m_meetingId))
        , m_timer(std::move(other.m_timer))
    {
    }

    OutgoingJoinMeetingRequest::~OutgoingJoinMeetingRequest() {
        stop();
    }

    const std::string& OutgoingJoinMeetingRequest::getMeetingId() const {
        return m_meetingId;
    }

    const tic::SingleShotTimer& OutgoingJoinMeetingRequest::getTimer() const {
        return m_timer;
    }

    void OutgoingJoinMeetingRequest::stop() {
        m_timer.stop();
    }
}
