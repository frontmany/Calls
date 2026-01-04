#include "pendingCall.h"
#include "user.h"
#include <chrono>

using namespace std::chrono_literals;

namespace server
{
PendingCall::PendingCall(const UserPtr& initiator, const UserPtr& receiver, std::function<void()> onTimeout)
	: m_initiator(initiator),
	m_receiver(receiver)
{
	m_timer.start(32s, std::move(onTimeout));
}

const UserPtr& PendingCall::getInitiator() const
{
	return m_initiator;
}

const UserPtr& PendingCall::getReceiver() const
{
	return m_receiver;
}

void PendingCall::stop()
{
	m_timer.stop();
}
} // namespace server
