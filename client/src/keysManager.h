# pragma once 

#include <future>
#include "crypto.h"

namespace calls {
    class KeysManager {
    public:
        KeysManager();
        ~KeysManager();

        void generateKeys();
        void resetKeys();
        const CryptoPP::RSA::PublicKey& getPublicKey();
        const CryptoPP::RSA::PrivateKey& getPrivateKey();

    private:
        CryptoPP::RSA::PublicKey m_publicKey;
        CryptoPP::RSA::PrivateKey m_privateKey;
        std::future<void> m_future;
    };
}

