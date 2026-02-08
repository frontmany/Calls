#pragma once

#include "logic/clientStateManager.h"
#include "logic/keyManager.h"
#include "constants/packetType.h"

#include <functional>
#include <memory>
#include <string>
#include <cstdint>

namespace core::logic
{
    class AuthorizationService {
    public:
        AuthorizationService(
            std::shared_ptr<ClientStateManager> stateManager,
            std::shared_ptr<KeyManager> keyManager,
            std::function<uint16_t()>&& getLocalUdpPort,
            std::function<std::error_code(const std::vector<unsigned char>&, core::constant::PacketType)>&& sendPacket
        );

        ~AuthorizationService() = default;

        std::error_code authorize(const std::string& nickname);
        std::error_code logout();

    private:
        std::shared_ptr<ClientStateManager> m_stateManager;
        std::shared_ptr<KeyManager> m_keyManager;
        std::function<uint16_t()>&& m_getLocalUdpPort;
        std::function<std::error_code(const std::vector<unsigned char>&, core::constant::PacketType)> m_sendPacket;
    };
}