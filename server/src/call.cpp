#include "call.h"

Call::Call(const UserPtr& initiator, const UserPtr& responder)
	: m_initiator(initiator),
	m_responder(responder)
{
}

const UserPtr& Call::getInitiator() const {
	return m_initiator;
}

const UserPtr& Call::getResponder() const {
	return m_responder;
}

bool Call::isEstablished() {
	return m_established;
}

void Call::setEstablished() {
	m_established = true;
}