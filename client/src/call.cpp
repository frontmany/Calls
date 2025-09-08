#include "call.h"

Call::Call(const std::string& friendNickname, const CryptoPP::RSA::PublicKey& friendPublicKey)
	: m_friendNickname(friendNickname), m_friendPublicKey(friendPublicKey)
{
}

void Call::createCallKey() {
	crypto::generateAESKey(m_callKey);
}

std::optional<const CryptoPP::SecByteBlock&> Call::getCallKey() const {
	if (!m_callKey.empty()) {
		return m_callKey;
	}

	return std::nullopt;
}

const CryptoPP::RSA::PublicKey& Call::getFriendPublicKey() const {
	return m_friendPublicKey;
}

const std::string& Call::getFriendNickname() const {
	return m_friendNickname;
}