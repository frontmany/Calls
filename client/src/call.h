#pragma once

#include <optional>

#include "utilities/crypto.h"
#include "incomingCall.h"

namespace calls
{
	class Call {
	public:
		Call(const std::string& nickname,
			const CryptoPP::RSA::PublicKey& publicKey,
			const CryptoPP::SecByteBlock& callKey
		);

		bool isParticipantConnectionDown() const;
		const CryptoPP::SecByteBlock& getCallKey() const;
		const CryptoPP::RSA::PublicKey& getPublicKey() const;
		const std::string& getNickname() const;
		void setParticipantConnectionDown(bool value);

	private:
		bool m_participantConnectionDown = false;
		std::string m_nickname;
		CryptoPP::RSA::PublicKey m_publicKey;
		CryptoPP::SecByteBlock m_callKey;
	};
}