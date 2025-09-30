#pragma once 
#include<vector>
#include<string>


class GropupCall {
public:
	enum class Status {
		ACTIVE,
		ENDED
	};

	GropupCall();
	~GropupCall();
	Status status() const;
	const std::vector<std::string> getParticipants() const;
	void addParticipant(const std::string& userNicknameHash);
	void removeParticipant(const std::string& userNicknameHash);

private:
	Status m_state = Status::ACTIVE;
	std::vector<std::string> m_participants;
};