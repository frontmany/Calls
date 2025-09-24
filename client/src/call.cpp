#include "call.h"
#include "incomingCallData.h"

using namespace calls;

Call::Call(const std::string& friendNicknameHash, const CryptoPP::RSA::PublicKey& friendPublicKey)
	: m_friendNicknameHash(friendNicknameHash), m_friendPublicKey(friendPublicKey)
{
}

Call::Call(const IncomingCallData& incomingCallData) {
	m_friendNicknameHash = crypto::calculateHash(incomingCallData.friendNickname);
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

const std::string& Call::getFriendNicknameHash() const {
	return m_friendNicknameHash;
}