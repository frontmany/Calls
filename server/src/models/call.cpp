#include "call.h"

namespace server
{
Call::Call(const UserPtr& initiator, const UserPtr& responder)
	: m_initiator(initiator),
	m_responder(responder)
{
}

const UserPtr& Call::getInitiator() const
{
	return m_initiator;
}

const UserPtr& Call::getResponder() const
{
	return m_responder;
}
} // namespace server