#pragma once
#include <system_error>
#include <string>

namespace calls
{
    enum class CallsErrorCode {
        network_error,
        taken_nickname,
        unexisting_user,
    };

    class CallsErrorCategory : public std::error_category {
    public:
        const char* name() const noexcept override {
            return "ñallsErrorCategory";
        }

        std::string message(int ev) const override {
            switch (static_cast<CallsErrorCode>(ev)) {
            case CallsErrorCode::network_error:
                return "Network error";
            case CallsErrorCode::taken_nickname:
                return "Taken nickname";
            case CallsErrorCode::unexisting_user:
                return "Unexisting user";
            default:
                return "Unknown error";
            }
        }
    };

    inline const CallsErrorCategory& calls_error_category() {
        static CallsErrorCategory instance;
        return instance;
    }

    inline std::error_code make_error_code(CallsErrorCode ec) {
        return { static_cast<int>(ec), calls_error_category() };
    }
}

namespace std
{
    template<>
    struct is_error_code_enum<calls::CallsErrorCode> : true_type {};
}