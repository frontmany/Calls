#include "keysManager.h"

namespace calls {
    KeysManager::KeysManager() 
    {
    }

    KeysManager::~KeysManager() 
    {
        if (m_future.valid()) {
            m_future.wait();
        }
    }

    void KeysManager::generateKeys() {
        m_future = std::async(std::launch::async, [this]() {
            crypto::generateRSAKeyPair(m_privateKey, m_publicKey);
        });
    }

    void KeysManager::resetKeys() {
        m_publicKey = CryptoPP::RSA::PublicKey();
        m_privateKey = CryptoPP::RSA::PrivateKey();
    }

    const CryptoPP::RSA::PublicKey& KeysManager::getPublicKey() {
        if (m_future.valid()) {
            m_future.wait();
        }

        return m_publicKey;
    }

    const CryptoPP::RSA::PrivateKey& KeysManager::getPrivateKey() {
        if (m_future.valid()) {
            m_future.wait();
        }

        return m_privateKey;
    }
}

