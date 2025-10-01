#pragma once 
#include<unordered_set>
#include<string>


class GroupCall {
public:
	GroupCall(const std::string& initiatorNicknameHash, const std::string& groupCallNameHash);
	~GroupCall() = default;
	const std::unordered_set<std::string>& getParticipants() const;
	const std::string& getGroupCallNameHash() const;
	const std::string& getInitiatorNicknameHash() const;
	void addParticipant(const std::string& userNicknameHash);
	void removeParticipant(const std::string& userNicknameHash);

private:
	std::unordered_set<std::string> m_participants;
	std::string m_groupCallNameHash;
	std::string m_initiatorNicknameHash;
};