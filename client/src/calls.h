#pragma once

#include <string>
#include <functional>

#include "callsClient.h"
#include "state.h"

namespace calls {
    // facade functions
    inline void init(
        const std::string& host,
        const std::string& port,
        std::function<void(Result)> authorizationResultCallback,
        std::function<void(Result)> createCallResultCallback,
        std::function<void(const std::string& friendNickName)> onIncomingCall,
        std::function<void(const std::string&)> onIncomingCallExpired,
        std::function<void(const std::string&)> onSimultaneousCalling,
        std::function<void()> onCallHangUpCallback,
        std::function<void()> onNetworkErrorCallback)
    {
        CallsClient::get().init(
            host,
            port,
            std::move(authorizationResultCallback),
            std::move(createCallResultCallback),
            std::move(onIncomingCall),
            std::move(onIncomingCallExpired),
            std::move(onSimultaneousCalling),
            std::move(onCallHangUpCallback),
            std::move(onNetworkErrorCallback));
    }

    inline void run()
    {
        CallsClient::get().run();
    }

    inline State getState()
    {
        return CallsClient::get().getStatus();
    }

    inline const std::string& getNickname()
    {
        return CallsClient::get().getNickname();
    }

    inline bool authorize(const std::string& nickname)
    {
        return CallsClient::get().authorize(nickname);
    }

    inline bool createCall(const std::string& friendNickname)
    {
        return CallsClient::get().createCall(friendNickname);
    }

    inline bool stopCalling()
    {
        return CallsClient::get().stopCalling();
    }

    inline bool acceptCall(const std::string& friendNickname)
    {
        return CallsClient::get().acceptIncomingCall(friendNickname);
    }

    inline bool declineCall(const std::string& friendNickname)
    {
        return CallsClient::get().declineIncomingCall(friendNickname);
    }

    inline bool endCall()
    {
        return CallsClient::get().endCall();
    }

    inline void refreshAudioDevices() {
        CallsClient::get().refreshAudioDevices();
    }

    inline void mute(bool isMute)
    {
        CallsClient::get().mute(isMute);
    }

    inline bool isMuted()
    {
        return CallsClient::get().isMuted();
    }

    inline void setInputVolume(int volume)
    {
        CallsClient::get().setInputVolume(volume);
    }

    inline void setOutputVolume(int volume)
    {
        CallsClient::get().setOutputVolume(volume);
    }

    inline int getInputVolume()
    {
        return CallsClient::get().getInputVolume();
    }

    inline int getOutputVolume()
    {
        return CallsClient::get().getOutputVolume();
    }

    inline void logout()
    {
        CallsClient::get().logout();
    }

    inline void stop()
    {
        CallsClient::get().stop();
    }

    inline bool isRunning()
    {
        return CallsClient::get().isRunning();
    }

    inline const std::string& getNicknameWhomCalling() {
        return CallsClient::get().getNicknameWhomCalling();
    }

    inline const std::string& getNicknameInCallWith() {
        return CallsClient::get().getNicknameInCallWith();
    }
}