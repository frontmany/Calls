#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>
#include <system_error>

#include "client.h"
#include "eventListener.h"
#include "errorCode.h"
#include "network/networkController.h"

namespace py = pybind11;

class PyEventListener : public callifornia::EventListener, public py::trampoline_self_life_support
{
public:
    using callifornia::EventListener::EventListener;

    void onAuthorizationResult(std::error_code ec) override
    {
        PYBIND11_OVERRIDE_PURE(void, callifornia::EventListener, onAuthorizationResult, ec);
    }

    void onLogoutCompleted() override
    {
        PYBIND11_OVERRIDE_PURE(void, callifornia::EventListener, onLogoutCompleted);
    }

    void onStartOutgoingCallResult(std::error_code ec) override
    {
        PYBIND11_OVERRIDE_PURE(void, callifornia::EventListener, onStartOutgoingCallResult, ec);
    }

    void onStopOutgoingCallResult(std::error_code ec) override
    {
        PYBIND11_OVERRIDE_PURE(void, callifornia::EventListener, onStopOutgoingCallResult, ec);
    }

    void onAcceptCallResult(std::error_code ec) override
    {
        PYBIND11_OVERRIDE_PURE(void, callifornia::EventListener, onAcceptCallResult, ec);
    }

    void onDeclineCallResult(std::error_code ec) override
    {
        PYBIND11_OVERRIDE_PURE(void, callifornia::EventListener, onDeclineCallResult, ec);
    }

    void onEndCallResult(std::error_code ec) override
    {
        PYBIND11_OVERRIDE_PURE(void, callifornia::EventListener, onEndCallResult, ec);
    }

    void onStartScreenSharingResult(std::error_code ec) override
    {
        PYBIND11_OVERRIDE_PURE(void, callifornia::EventListener, onStartScreenSharingResult, ec);
    }

    void onStopScreenSharingResult(std::error_code ec) override
    {
        PYBIND11_OVERRIDE_PURE(void, callifornia::EventListener, onStopScreenSharingResult, ec);
    }

    void onStartCameraSharingResult(std::error_code ec) override
    {
        PYBIND11_OVERRIDE_PURE(void, callifornia::EventListener, onStartCameraSharingResult, ec);
    }

    void onStopCameraSharingResult(std::error_code ec) override
    {
        PYBIND11_OVERRIDE_PURE(void, callifornia::EventListener, onStopCameraSharingResult, ec);
    }

    void onIncomingScreenSharingStarted() override
    {
        PYBIND11_OVERRIDE_PURE(void, callifornia::EventListener, onIncomingScreenSharingStarted);
    }

    void onIncomingScreenSharingStopped() override
    {
        PYBIND11_OVERRIDE_PURE(void, callifornia::EventListener, onIncomingScreenSharingStopped);
    }

    void onIncomingScreen(const std::vector<unsigned char>& data) override
    {
        PYBIND11_OVERRIDE_PURE(void, callifornia::EventListener, onIncomingScreen, data);
    }

    void onIncomingCameraSharingStarted() override
    {
        PYBIND11_OVERRIDE_PURE(void, callifornia::EventListener, onIncomingCameraSharingStarted);
    }

    void onIncomingCameraSharingStopped() override
    {
        PYBIND11_OVERRIDE_PURE(void, callifornia::EventListener, onIncomingCameraSharingStopped);
    }

    void onIncomingCamera(const std::vector<unsigned char>& data) override
    {
        PYBIND11_OVERRIDE_PURE(void, callifornia::EventListener, onIncomingCamera, data);
    }

    void onOutgoingCallAccepted() override
    {
        PYBIND11_OVERRIDE_PURE(void, callifornia::EventListener, onOutgoingCallAccepted);
    }

    void onOutgoingCallDeclined() override
    {
        PYBIND11_OVERRIDE_PURE(void, callifornia::EventListener, onOutgoingCallDeclined);
    }

    void onOutgoingCallTimeout(std::error_code ec) override
    {
        PYBIND11_OVERRIDE_PURE(void, callifornia::EventListener, onOutgoingCallTimeout, ec);
    }

    void onIncomingCall(const std::string& nickname) override
    {
        PYBIND11_OVERRIDE_PURE(void, callifornia::EventListener, onIncomingCall, nickname);
    }

    void onIncomingCallExpired(std::error_code ec, const std::string& nickname) override
    {
        PYBIND11_OVERRIDE_PURE(void, callifornia::EventListener, onIncomingCallExpired, ec, nickname);
    }

