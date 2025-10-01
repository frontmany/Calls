#include "call.h"

Call::Call(const std::string& initiatorNicknameHash, const std::string& responderNicknameHash)
	: m_initiatorNicknameHash(initiatorNicknameHash),
	m_responderNicknameHash(responderNicknameHash)
{
}

const std::string& Call::getInitiatorNicknameHash() const {
	return m_initiatorNicknameHash;
}

const std::string& Call::getResponderNicknameHash() const {
	return m_responderNicknameHash;
}