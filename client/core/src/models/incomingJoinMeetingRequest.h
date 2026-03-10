#pragma once

#include "utilities/crypto.h"
#include <string>

namespace core
{
    class IncomingJoinMeetingRequest {
    public:
        IncomingJoinMeetingRequest(std::string nickname, CryptoPP::RSA::PublicKey publicKey);

        const std::string& getNickname() const;
        const CryptoPP::RSA::PublicKey& getPublicKey() const;

    private:
        std::string m_nickname;
        CryptoPP::RSA::PublicKey m_publicKey;
    };
}