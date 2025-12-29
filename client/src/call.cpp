#include "call.h"
#include "incomingCall.h"

using namespace utilities;

namespace calls
{
	Call::Call(const std::string& nickname,
		const CryptoPP::RSA::PublicKey& publicKey)
		: m_nickname(nickname),
		m_publicKey(publicKey)
	{
		createCallKey();
	}

	Call::Call(const IncomingCall& incomingCallData) {
		m_nickname = incomingCallData.friendNickname;
		m_publicKey = incomingCallData.friendPublicKey;
		m_callKey = incomingCallData.callKey;
	}

	const CryptoPP::SecByteBlock& Call::getCallKey() const {
		return m_callKey;
	}

	const CryptoPP::RSA::PublicKey& Call::getPublicKey() const {
		return m_publicKey;
	}

	const std::string& Call::getNickname() const {
		return m_nickname;
	}

	void Call::createCallKey() {
		crypto::generateAESKey(m_callKey);
	}
}