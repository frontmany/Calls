#include "authorizationService.h"
#include "logic/packetFactory.h"
#include "utilities/logger.h"
#include "constants/errorCode.h"

using namespace core::constant;

namespace core::logic
{
    AuthorizationService::AuthorizationService(
        std::shared_ptr<ClientStateManager> stateManager,
        std::shared_ptr<KeyManager> keyManager,
        std::function<uint16_t()>&& getLocalUdpPort,
        std::function<std::error_code(const std::vector<unsigned char>&, PacketType)>&& sendPacket)
        : m_stateManager(stateManager)
        , m_keyManager(keyManager)
        , m_getLocalUdpPort(std::move(getLocalUdpPort))
        , m_sendPacket(std::move(sendPacket))
    {
    }

    std::error_code AuthorizationService::authorize(const std::string& nickname) {
        if (m_stateManager->isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (m_stateManager->isAuthorized()) return make_error_code(ErrorCode::already_authorized);

        if (!m_keyManager->keysAvailable()) {
            if (!m_keyManager->isGeneratingKeys()) {
                m_keyManager->generateKeys();
            }
        }

        m_keyManager->awaitKeysGeneration();

        auto packet = PacketFactory::getAuthorizationPacket(nickname, m_keyManager->getMyPublicKey(), m_getLocalUdpPort());

        return m_sendPacket(packet, PacketType::AUTHORIZATION);
    }

    std::error_code AuthorizationService::logout() {
        if (m_stateManager->isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager->isAuthorized()) return make_error_code(ErrorCode::not_authorized);

        auto packet = PacketFactory::getNicknamePacket(m_stateManager->getMyNickname());

        auto ec = m_sendPacket(packet, PacketType::LOGOUT);

        if (ec) {
            return ec;
        }
        else {
            m_stateManager->reset();
        }

        return {};
    }
}