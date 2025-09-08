#include "call.h"
#include "incomingCallData.h"

Call::Call(const std::string& friendNickname, const CryptoPP::RSA::PublicKey& friendPublicKey)
	: m_friendNickname(friendNickname), m_friendPublicKey(friendPublicKey)
{
}

Call::Call(const IncomingCallData& incomingCallData) {
	m_friendNickname = incomingCallData.friendNickname;
	m_friendPublicKey = incomingCallData.friendPublicKey;
	m_callKey = incomingCallData.callKey;
}

void Call::createCallKey() {
	crypto::generateAESKey(m_callKey);
}

const CryptoPP::SecByteBlock& Call::getCallKey() const {
	return m_callKey;
}

const CryptoPP::RSA::PublicKey& Call::getFriendPublicKey() const {
	return m_friendPublicKey;
}

const std::string& Call::getFriendNickname() const {
	return m_friendNickname;
}