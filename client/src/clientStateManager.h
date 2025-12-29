#pragma once

#include <atomic>
#include <string>
#include <unordered_map>

#include "call.h"
#include "callStateManager.h"
#include "incomingCall.h"

namespace calls
{
    class ClientStateManager {
    public:
        ClientStateManager();
        ~ClientStateManager() = default;

        bool isAuthorized() const;
        bool isConnectionDown() const;
        bool isScreenSharing() const;
        bool isCameraSharing() const;
        bool isViewingRemoteScreen() const;
        bool isViewingRemoteCamera() const;

        void setAuthorized(bool value);
        void setConnectionDown(bool value);
        void setScreenSharing(bool value);
        void setCameraSharing(bool value);
        void setViewingRemoteScreen(bool value);
        void setViewingRemoteCamera(bool value);

        const std::string& getMyNickname() const;
        void setMyNickname(const std::string& nickname);
        void clearMyNickname();

        const CallStateManager& getCallStateManager() const;

        template <typename Rep, typename Period, typename Callable>
        void setOutgoingCall(const std::string& nickname,
            const std::chrono::duration<Rep, Period>& timeout,
            Callable&& onTimeout)
        {
            m_callStateManager.setOutgoingCall(nickname, timeout, onTimeout);
        }

        void setActiveCall(const std::string& nickname,
            const CryptoPP::RSA::PublicKey& publicKey);

        void setActiveCall(const IncomingCall& incomingCall);

        void clearCallState();

        const std::unordered_map<std::string, IncomingCall>& getIncomingCalls() const;

        // TODO implement participant experiencing connection problems and restored also figure out 2 min timer, should or not set up

        template <typename Rep, typename Period, typename Callable>
        void addIncomingCall(const std::string& nickname,
            const CryptoPP::RSA::PublicKey& publicKey,
            const CryptoPP::SecByteBlock& callKey,
            const std::chrono::duration<Rep, Period>& timeout,
            Callable&& onTimeout)
        {
            m_incomingCalls.emplace(nickname, nickname, publicKey, callKey, timeout, std::forward<Callable>(onTimeout));
        }
        
        void removeIncomingCall(const std::string& nickname);

        void clearIncomingCalls();

        void reset();

    private:
        CallStateManager m_callStateManager;
        std::atomic_bool m_authorized;
        std::atomic_bool m_connectionDown;
        std::atomic_bool m_screenSharing;
        std::atomic_bool m_viewingRemoteScreen;
        std::atomic_bool m_cameraSharing;
        std::atomic_bool m_viewingRemoteCamera;

        std::string m_myNickname;
        std::unordered_map<std::string, IncomingCall> m_incomingCalls;
    };
}

