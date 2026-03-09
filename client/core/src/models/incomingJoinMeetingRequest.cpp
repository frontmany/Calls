#include "incomingJoinMeetingRequest.h"

namespace core
{
    IncomingJoinMeetingRequest::IncomingJoinMeetingRequest(std::string nickname)
        : m_nickname(std::move(nickname))
    {
    }

    const std::string& IncomingJoinMeetingRequest::getNickname() const {
        return m_nickname;
    }
}
