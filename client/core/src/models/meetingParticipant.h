#pragma once

#include "models/user.h"
#include <memory>

namespace core
{
    class MeetingParticipant {
    public:
        MeetingParticipant(bool isOwner, std::shared_ptr<User> user);
        MeetingParticipant(bool isOwner, const User& user);

        bool isOwner() const;
        void setOwner(bool isOwner);
        const User& getUser() const;
        std::shared_ptr<User> getUserPtr() const;

    private:
        bool m_isOwner = false;
        std::shared_ptr<User> m_user;
    };
}
