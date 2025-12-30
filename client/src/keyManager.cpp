#include "keyManager.h"

namespace calls {
    KeyManager::~KeyManager()
    {
        if (m_future.valid()) {
            m_future.wait();
        }
    }

    void KeyManager::generateKeys() {
        if (m_generating.load()) {
            return;
        }

        std::lock_guard<std::mutex> lock(m_mutex);
        m_future = std::async(std::launch::async, [this]() {
            utilities::crypto::generateRSAKeyPair(m_privateKey, m_publicKey);
            m_generating.store(false);
            m_keysReady.store(true);
        });

        m_generating.store(true);
        m_keysReady.store(false);
    }

    bool KeyManager::isKeys() const {
        return m_keysReady.load();
    }

    bool KeyManager::isGeneratingKeys() const {
        return m_generating.load();
    }

    void KeyManager::awaitKeysGeneration() {
        if (m_future.valid()) {
            m_future.wait();
        }

        m_keysReady.store(true);
        m_generating.store(false);
    }

    void KeyManager::resetKeys() {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_future.valid()) {
            m_future.wait();
        }

        m_publicKey = CryptoPP::RSA::PublicKey();
        m_privateKey = CryptoPP::RSA::PrivateKey();

        m_keysReady.store(false);
        m_generating.store(false);
    }

    const CryptoPP::RSA::PublicKey& KeyManager::getPublicKey() {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_future.valid()) {
            m_future.wait();
        }

        return m_publicKey;
    }

    const CryptoPP::RSA::PrivateKey& KeyManager::getPrivateKey() {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_future.valid()) {
            m_future.wait();
        }

        return m_privateKey;
    }
}