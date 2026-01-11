#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>
#include <system_error>

#include "core.h"
#include "eventListener.h"
#include "errorCode.h"
#include "network/networkController.h"

namespace py = pybind11;

class PyEventListener : public core::EventListener, public py::trampoline_self_life_support
{
public:
    using core::EventListener::EventListener;

    void onAuthorizationResult(std::error_code ec) override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onAuthorizationResult, ec);
    }

    void onLogoutCompleted() override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onLogoutCompleted);
    }

    void onStartOutgoingCallResult(std::error_code ec) override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onStartOutgoingCallResult, ec);
    }

    void onStopOutgoingCallResult(std::error_code ec) override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onStopOutgoingCallResult, ec);
    }

    void onAcceptCallResult(std::error_code ec, const std::string& nickname) override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onAcceptCallResult, ec, nickname);
    }

    void onDeclineCallResult(std::error_code ec, const std::string& nickname) override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onDeclineCallResult, ec, nickname);
    }

    void onEndCallResult(std::error_code ec) override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onEndCallResult, ec);
    }

    void onStartScreenSharingResult(std::error_code ec) override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onStartScreenSharingResult, ec);
    }

    void onStopScreenSharingResult(std::error_code ec) override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onStopScreenSharingResult, ec);
    }

    void onStartCameraSharingResult(std::error_code ec) override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onStartCameraSharingResult, ec);
    }

    void onStopCameraSharingResult(std::error_code ec) override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onStopCameraSharingResult, ec);
    }

    void onIncomingScreenSharingStarted() override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onIncomingScreenSharingStarted);
    }

    void onIncomingScreenSharingStopped() override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onIncomingScreenSharingStopped);
    }

    void onIncomingScreen(const std::vector<unsigned char>& data) override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onIncomingScreen, data);
    }

    void onIncomingCameraSharingStarted() override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onIncomingCameraSharingStarted);
    }

    void onIncomingCameraSharingStopped() override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onIncomingCameraSharingStopped);
    }

    void onIncomingCamera(const std::vector<unsigned char>& data) override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onIncomingCamera, data);
    }

    void onOutgoingCallAccepted() override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onOutgoingCallAccepted);
    }

    void onOutgoingCallDeclined() override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onOutgoingCallDeclined);
    }

    void onOutgoingCallTimeout(std::error_code ec) override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onOutgoingCallTimeout, ec);
    }

    void onIncomingCall(const std::string& nickname) override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onIncomingCall, nickname);
    }

    void onIncomingCallExpired(std::error_code ec, const std::string& nickname) override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onIncomingCallExpired, ec, nickname);
    }

    void onCallEndedByRemote(std::error_code ec) override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onCallEndedByRemote, ec);
    }

    void onCallParticipantConnectionDown() override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onCallParticipantConnectionDown);
    }

    void onCallParticipantConnectionRestored() override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onCallParticipantConnectionRestored);
    }

    void onConnectionDown() override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onConnectionDown);
    }

    void onConnectionRestored() override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onConnectionRestored);
    }

    void onConnectionRestoredAuthorizationNeeded() override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onConnectionRestoredAuthorizationNeeded);
    }
};

