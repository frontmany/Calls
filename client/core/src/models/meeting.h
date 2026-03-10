#pragma once

#include "utilities/crypto.h"
#include "models/user.h"
#include "models/meetingParticipant.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace core
{
    class Meeting {
    public:
        Meeting(std::string meetingId, CryptoPP::SecByteBlock meetingKey);

        const std::string& getMeetingId() const;
        const CryptoPP::SecByteBlock& getMeetingKey() const;
        void setMeetingKey(CryptoPP::SecByteBlock key);

        std::optional<MeetingParticipant> getOwner() const;
        const std::vector<MeetingParticipant>& getParticipants() const;
        void addParticipant(const MeetingParticipant& participant);
        void removeParticipant(const std::string& nickname);
        std::optional<MeetingParticipant> findParticipant(const std::string& nickname) const;

    private:
        std::string m_meetingId;
        CryptoPP::SecByteBlock m_meetingKey;
        std::vector<MeetingParticipant> m_participants;
    };
}