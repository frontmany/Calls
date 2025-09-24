#pragma once

#include <string>
#include <functional>

#include "callsClient.h"

namespace calls {

    // facade functions
    inline void init(
        const std::string& host,
        std::function<void(Result)> authorizationResultCallback,
        std::function<void(Result)> createCallResultCallback,
        std::function<void(const std::string& friendNickName)> onIncomingCall,
        std::function<void(const std::string&)> onIncomingCallExpired,
        std::function<void()> onCallHangUpCallback,
        std::function<void()> onNetworkErrorCallback)
    {
        CallsClient::get().init(host,
            std::move(authorizationResultCallback),
            std::move(createCallResultCallback),
            std::move(onIncomingCall),
            std::move(onIncomingCallExpired),
            std::move(onCallHangUpCallback),
            std::move(onNetworkErrorCallback));
    }

    inline ClientStatus getStatus()
    {
        return CallsClient::get().getStatus();
    }

    inline const std::string& getNickname()
    {
        return CallsClient::get().getNickname();
    }

    inline void authorize(const std::string& nickname)
    {
        CallsClient::get().authorize(nickname);
    }

    inline void createCall(const std::string& friendNickname)
    {
        CallsClient::get().createCall(friendNickname);
    }

    inline void acceptCall(const std::string& friendNickname)
    {
        CallsClient::get().acceptIncomingCall(friendNickname);
    }

    inline void declineCall(const std::string& friendNickname)
    {
        CallsClient::get().declineIncomingCall(friendNickname);
    }

    inline void refreshAudioDevices() {
        CallsClient::get().refreshAudioDevices();
    }

    inline void mute(bool isMute)
    {
        CallsClient::get().mute(isMute);
    }

    inline void endCall()
    {
        CallsClient::get().endCall();
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
}