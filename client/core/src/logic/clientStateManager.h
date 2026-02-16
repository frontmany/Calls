#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>
#include <functional>
#include <optional>

#include "models/call.h"
#include "models/incomingCall.h"
#include "models/outgoingCall.h"
#include "media/mediaState.h"
#include "media/mediaType.h"

namespace core::logic
{
    class ClientStateManager {
    public:
        ClientStateManager();
        ~ClientStateManager() = default;

        bool isAuthorized() const;
        bool isConnectionDown() const;
        bool isIncomingCalls() const;
        bool isViewingRemoteScreen() const;
        bool isViewingRemoteCamera() const;
        bool isOutgoingCall() const;
        bool isActiveCall() const;
        bool isCallParticipantConnectionDown() const;

        void setAuthorized(bool value);
        void setConnectionDown(bool value);
        void setCallParticipantConnectionDown(bool value);
        void setMediaState(media::MediaType type, media::MediaState state);
        void setViewingRemoteScreen(bool value);
        void setViewingRemoteCamera(bool value);
        void setSharingScreen(bool value);
        void setSharingCamera(bool value);

        const std::string& getMyNickname() const;
        void setMyNickname(const std::string& nickname);
        void resetMyNickname();

        const std::string& getMyToken() const;
        void setMyToken(const std::string& token);
        void resetMyToken();

        const media::MediaState getMediaState(media::MediaType type) const;

        const OutgoingCall& getOutgoingCall() const;
        const Call& getActiveCall() const;

        template <typename Rep, typename Period>
        void setOutgoingCall(const std::string& nickname,
            const std::chrono::duration<Rep, Period>& timeout,
            std::function<void()> onTimeout,
            std::optional<CryptoPP::RSA::PublicKey> publicKey = std::nullopt,
            std::optional<CryptoPP::SecByteBlock> callKey = std::nullopt)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_outgoingCall.has_value()) {
                m_outgoingCall->stop();
            }
            m_outgoingCall.emplace(nickname, timeout, std::move(onTimeout), std::move(publicKey), std::move(callKey));
        }

        void setActiveCall(const std::string& nickname,
            const CryptoPP::RSA::PublicKey& publicKey, const CryptoPP::SecByteBlock& callKey);

        void resetActiveCall();
        void resetOutgoingCall();

        const std::unordered_map<std::string, IncomingCall>& getIncomingCalls() const;
        int getIncomingCallsCount() const;

        template <typename Rep, typename Period>
        void addIncomingCall(const std::string& nickname,
            const CryptoPP::RSA::PublicKey& publicKey,
            const CryptoPP::SecByteBlock& callKey,
            const std::chrono::duration<Rep, Period>& timeout,
            std::function<void()> onTimeout)
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            IncomingCall incomingCall(nickname, publicKey, callKey, timeout, std::move(onTimeout));
            m_incomingCalls.emplace(nickname, std::move(incomingCall));
        }
        
        void removeIncomingCall(const std::string& nickname);

        void resetIncomingCalls();

        void reset();

    private:
        mutable std::mutex m_mutex;
        std::map<media::MediaType, media::MediaState> m_mediaState;
        std::atomic_bool m_authorized;
        std::atomic_bool m_connectionDown;
        std::atomic_bool m_viewingRemoteScreen;
        std::atomic_bool m_viewingRemoteCamera;
        std::string m_myNickname;
        std::string m_myToken;

        std::optional<Call> m_activeCall;
        std::optional<OutgoingCall> m_outgoingCall;
        std::unordered_map<std::string, IncomingCall> m_incomingCalls;
    };
}