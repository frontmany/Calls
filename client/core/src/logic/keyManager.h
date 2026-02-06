#pragma once 

#include <future>
#include <mutex>
#include <atomic>

#include "utilities/crypto.h"

namespace core 
{
    class KeyManager {
    public:
        KeyManager() = default;
        ~KeyManager();

        void generateKeys();
        void resetKeys();
        bool keysAvailable() const;
        bool isGeneratingKeys() const;
        void awaitKeysGeneration();
        const CryptoPP::RSA::PublicKey& getMyPublicKey();
        const CryptoPP::RSA::PrivateKey& getMyPrivateKey();

    private:
        mutable std::mutex m_mutex;
        CryptoPP::RSA::PublicKey m_publicKey;
        CryptoPP::RSA::PrivateKey m_privateKey;
        std::future<void> m_future;
        std::atomic<bool> m_keysReady{ false };
        std::atomic<bool> m_generating{ false };
    };
}