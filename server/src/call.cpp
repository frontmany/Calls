#include "call.h"

Call::Call(const std::string& initiatorNicknameHash, const std::string& responderNicknameHash)
	: m_initiatorNicknameHash(initiatorNicknameHash),
	m_responderNicknameHash(responderNicknameHash)
{
}

void Call::setActive() {
	m_state = Status::ACTIVE;
}

void Call::setEnded() {
	m_state = Status::ENDED;
}

Call::Status Call::status() const{
	return m_state;
}

const std::string& Call::getInitiatorNicknameHash() const {
	return m_initiatorNicknameHash;
}

const std::string& Call::getResponderNicknameHash() const {
	return m_responderNicknameHash;
}