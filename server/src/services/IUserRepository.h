#pragma once

#include <memory>
#include <string>
#include "user.h"
#include "asio.hpp"

namespace server
{
    namespace services
    {
        // Интерфейс для работы с репозиторием пользователей
        class IUserRepository {
        public:
            virtual ~IUserRepository() = default;

            // Найти пользователя по nickname hash
            virtual UserPtr findUserByNickname(const std::string& nicknameHash) = 0;

            // Найти пользователя по endpoint
            virtual UserPtr findUserByEndpoint(const asio::ip::udp::endpoint& endpoint) = 0;

            // Добавить пользователя
            virtual void addUser(UserPtr user) = 0;

            // Удалить пользователя по nickname hash
            virtual void removeUser(const std::string& nicknameHash) = 0;

            // Проверить существование пользователя
            virtual bool containsUser(const std::string& nicknameHash) const = 0;
            virtual bool containsUser(const asio::ip::udp::endpoint& endpoint) const = 0;

            // Обновить endpoint пользователя
            virtual void updateUserEndpoint(const std::string& nicknameHash, 
                                           const asio::ip::udp::endpoint& newEndpoint) = 0;
        };
    }
}
