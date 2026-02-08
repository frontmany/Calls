#include "authorizationPacketHandler.h"
#include "logic/clientStateManager.h"
#include "logic/keyManager.h"
#include "utilities/crypto.h"
#include "constants/errorCode.h"
#include "constants/jsonType.h"


using namespace core;
using namespace core::constant;
using namespace core::utilities::crypto;

namespace core::logic
{
    AuthorizationPacketHandler::AuthorizationPacketHandler(
        std::shared_ptr<ClientStateManager> stateManager,
        std::shared_ptr<KeyManager> keyManager,
        std::shared_ptr<EventListener> eventListener)
        : m_stateManager(stateManager)
        , m_keyManager(keyManager)
        , m_eventListener(eventListener)
    {
    }

    void AuthorizationPacketHandler::handleAuthorizationResult(const nlohmann::json& jsonObject) {
        bool authorized = jsonObject[RESULT].get<bool>();

        if (authorized) {
            const std::string& encryptedPacketKey = jsonObject[PACKET_KEY].get<std::string>();
            auto packetKey = RSADecryptAESKey(m_keyManager->getMyPrivateKey(), encryptedPacketKey);

            const std::string& encryptedNickname = jsonObject[ENCRYPTED_NICKNAME].get<std::string>();
            std::string nickname = AESDecrypt(packetKey, encryptedNickname);

            const std::string& token = jsonObject[TOKEN].get<std::string>();

            m_stateManager->setMyNickname(nickname);
            m_stateManager->setMyToken(token);
            m_stateManager->setAuthorized(true);

            if (m_eventListener)
                m_eventListener->onAuthorizationResult({});
        }
        else {
            if (m_eventListener)
                m_eventListener->onAuthorizationResult(make_error_code(ErrorCode::taken_nickname));
        }
    }
}