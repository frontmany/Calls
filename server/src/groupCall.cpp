#include "groupCall.h"


GropupCall::GropupCall() {

}

GropupCall::~GropupCall() {

}

void GropupCall::addParticipant(const std::string& userNicknameHash) {

}

void GropupCall::removeParticipant(const std::string& userNicknameHash) {

}

const std::vector<std::string> GropupCall::getParticipants() const {
	return m_participants;
}

GropupCall::Status GropupCall::status() const {
	return m_state;
}