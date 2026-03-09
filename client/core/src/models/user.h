#pragma once

#include "utilities/crypto.h"
#include <string>

namespace core
{
    class User {
    public:
        User(const std::string& nickname, const CryptoPP::RSA::PublicKey& publicKey);
        bool isConnected() const;
        const std::string& getNickname() const;
        const CryptoPP::PublicKey& getPublicKey() const;
        void setConnectionDown(bool value);

    private:
        bool m_connected = true;
        std::string m_nickname;
        CryptoPP::RSA::PublicKey m_publicKey;
    };
}