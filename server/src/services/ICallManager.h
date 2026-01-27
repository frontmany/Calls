#pragma once

#include <memory>
#include <string>
#include <vector>
#include "user.h"
#include "call.h"
#include "pendingCall.h"

namespace server
{
    namespace services
    {
        // Интерфейс для управления звонками
        class ICallManager {
        public:
            virtual ~ICallManager() = default;

            // Проверить, находится ли пользователь в звонке
            virtual bool isUserInCall(const std::string& nicknameHash) = 0;

            // Получить партнера по звонку
            virtual UserPtr getCallPartner(const std::string& nicknameHash) = 0;

            // Создать активный звонок
            virtual std::shared_ptr<Call> createCall(UserPtr user1, UserPtr user2) = 0;

            // Завершить звонок
            virtual void endCall(const std::string& user1Nickname, const std::string& user2Nickname) = 0;

            // Получить звонок пользователя
            virtual std::shared_ptr<Call> getCall(const std::string& nicknameHash) = 0;

            // Управление pending calls
            virtual void addPendingCall(std::shared_ptr<PendingCall> pendingCall) = 0;
            virtual void removePendingCall(std::shared_ptr<PendingCall> pendingCall) = 0;
            virtual bool hasPendingCall(std::shared_ptr<PendingCall> pendingCall) const = 0;
        };
    }
}
