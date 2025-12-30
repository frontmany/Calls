#pragma once 

#include <future>
#include <mutex>
#include <atomic>

#include "utilities/crypto.h"

namespace calls 
{
    class KeyManager {
    public:
        KeyManager() = default;
        ~KeyManager();

        void generateKeys();
        void resetKeys();
        bool isKeys() const;
        bool isGeneratingKeys() const;
        void awaitKeysGeneration();
        const CryptoPP::RSA::PublicKey& getPublicKey();
        const CryptoPP::RSA::PrivateKey& getPrivateKey();

    private:
        mutable std::mutex m_mutex;
        CryptoPP::RSA::PublicKey m_publicKey;
        CryptoPP::RSA::PrivateKey m_privateKey;
        std::future<void> m_future;
        std::atomic<bool> m_keysReady{ false };
        std::atomic<bool> m_generating{ false };
    };
}