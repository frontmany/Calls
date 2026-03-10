#include "meeting.h"
#include <algorithm>

namespace core
{
    Meeting::Meeting(std::string meetingId, CryptoPP::SecByteBlock meetingKey)
        : m_meetingId(std::move(meetingId))
        , m_meetingKey(std::move(meetingKey))
    {
    }

    const std::string& Meeting::getMeetingId() const {
        return m_meetingId;
    }

    const CryptoPP::SecByteBlock& Meeting::getMeetingKey() const {
        return m_meetingKey;
    }

    void Meeting::setMeetingKey(CryptoPP::SecByteBlock key) {
        m_meetingKey = std::move(key);
    }

    std::optional<MeetingParticipant> Meeting::getOwner() const {
        auto it = std::find_if(m_participants.begin(), m_participants.end(),
            [](const MeetingParticipant& p) { return p.isOwner(); });
        return it != m_participants.end() ? std::optional<MeetingParticipant>(*it) : std::nullopt;
    }

    const std::vector<MeetingParticipant>& Meeting::getParticipants() const {
        return m_participants;
    }

    void Meeting::addParticipant(const MeetingParticipant& participant) {
        if (findParticipant(participant.getUser().getNickname()).has_value()) return;
        if (participant.isOwner()) {
            for (auto& p : m_participants)
                p.setOwner(false);
        }
        m_participants.push_back(std::move(participant));
    }

    void Meeting::removeParticipant(const std::string& nickname) {
        m_participants.erase(
            std::remove_if(m_participants.begin(), m_participants.end(),
                [&nickname](const MeetingParticipant& p) {
                    return p.getUser().getNickname() == nickname;
                }),
            m_participants.end());
    }

    std::optional<MeetingParticipant> Meeting::findParticipant(const std::string& nickname) const {
        auto it = std::find_if(m_participants.begin(), m_participants.end(),
            [&nickname](const MeetingParticipant& p) {
                return p.getUser().getNickname() == nickname;
            });
        return it != m_participants.end() ? std::optional<MeetingParticipant>(*it) : std::nullopt;
    }
}
