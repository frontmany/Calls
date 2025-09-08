#pragma once

#include <optional>

#include "crypto.h"

struct IncomingCallData;

class Call {
public:
	Call(const std::string& friendNickname, const CryptoPP::RSA::PublicKey& friendPublicKey);
	Call(const IncomingCallData& incomingCallData);
	void createCallKey();
	const CryptoPP::SecByteBlock& getCallKey() const;
	const CryptoPP::RSA::PublicKey& getFriendPublicKey() const;
	const std::string& getFriendNickname() const;

private:
	std::string m_friendNickname{};
	CryptoPP::RSA::PublicKey m_friendPublicKey;
	CryptoPP::SecByteBlock m_callKey;
};