#pragma once

#include <string>

class Call {
public:
	enum class Status {
		ACTIVE,
		ENDED
	};

	Call(const std::string& initiatorNicknameHash, const std::string& responderNicknameHash);
	Status status() const;
	const std::string& getInitiatorNicknameHash() const;
	const std::string& getResponderNicknameHash() const;
	void setActive();
	void setEnded();

private:
	Status m_state = Status::ACTIVE;
	std::string m_initiatorNicknameHash;
	std::string m_responderNicknameHash;
};