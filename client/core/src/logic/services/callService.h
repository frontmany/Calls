#pragma once

#include "logic/clientStateManager.h"
#include "logic/keyManager.h"
#include "constants/packetType.h"

#include <functional>
#include <memory>


namespace core::logic
{ 
    class CallService {
    public:
        CallService(
            std::shared_ptr<ClientStateManager> stateManager,
            std::shared_ptr<KeyManager> keyManager,
            std::function<std::error_code(const std::vector<unsigned char>&, core::constant::PacketType)>&& sendPacket
        );

        std::error_code startOutgoingCall(const std::string& nickname);
        std::error_code stopOutgoingCall();
        std::error_code acceptCall(const std::string& nickname);
        std::error_code declineCall(const std::string& nickname);
        std::error_code endCall();

    private:
        void changeStateOnEndCall();

    private:
        std::shared_ptr<ClientStateManager> m_stateManager;
        std::shared_ptr<KeyManager> m_keyManager;
        std::function<std::error_code(const std::vector<unsigned char>&, core::constant::PacketType)> m_sendPacket;
    };
}
