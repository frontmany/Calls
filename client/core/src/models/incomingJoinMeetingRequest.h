#pragma once

#include <string>

namespace core
{
    class IncomingJoinMeetingRequest {
    public:
        explicit IncomingJoinMeetingRequest(std::string nickname);

        const std::string& getNickname() const;

    private:
        std::string m_nickname;
    };
}