#pragma once

#include <string>
#include <system_error>

namespace core
{
    namespace services
    {
        // Интерфейс для управления звонками
        class ICallService {
        public:
            virtual ~ICallService() = default;

            virtual std::error_code startOutgoingCall(const std::string& nickname) = 0;
            virtual std::error_code stopOutgoingCall() = 0;
            virtual std::error_code acceptCall(const std::string& nickname) = 0;
            virtual std::error_code declineCall(const std::string& nickname) = 0;
            virtual std::error_code endCall() = 0;
        };
    }
}
