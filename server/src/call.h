#pragma once

#include <string>

class Call {
public:
	Call(const std::string& initiatorNicknameHash, const std::string& responderNicknameHash);
	const std::string& getInitiatorNicknameHash() const;
	const std::string& getResponderNicknameHash() const;

private:
	std::string m_initiatorNicknameHash;
	std::string m_responderNicknameHash;
};