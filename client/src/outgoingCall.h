#pragma once

#include <string>
#include "utilities/timer.h"

namespace calls
{
    class OutgoingCall
    {
    public:
        template <typename Rep, typename Period, typename Callable>
        explicit OutgoingCall(const std::string& nickname,
            const std::chrono::duration<Rep, Period>& timeout,
            Callable&& onTimeout)
            : m_nickname(nickname)
        {
            m_timer.start(timeout, std::forward<Callable>(onTimeout));
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