PYBIND11_MODULE(callsClientPy, m) {
    m.doc() = "Calls Client Python Binding";

    // Error code enum
    py::enum_<core::ErrorCode>(m, "ErrorCode")
        .value("NETWORK_ERROR", core::ErrorCode::network_error)
        .value("TAKEN_NICKNAME", core::ErrorCode::taken_nickname)
        .value("UNEXISTING_USER", core::ErrorCode::unexisting_user)
        .value("CONNECTION_DOWN_WITH_USER", core::ErrorCode::connection_down_with_user)
        .value("USER_LOGOUT", core::ErrorCode::user_logout)
        .export_values();

    // EventListener interface
    py::class_<core::EventListener, PyEventListener, py::smart_holder>(m, "EventListener")
        .def(py::init<>())
        .def("onAuthorizationResult", &core::EventListener::onAuthorizationResult)
        .def("onLogoutCompleted", &core::EventListener::onLogoutCompleted)
        .def("onStartOutgoingCallResult", &core::EventListener::onStartOutgoingCallResult)
        .def("onStopOutgoingCallResult", &core::EventListener::onStopOutgoingCallResult)
        .def("onAcceptCallResult", &core::EventListener::onAcceptCallResult)
        .def("onDeclineCallResult", &core::EventListener::onDeclineCallResult)
        .def("onEndCallResult", &core::EventListener::onEndCallResult)
        .def("onStartScreenSharingResult", &core::EventListener::onStartScreenSharingResult)
        .def("onStopScreenSharingResult", &core::EventListener::onStopScreenSharingResult)
        .def("onStartCameraSharingResult", &core::EventListener::onStartCameraSharingResult)
        .def("onStopCameraSharingResult", &core::EventListener::onStopCameraSharingResult)
        .def("onIncomingScreenSharingStarted", &core::EventListener::onIncomingScreenSharingStarted)
        .def("onIncomingScreenSharingStopped", &core::EventListener::onIncomingScreenSharingStopped)
        .def("onIncomingScreen", &core::EventListener::onIncomingScreen)
        .def("onIncomingCameraSharingStarted", &core::EventListener::onIncomingCameraSharingStarted)
        .def("onIncomingCameraSharingStopped", &core::EventListener::onIncomingCameraSharingStopped)
        .def("onIncomingCamera", &core::EventListener::onIncomingCamera)
        .def("onOutgoingCallAccepted", &core::EventListener::onOutgoingCallAccepted)
        .def("onOutgoingCallDeclined", &core::EventListener::onOutgoingCallDeclined)
        .def("onOutgoingCallTimeout", &core::EventListener::onOutgoingCallTimeout)
        .def("onIncomingCall", &core::EventListener::onIncomingCall)
        .def("onIncomingCallExpired", &core::EventListener::onIncomingCallExpired)
        .def("onCallEndedByRemote", &core::EventListener::onCallEndedByRemote)
        .def("onCallParticipantConnectionDown", &core::EventListener::onCallParticipantConnectionDown)
        .def("onCallParticipantConnectionRestored", &core::EventListener::onCallParticipantConnectionRestored)
        .def("onConnectionDown", &core::EventListener::onConnectionDown)
        .def("onConnectionRestored", &core::EventListener::onConnectionRestored)
        .def("onConnectionRestoredAuthorizationNeeded", &core::EventListener::onConnectionRestoredAuthorizationNeeded);

    // Client class
    py::class_<core::Client>(m, "Client")
        .def(py::init<>())
        .def("init", &core::Client::start,
            "Initialize calls client",
            py::arg("host"), py::arg("port"), py::arg("event_listener"))
        .def("refresh_audio_devices", &core::Client::refreshAudioDevices,
            "Refresh audio devices")
        .def("mute_microphone", &core::Client::muteMicrophone,
            "Mute microphone", py::arg("is_mute"))
        .def("mute_speaker", &core::Client::muteSpeaker,
            "Mute speaker", py::arg("is_mute"))
        .def("set_input_volume", &core::Client::setInputVolume,
            "Set input volume", py::arg("volume"))
        .def("set_output_volume", &core::Client::setOutputVolume,
            "Set output volume", py::arg("volume"))
        .def("is_screen_sharing", &core::Client::isScreenSharing,
            "Check if screen sharing")
        .def("is_viewing_remote_screen", &core::Client::isViewingRemoteScreen,
            "Check if viewing remote screen")
        .def("is_camera_sharing", &core::Client::isCameraSharing,
            "Check if camera sharing")
        .def("is_viewing_remote_camera", &core::Client::isViewingRemoteCamera,
            "Check if viewing remote camera")
        .def("is_microphone_muted", &core::Client::isMicrophoneMuted,
            "Check if microphone is muted")
        .def("is_speaker_muted", &core::Client::isSpeakerMuted,
            "Check if speaker is muted")
        .def("is_authorized", &core::Client::isAuthorized,
            "Check if authorized")
        .def("is_outgoing_call", &core::Client::isOutgoingCall,
            "Check if outgoing call")
        .def("is_active_call", &core::Client::isActiveCall,
            "Check if active call")
        .def("is_connection_down", &core::Client::isConnectionDown,
            "Check if connection down")
        .def("get_input_volume", &core::Client::getInputVolume,
            "Get input volume")
        .def("get_output_volume", &core::Client::getOutputVolume,
            "Get output volume")
        .def("get_incoming_calls_count", &core::Client::getIncomingCallsCount,
            "Get incoming calls count")
        .def("get_callers", &core::Client::getCallers,
            "Get list of callers")
        .def("get_my_nickname", &core::Client::getMyNickname,
            "Get my nickname")
        .def("get_nickname_whom_calling", &core::Client::getNicknameWhomCalling,
            "Get nickname of person being called")
        .def("get_nickname_in_call_with", &core::Client::getNicknameInCallWith,
            "Get nickname of person in call with")
        .def("authorize", &core::Client::authorize,
            "Authorize with nickname", py::arg("nickname"))
        .def("logout", &core::Client::logout,
            "Logout")
        .def("start_outgoing_call", &core::Client::startOutgoingCall,
            "Start outgoing call", py::arg("friend_nickname"))
        .def("stop_outgoing_call", &core::Client::stopOutgoingCall,
            "Stop outgoing call")
        .def("accept_call", &core::Client::acceptCall,
            "Accept incoming call", py::arg("friend_nickname"))
        .def("decline_call", &core::Client::declineCall,
            "Decline incoming call", py::arg("friend_nickname"))
        .def("end_call", &core::Client::endCall,
            "End current call")
        .def("start_screen_sharing", &core::Client::startScreenSharing,
            "Start screen sharing")
        .def("stop_screen_sharing", &core::Client::stopScreenSharing,
            "Stop screen sharing")
        .def("send_screen", &core::Client::sendScreen,
            "Send screen data", py::arg("data"))
        .def("start_camera_sharing", &core::Client::startCameraSharing,
            "Start camera sharing")
        .def("stop_camera_sharing", &core::Client::stopCameraSharing,
            "Stop camera sharing")
        .def("send_camera", &core::Client::sendCamera,
            "Send camera data", py::arg("data"));
}
