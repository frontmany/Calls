#pragma once

#include <string>
#include <functional>
#include "utilities/timer.h"

namespace callifornia
{
    class OutgoingCall
    {
    public:
        template <typename Rep, typename Period>
        explicit OutgoingCall(const std::string& nickname,
            const std::chrono::duration<Rep, Period>& timeout,
            std::function<void()> onTimeout)
            : m_nickname(nickname)
        {
            m_timer.start(timeout, std::move(onTimeout));
        }
        OutgoingCall(const OutgoingCall& other) = delete;
        OutgoingCall(OutgoingCall&& other) noexcept;
        ~OutgoingCall();

        const std::string& getNickname() const;
        const utilities::tic::SingleShotTimer& getTimer() const;
        void stop();

    private:
        std::string m_nickname;
        utilities::tic::SingleShotTimer m_timer;
    };
}