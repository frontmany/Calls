#pragma once

#include <string>
#include <vector>
#include <functional>

#include "client.h"
#include "jsonTypes.h"

namespace calls {
    inline bool init(
        const std::string& host,
        const std::string& port,
        std::shared_ptr<EventListener> eventListener)
    {
        return CallsClient::get().init(
            host,
            port,
            eventListener);
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

    inline bool startScreenSharing()
    {
        return CallsClient::get().startScreenSharing();
    }

    inline bool stopScreenSharing()
    {
        return CallsClient::get().stopScreenSharing();
    }

    inline bool sendScreen(const std::vector<unsigned char>& data)
    {
        return CallsClient::get().sendScreen(data);
    }

    inline bool startCameraSharing()
    {
        return CallsClient::get().startCameraSharing();
    }

    inline bool stopCameraSharing()
    {
        return CallsClient::get().stopCameraSharing();
    }

    inline bool sendCamera(const std::vector<unsigned char>& data)
    {
        return CallsClient::get().sendCamera(data);
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
        return CallsClient::get().isSpeakerMuted();
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

    inline bool isScreenSharing()
    {
        return CallsClient::get().isScreenSharing();
    }

    inline bool isViewingRemoteScreen()
    {
        return CallsClient::get().isViewingRemoteScreen();
    }

    inline bool isCameraSharing()
    {
        return CallsClient::get().isCameraSharing();
    }

    inline bool isViewingRemoteCamera()
    {
        return CallsClient::get().isViewingRemoteCamera();
    }

    inline bool isAuthorized()
    {
        return CallsClient::get().isAuthorized();
    }

    inline bool isNetworkError()
    {
        return CallsClient::get().isNetworkError();
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
