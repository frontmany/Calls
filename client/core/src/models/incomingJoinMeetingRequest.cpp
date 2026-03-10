#include "incomingJoinMeetingRequest.h"

namespace core
{
    IncomingJoinMeetingRequest::IncomingJoinMeetingRequest(std::string nickname, CryptoPP::RSA::PublicKey publicKey)
        : m_nickname(std::move(nickname))
        , m_publicKey(std::move(publicKey))
    {
    }

    const std::string& IncomingJoinMeetingRequest::getNickname() const {
        return m_nickname;
    }

    const CryptoPP::RSA::PublicKey& IncomingJoinMeetingRequest::getPublicKey() const {
        return m_publicKey;
    }
}
