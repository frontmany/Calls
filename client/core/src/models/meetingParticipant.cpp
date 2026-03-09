#include "meetingParticipant.h"
#include "utilities/crypto.h"

namespace core
{
    MeetingParticipant::MeetingParticipant(bool isOwner, std::shared_ptr<User> user)
        : m_isOwner(isOwner)
        , m_user(std::move(user))
    {
    }

    MeetingParticipant::MeetingParticipant(bool isOwner, const User& user)
        : m_isOwner(isOwner)
        , m_user(std::make_shared<User>(user.getNickname(),
              static_cast<const CryptoPP::RSA::PublicKey&>(user.getPublicKey())))
    {
    }

    bool MeetingParticipant::isOwner() const {
        return m_isOwner;
    }

    void MeetingParticipant::setOwner(bool isOwner) {
        m_isOwner = isOwner;
    }

    const User& MeetingParticipant::getUser() const {
        return *m_user;
    }

    std::shared_ptr<User> MeetingParticipant::getUserPtr() const {
        return m_user;
    }
}
