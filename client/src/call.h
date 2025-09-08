#pragma once

#include <optional>

#include "crypto.h"

class Call {
public:
	Call(const std::string& friendNickname, const CryptoPP::RSA::PublicKey& friendPublicKey);
	void createCallKey();
	std::optional<const CryptoPP::SecByteBlock&> getCallKey() const;
	const CryptoPP::RSA::PublicKey& getFriendPublicKey() const;
	const std::string& getFriendNickname() const;

private:
	std::string m_friendNickname{};
	CryptoPP::RSA::PublicKey m_friendPublicKey;
	CryptoPP::SecByteBlock m_callKey;
};