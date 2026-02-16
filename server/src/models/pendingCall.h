#pragma once

#include <string>
#include <memory>
#include <functional>
#include <vector>

#include "ticTimer.h"

namespace server
{
    class User;

    typedef std::shared_ptr<User> UserPtr;

    class PendingCall {
public:
	PendingCall(const UserPtr& initiator, const UserPtr& receiver, std::function<void()> onTimeout, std::vector<unsigned char> callingBeginBody);

	const UserPtr& getInitiator() const;
	const UserPtr& getReceiver() const;
	const std::vector<unsigned char>& getCallingBeginBody() const;
	void stop();

private:
	UserPtr m_initiator;
	UserPtr m_receiver;
	std::vector<unsigned char> m_callingBeginBody;
	tic::SingleShotTimer m_timer;
    };
}