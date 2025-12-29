#include "outgoingCall.h"

namespace calls
{
    OutgoingCall::OutgoingCall(OutgoingCall&& other) noexcept
        : m_nickname(std::move(other.m_nickname)),
        m_timer(std::move(other.m_timer))
    {
    }

    OutgoingCall::~OutgoingCall() {
        stop();
    }

    const std::string& OutgoingCall::getNickname() const {
        return m_nickname;
    }

    const utilities::tic::SingleShotTimer& OutgoingCall::getTimer() const {
        return m_timer;
    }

    void OutgoingCall::stop() {
        m_timer.stop();
    }
}
