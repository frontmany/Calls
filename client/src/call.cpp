#include "call.h"
#include "incomingCall.h"

using namespace utilities;

namespace calls
{
	Call::Call(const std::string& nickname,
		const CryptoPP::RSA::PublicKey& publicKey,
		const CryptoPP::SecByteBlock& callKey)
		: m_nickname(nickname),
		m_publicKey(publicKey),
		m_callKey(callKey)
	{
	}

	bool Call::isParticipantConnectionDown() const {
		return m_participantConnectionDown;
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

	void Call::setParticipantConnectionDown(bool value) {
		m_participantConnectionDown = value;
	}
}