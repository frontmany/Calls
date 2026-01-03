#pragma once

#include <string>
#include <memory>

namespace callifornia
{
    class User;

    typedef std::shared_ptr<User> UserPtr;

    class Call {
public:
	Call(const UserPtr& initiator, const UserPtr& responder);

	const UserPtr& getInitiator() const;
	const UserPtr& getResponder() const;

private:
	UserPtr m_initiator;
	UserPtr m_responder;
    };
}