    void onCallEndedByRemote(std::error_code ec) override
    {
        PYBIND11_OVERRIDE_PURE(void, callifornia::EventListener, onCallEndedByRemote, ec);
    }

    void onCallParticipantConnectionDown() override
    {
        PYBIND11_OVERRIDE_PURE(void, callifornia::EventListener, onCallParticipantConnectionDown);
    }

    void onCallParticipantConnectionRestored() override
    {
        PYBIND11_OVERRIDE_PURE(void, callifornia::EventListener, onCallParticipantConnectionRestored);
    }

    void onConnectionDown() override
    {
        PYBIND11_OVERRIDE_PURE(void, callifornia::EventListener, onConnectionDown);
    }

    void onConnectionRestored() override
    {
        PYBIND11_OVERRIDE_PURE(void, callifornia::EventListener, onConnectionRestored);
    }

    void onConnectionRestoredAuthorizationNeeded() override
    {
        PYBIND11_OVERRIDE_PURE(void, callifornia::EventListener, onConnectionRestoredAuthorizationNeeded);
    }
};

PYBIND11_MODULE(callsClientPy, m) {
    m.doc() = "Calls Client Python Binding";

    // Error code enum
    py::enum_<callifornia::ErrorCode>(m, "ErrorCode")
        .value("NETWORK_ERROR", callifornia::ErrorCode::network_error)
        .value("TAKEN_NICKNAME", callifornia::ErrorCode::taken_nickname)
        .value("UNEXISTING_USER", callifornia::ErrorCode::unexisting_user)
        .value("CONNECTION_DOWN_WITH_USER", callifornia::ErrorCode::connection_down_with_user)
        .value("USER_LOGOUT", callifornia::ErrorCode::user_logout)
        .export_values();

    // EventListener interface
    py::class_<callifornia::EventListener, PyEventListener, py::smart_holder>(m, "EventListener")
        .def(py::init<>())
        .def("onAuthorizationResult", &callifornia::EventListener::onAuthorizationResult)
        .def("onLogoutCompleted", &callifornia::EventListener::onLogoutCompleted)
        .def("onStartOutgoingCallResult", &callifornia::EventListener::onStartOutgoingCallResult)
        .def("onStopOutgoingCallResult", &callifornia::EventListener::onStopOutgoingCallResult)
        .def("onAcceptCallResult", &callifornia::EventListener::onAcceptCallResult)
        .def("onDeclineCallResult", &callifornia::EventListener::onDeclineCallResult)
        .def("onEndCallResult", &callifornia::EventListener::onEndCallResult)
        .def("onStartScreenSharingResult", &callifornia::EventListener::onStartScreenSharingResult)
        .def("onStopScreenSharingResult", &callifornia::EventListener::onStopScreenSharingResult)
        .def("onStartCameraSharingResult", &callifornia::EventListener::onStartCameraSharingResult)
        .def("onStopCameraSharingResult", &callifornia::EventListener::onStopCameraSharingResult)
        .def("onIncomingScreenSharingStarted", &callifornia::EventListener::onIncomingScreenSharingStarted)
        .def("onIncomingScreenSharingStopped", &callifornia::EventListener::onIncomingScreenSharingStopped)
        .def("onIncomingScreen", &callifornia::EventListener::onIncomingScreen)
        .def("onIncomingCameraSharingStarted", &callifornia::EventListener::onIncomingCameraSharingStarted)
        .def("onIncomingCameraSharingStopped", &callifornia::EventListener::onIncomingCameraSharingStopped)
        .def("onIncomingCamera", &callifornia::EventListener::onIncomingCamera)
        .def("onOutgoingCallAccepted", &callifornia::EventListener::onOutgoingCallAccepted)
        .def("onOutgoingCallDeclined", &callifornia::EventListener::onOutgoingCallDeclined)
        .def("onOutgoingCallTimeout", &callifornia::EventListener::onOutgoingCallTimeout)
        .def("onIncomingCall", &callifornia::EventListener::onIncomingCall)
        .def("onIncomingCallExpired", &callifornia::EventListener::onIncomingCallExpired)
        .def("onCallEndedByRemote", &callifornia::EventListener::onCallEndedByRemote)
        .def("onCallParticipantConnectionDown", &callifornia::EventListener::onCallParticipantConnectionDown)
        .def("onCallParticipantConnectionRestored", &callifornia::EventListener::onCallParticipantConnectionRestored)
        .def("onConnectionDown", &callifornia::EventListener::onConnectionDown)
        .def("onConnectionRestored", &callifornia::EventListener::onConnectionRestored)
        .def("onConnectionRestoredAuthorizationNeeded", &callifornia::EventListener::onConnectionRestoredAuthorizationNeeded);

    // Client class
    py::class_<callifornia::Client>(m, "Client")
        .def(py::init<>())
        .def("init", &callifornia::Client::init,
            "Initialize calls client",
            py::arg("host"), py::arg("port"), py::arg("event_listener"))
        .def("refresh_audio_devices", &callifornia::Client::refreshAudioDevices,
            "Refresh audio devices")
        .def("mute_microphone", &callifornia::Client::muteMicrophone,
            "Mute microphone", py::arg("is_mute"))
        .def("mute_speaker", &callifornia::Client::muteSpeaker,
            "Mute speaker", py::arg("is_mute"))
        .def("set_input_volume", &callifornia::Client::setInputVolume,
            "Set input volume", py::arg("volume"))
        .def("set_output_volume", &callifornia::Client::setOutputVolume,
            "Set output volume", py::arg("volume"))
        .def("is_screen_sharing", &callifornia::Client::isScreenSharing,
            "Check if screen sharing")
        .def("is_viewing_remote_screen", &callifornia::Client::isViewingRemoteScreen,
            "Check if viewing remote screen")
        .def("is_camera_sharing", &callifornia::Client::isCameraSharing,
            "Check if camera sharing")
        .def("is_viewing_remote_camera", &callifornia::Client::isViewingRemoteCamera,
            "Check if viewing remote camera")
        .def("is_microphone_muted", &callifornia::Client::isMicrophoneMuted,
            "Check if microphone is muted")
        .def("is_speaker_muted", &callifornia::Client::isSpeakerMuted,
            "Check if speaker is muted")
        .def("is_authorized", &callifornia::Client::isAuthorized,
            "Check if authorized")
        .def("is_outgoing_call", &callifornia::Client::isOutgoingCall,
            "Check if outgoing call")
        .def("is_active_call", &callifornia::Client::isActiveCall,
            "Check if active call")
        .def("is_connection_down", &callifornia::Client::isConnectionDown,
            "Check if connection down")
        .def("get_input_volume", &callifornia::Client::getInputVolume,
            "Get input volume")
        .def("get_output_volume", &callifornia::Client::getOutputVolume,
            "Get output volume")
        .def("get_incoming_calls_count", &callifornia::Client::getIncomingCallsCount,
            "Get incoming calls count")
        .def("get_callers", &callifornia::Client::getCallers,
            "Get list of callers")
        .def("get_my_nickname", &callifornia::Client::getMyNickname,
            "Get my nickname")
        .def("get_nickname_whom_calling", &callifornia::Client::getNicknameWhomCalling,
            "Get nickname of person being called")
        .def("get_nickname_in_call_with", &callifornia::Client::getNicknameInCallWith,
            "Get nickname of person in call with")
        .def("authorize", &callifornia::Client::authorize,
            "Authorize with nickname", py::arg("nickname"))
        .def("logout", &callifornia::Client::logout,
            "Logout")
        .def("start_outgoing_call", &callifornia::Client::startOutgoingCall,
            "Start outgoing call", py::arg("friend_nickname"))
        .def("stop_outgoing_call", &callifornia::Client::stopOutgoingCall,
            "Stop outgoing call")
        .def("accept_call", &callifornia::Client::acceptCall,
            "Accept incoming call", py::arg("friend_nickname"))
        .def("decline_call", &callifornia::Client::declineCall,
            "Decline incoming call", py::arg("friend_nickname"))
        .def("end_call", &callifornia::Client::endCall,
            "End current call")
        .def("start_screen_sharing", &callifornia::Client::startScreenSharing,
            "Start screen sharing")
        .def("stop_screen_sharing", &callifornia::Client::stopScreenSharing,
            "Stop screen sharing")
        .def("send_screen", &callifornia::Client::sendScreen,
            "Send screen data", py::arg("data"))
        .def("start_camera_sharing", &callifornia::Client::startCameraSharing,
            "Start camera sharing")
        .def("stop_camera_sharing", &callifornia::Client::stopCameraSharing,
            "Stop camera sharing")
        .def("send_camera", &callifornia::Client::sendCamera,
            "Send camera data", py::arg("data"));
}
