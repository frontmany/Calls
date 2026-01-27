#pragma once

#include <memory>
#include "packetType.h"
#include "asio.hpp"
#include "IUserRepository.h"
#include "ICallManager.h"
#include "IPacketSender.h"

namespace server
{
    namespace services
    {
        // Сервис для ретрансляции медиа-данных (voice, screen, camera)
        class MediaRelayService {
        public:
            MediaRelayService(IUserRepository& userRepository,
                            ICallManager& callManager,
                            IPacketSender& packetSender)
                : m_userRepository(userRepository)
                , m_callManager(callManager)
                , m_packetSender(packetSender)
            {
            }

            // Ретранслировать медиа-данные от одного пользователя к его партнеру по звонку
            bool relayMedia(const unsigned char* data, 
                          int size, 
                          PacketType type, 
                          const asio::ip::udp::endpoint& fromEndpoint)
            {
                // Найти пользователя по endpoint
                auto user = m_userRepository.findUserByEndpoint(fromEndpoint);
                if (!user || !user->isInCall()) {
                    return false;
                }

                // Получить партнера по звонку
                auto callPartner = m_callManager.getCallPartner(user->getNicknameHash());
                if (!callPartner) {
                    return false;
                }

                // Проверить, что партнер существует в репозитории и не отключен
                auto partnerInRepo = m_userRepository.findUserByNickname(callPartner->getNicknameHash());
                if (!partnerInRepo || partnerInRepo->isConnectionDown()) {
                    return false;
                }

                // Отправить данные партнеру
                return m_packetSender.send(data, size, static_cast<uint32_t>(type), partnerInRepo->getEndpoint());
            }

        private:
            IUserRepository& m_userRepository;
            ICallManager& m_callManager;
            IPacketSender& m_packetSender;
        };
    }
}
