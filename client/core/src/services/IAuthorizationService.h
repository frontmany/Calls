#pragma once

#include <string>
#include <system_error>

namespace core
{
    namespace services
    {
        // Интерфейс для управления авторизацией и переподключением
        class IAuthorizationService {
        public:
            virtual ~IAuthorizationService() = default;

            virtual std::error_code authorize(const std::string& nickname) = 0;
            virtual std::error_code logout() = 0;
            virtual void handleReconnect() = 0;
        };
    }
}
