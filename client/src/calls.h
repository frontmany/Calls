#pragma once

#include <string>
#include <functional>

#include "callsClient.h"

namespace calls {
    // facade functions
    inline bool init(
        const std::string& host,
        const std::string& port,
        std::unique_ptr<Handler>&& handler)
    {
        return CallsClient::get().init(
            host,
            port,
            std::move(handler));
    }

    inline void run()
    {
        CallsClient::get().run();
    }

    inline const std::string& getNickname()
    {
        return CallsClient::get().getNickname();
    }

    inline std::vector<std::string> getCallers()
    {
        return CallsClient::get().getCallers();
    }

    inline bool authorize(const std::string& nickname)
    {
        return CallsClient::get().authorize(nickname);
    }

    inline bool startCalling(const std::string& friendNickname)
    {
        return CallsClient::get().startCalling(friendNickname);
    }

    inline bool stopCalling()
    {
        return CallsClient::get().stopCalling();
    }

    inline bool acceptCall(const std::string& friendNickname)
    {
        return CallsClient::get().acceptCall(friendNickname);
    }

    inline bool declineCall(const std::string& friendNickname)
    {
        return CallsClient::get().declineCall(friendNickname);
    }

    inline bool endCall()
    {
        return CallsClient::get().endCall();
    }

    inline int getIncomingCallsCount() {
        return CallsClient::get().getIncomingCallsCount();
    }

    inline void refreshAudioDevices() {
        CallsClient::get().refreshAudioDevices();
    }

    inline void muteMicrophone(bool isMute)
    {
        CallsClient::get().muteMicrophone(isMute);
    }

    inline void muteSpeaker(bool isMute)
    {
        CallsClient::get().muteSpeaker(isMute);
    }

    inline bool isMicrophoneMuted()
    {
        return CallsClient::get().isMicrophoneMuted();
    }

    inline bool isSpeakerMuted()
    {
        return CallsClient::get().isMicrophoneMuted();
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

    inline void reset()
    {
        CallsClient::get().reset();
    }

    inline void stop()
    {
        CallsClient::get().stop();
    }

    inline bool isRunning()
    {
        return CallsClient::get().isRunning();
    }

    inline bool isAuthorized()
    {
        return CallsClient::get().isAuthorized();
    }

    inline bool isCalling()
    {
        return CallsClient::get().isCalling();
    }

    inline bool isBusy()
    {
        return CallsClient::get().isBusy();
    }

    inline const std::string& getNicknameWhomCalling() {
        return CallsClient::get().getNicknameWhomCalling();
    }

    inline const std::string& getNicknameInCallWith() {
        return CallsClient::get().getNicknameInCallWith();
    }
}