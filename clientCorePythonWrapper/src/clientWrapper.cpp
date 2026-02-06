#include <memory>

#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>
#include <system_error>

#include "core.h"
#include "eventListener.h"
#include "errorCode.h"

namespace py = pybind11;

class PyEventListener : public core::EventListener
{
public:
    using core::EventListener::EventListener;

    void onAuthorizationResult(std::error_code ec) override
    {
        // Convert error_code to int to avoid serialization issues
        int ec_value = static_cast<int>(ec.value());
        py::gil_scoped_acquire acquire;
        if (auto py_func = py::get_override(this, "onAuthorizationResult")) {
            py_func(ec_value);
        }
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

    void onScreenSharingStarted() override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onScreenSharingStarted);
    }

    void onScreenSharingStopped() override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onScreenSharingStopped);
    }

    void onStartScreenSharingError() override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onStartScreenSharingError);
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

    void onCameraSharingStarted() override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onCameraSharingStarted);
    }

    void onCameraSharingStopped() override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onCameraSharingStopped);
    }

    void onStartCameraSharingError() override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onStartCameraSharingError);
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
        int ec_value = static_cast<int>(ec.value());
        py::gil_scoped_acquire acquire;
        if (auto py_func = py::get_override(this, "onOutgoingCallTimeout")) {
            py_func(ec_value);
        }
    }

    void onIncomingCall(const std::string& nickname) override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onIncomingCall, nickname);
    }

    void onIncomingCallExpired(std::error_code ec, const std::string& nickname) override
    {
        int ec_value = static_cast<int>(ec.value());
        py::gil_scoped_acquire acquire;
        if (auto py_func = py::get_override(this, "onIncomingCallExpired")) {
            py_func(ec_value, nickname);
        }
    }

    void onCallEndedByRemote(std::error_code ec) override
    {
        int ec_value = static_cast<int>(ec.value());
        py::gil_scoped_acquire acquire;
        if (auto py_func = py::get_override(this, "onCallEndedByRemote")) {
            py_func(ec_value);
        }
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

    // EventListener interface - methods with error_code now take int instead
    // Use std::shared_ptr as holder: Client::start expects std::shared_ptr<EventListener>
    py::class_<core::EventListener, PyEventListener, std::shared_ptr<core::EventListener>>(m, "EventListener")
        .def(py::init<>())
        .def("onAuthorizationResult", [](core::EventListener& self, int ec_value) {
            self.onAuthorizationResult(std::error_code(ec_value, core::error_category()));
        }, py::arg("ec_value"))
.def("onIncomingScreenSharingStarted", &core::EventListener::onIncomingScreenSharingStarted)
        .def("onIncomingScreenSharingStopped", &core::EventListener::onIncomingScreenSharingStopped)
        .def("onIncomingScreen", &core::EventListener::onIncomingScreen)
        .def("onScreenSharingStarted", &core::EventListener::onScreenSharingStarted)
        .def("onScreenSharingStopped", &core::EventListener::onScreenSharingStopped)
        .def("onStartScreenSharingError", &core::EventListener::onStartScreenSharingError)
        .def("onIncomingCameraSharingStarted", &core::EventListener::onIncomingCameraSharingStarted)
        .def("onIncomingCameraSharingStopped", &core::EventListener::onIncomingCameraSharingStopped)
        .def("onIncomingCamera", &core::EventListener::onIncomingCamera)
        .def("onCameraSharingStarted", &core::EventListener::onCameraSharingStarted)
        .def("onCameraSharingStopped", &core::EventListener::onCameraSharingStopped)
        .def("onStartCameraSharingError", &core::EventListener::onStartCameraSharingError)
        .def("onOutgoingCallAccepted", &core::EventListener::onOutgoingCallAccepted)
        .def("onOutgoingCallDeclined", &core::EventListener::onOutgoingCallDeclined)
        .def("onOutgoingCallTimeout", [](core::EventListener& self, int ec_value) {
            self.onOutgoingCallTimeout(std::error_code(ec_value, core::error_category()));
        }, py::arg("ec_value"))
        .def("onIncomingCall", &core::EventListener::onIncomingCall)
        .def("onIncomingCallExpired", [](core::EventListener& self, int ec_value, const std::string& nickname) {
            self.onIncomingCallExpired(std::error_code(ec_value, core::error_category()), nickname);
        }, py::arg("ec_value"), py::arg("nickname"))
        .def("onCallEndedByRemote", [](core::EventListener& self, int ec_value) {
            self.onCallEndedByRemote(std::error_code(ec_value, core::error_category()));
        }, py::arg("ec_value"))
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
            py::arg("host"), py::arg("tcp_port"), py::arg("udp_port"), py::arg("event_listener"))
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
        .def("authorize", [](core::Client& self, const std::string& nickname) -> int {
            return static_cast<int>(self.authorize(nickname).value());
        }, "Authorize with nickname", py::arg("nickname"))
        .def("logout", [](core::Client& self) -> int {
            return static_cast<int>(self.logout().value());
        }, "Logout")
        .def("start_outgoing_call", [](core::Client& self, const std::string& friend_nickname) -> int {
            return static_cast<int>(self.startOutgoingCall(friend_nickname).value());
        }, "Start outgoing call", py::arg("friend_nickname"))
        .def("stop_outgoing_call", [](core::Client& self) -> int {
            return static_cast<int>(self.stopOutgoingCall().value());
        }, "Stop outgoing call")
        .def("accept_call", [](core::Client& self, const std::string& friend_nickname) -> int {
            return static_cast<int>(self.acceptCall(friend_nickname).value());
        }, "Accept incoming call", py::arg("friend_nickname"))
        .def("decline_call", [](core::Client& self, const std::string& friend_nickname) -> int {
            return static_cast<int>(self.declineCall(friend_nickname).value());
        }, "Decline incoming call", py::arg("friend_nickname"))
        .def("end_call", [](core::Client& self) -> int {
            return static_cast<int>(self.endCall().value());
        }, "End current call")
        .def("start_screen_sharing", [](core::Client& self) -> int {
            return static_cast<int>(self.startScreenSharing().value());
        }, "Start screen sharing")
        .def("stop_screen_sharing", [](core::Client& self) -> int {
            return static_cast<int>(self.stopScreenSharing().value());
        }, "Stop screen sharing")
        .def("send_screen", [](core::Client& self, const std::vector<unsigned char>& data) -> int {
            return static_cast<int>(self.sendScreen(data).value());
        }, "Send screen data", py::arg("data"))
        .def("start_camera_sharing", [](core::Client& self) -> int {
            return static_cast<int>(self.startCameraSharing().value());
        }, "Start camera sharing")
        .def("stop_camera_sharing", [](core::Client& self) -> int {
            return static_cast<int>(self.stopCameraSharing().value());
        }, "Stop camera sharing")
        .def("send_camera", [](core::Client& self, const std::vector<unsigned char>& data) -> int {
            return static_cast<int>(self.sendCamera(data).value());
        }, "Send camera data", py::arg("data"))
        .def("stop", &core::Client::stop, "Stop client and cleanup resources");
}
