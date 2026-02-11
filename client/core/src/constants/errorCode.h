#pragma once
#include <system_error>
#include <string>

namespace core::constant
{
    enum class ErrorCode {
        success = 0,
        network_error,
        taken_nickname,
        unexisting_user,
        connection_down_with_user,
        user_logout,
        connection_down,
        not_authorized,
        already_authorized,
        operation_in_progress,
        active_call_exists,
        no_incoming_call,
        no_outgoing_call,
        no_active_call,
        screen_sharing_already_active,
        camera_sharing_already_active,
        screen_sharing_not_active,
        camera_sharing_not_active,
        viewing_remote_screen,
        encryption_error
    };

    class ErrorCategory : public std::error_category {
    public:
        const char* name() const noexcept override {
            return "calliforniaErrorCategory";
        }

        std::string message(int ev) const override {
            switch (static_cast<ErrorCode>(ev)) {
            case ErrorCode::success:
                return "Success";
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
            case ErrorCode::connection_down:
                return "Connection down";
            case ErrorCode::not_authorized:
                return "Not authorized";
            case ErrorCode::already_authorized:
                return "Already authorized";
            case ErrorCode::operation_in_progress:
                return "Operation in progress";
            case ErrorCode::active_call_exists:
                return "Active call exists";
            case ErrorCode::no_incoming_call:
                return "No incoming call";
            case ErrorCode::no_outgoing_call:
                return "No outgoing call";
            case ErrorCode::no_active_call:
                return "No active call";
            case ErrorCode::screen_sharing_already_active:
                return "Screen sharing already active";
            case ErrorCode::camera_sharing_already_active:
                return "Camera sharing already active";
            case ErrorCode::screen_sharing_not_active:
                return "Screen sharing not active";
            case ErrorCode::camera_sharing_not_active:
                return "Camera sharing not active";
            case ErrorCode::viewing_remote_screen:
                return "Viewing remote screen";
            case ErrorCode::encryption_error:
                return "Encryption error";
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
    struct is_error_code_enum<core::constant::ErrorCode> : true_type {};
}