#pragma once

#include <string>
#include <memory>
#include <functional>

#include "ticTimer.h"

namespace server
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
	tic::SingleShotTimer m_timer;
    };
}