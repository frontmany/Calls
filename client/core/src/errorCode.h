#pragma once
#include <system_error>
#include <string>

namespace core
{
    enum class ErrorCode {
        network_error,
        taken_nickname,
        unexisting_user,
        connection_down_with_user,
        user_logout,
    };

    class ErrorCategory : public std::error_category {
    public:
        const char* name() const noexcept override {
            return "calliforniaErrorCategory";
        }

        std::string message(int ev) const override {
            switch (static_cast<ErrorCode>(ev)) {
            case ErrorCode::network_error:
                return "Network error";
            case ErrorCode::taken_nickname:
                return "Taken nickname";
            case ErrorCode::unexisting_user:
                return "Unexisting user";
            case ErrorCode::connection_down_with_user:
                return "Connection down with user";
            case ErrorCode::user_logout:
                return "User logout";
            default:
                return "Unknown error";
            }
        }
    };

    inline const ErrorCategory& error_category() {
        static ErrorCategory instance;
        return instance;
    }

    inline std::error_code make_error_code(ErrorCode ec) {
        return { static_cast<int>(ec), error_category() };
    }
}

namespace std
{
    template<>
    struct is_error_code_enum<core::ErrorCode> : true_type {};
}