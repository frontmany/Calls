#pragma once

#include <optional>

#include "crypto.h"
#include "incomingCallData.h"

namespace calls {

class Call {
public:
	Call(const std::string& friendNicknameHash, const std::string& friendNickname, const CryptoPP::RSA::PublicKey& friendPublicKey);
	Call(const IncomingCallData& incomingCallData);
	void createCallKey();
	const CryptoPP::SecByteBlock& getCallKey() const;
	const CryptoPP::RSA::PublicKey& getFriendPublicKey() const;
	const std::string& getFriendNicknameHash() const;
	const std::string& getFriendNickname() const;

private:
	std::string m_friendNickname{};
	std::string m_friendNicknameHash{};
	CryptoPP::RSA::PublicKey m_friendPublicKey;
	CryptoPP::SecByteBlock m_callKey;
};

}