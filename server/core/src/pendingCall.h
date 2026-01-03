#pragma once

#include <string>
#include <memory>
#include <functional>

#include "utilities/timer.h"

namespace callifornia
{
    class User;

    typedef std::shared_ptr<User> UserPtr;

    class PendingCall {
public:
	PendingCall(const UserPtr& initiator, const UserPtr& receiver, std::function<void()> onTimeout);

	const UserPtr& getInitiator() const;
	const UserPtr& getReceiver() const;
	void stop();

private:
	UserPtr m_initiator;
	UserPtr m_receiver;
	utilities::tic::SingleShotTimer m_timer;
    };
}