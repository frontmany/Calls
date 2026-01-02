#pragma once

#include <string>
#include <memory>

class User;

typedef std::shared_ptr<User> UserPtr;

class Call {
public:
	Call(const UserPtr& initiator, const UserPtr& responder);
	bool isEstablished();
	void setEstablished();
	const UserPtr& getInitiator() const;
	const UserPtr& getResponder() const;

private:
	bool m_established = false;
	UserPtr m_initiator;
	UserPtr m_responder;
};