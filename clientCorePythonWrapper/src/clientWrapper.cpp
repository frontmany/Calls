#include <memory>

#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>
#include <system_error>

#include "core.h"
#include "eventListener.h"
#include "constants/errorCode.h"

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

    void onIncomingScreen(const std::vector<unsigned char>& data, int width, int height) override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onIncomingScreen, data, width, height);
    }

    void onStartOutgoingCallResult(std::error_code ec) override
    {
        int ec_value = static_cast<int>(ec.value());
        py::gil_scoped_acquire acquire;
        if (auto py_func = py::get_override(this, "onStartOutgoingCallResult")) {
            py_func(ec_value);
        }
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

    void onIncomingCamera(const std::vector<unsigned char>& data, int width, int height) override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onIncomingCamera, data, width, height);
    }

    void onLocalScreen(const std::vector<unsigned char>& data, int width, int height) override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onLocalScreen, data, width, height);
    }

    void onLocalCamera(const std::vector<unsigned char>& data, int width, int height) override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onLocalCamera, data, width, height);
    }

    void onStartCameraSharingError() override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onStartCameraSharingError);
    }

    void onOutgoingCallAccepted(const std::string& nickname) override
    {
        PYBIND11_OVERRIDE_PURE(void, core::EventListener, onOutgoingCallAccepted, nickname);
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
    py::enum_<core::constant::ErrorCode>(m, "ErrorCode")
        .value("NETWORK_ERROR", core::constant::ErrorCode::network_error)
        .value("TAKEN_NICKNAME", core::constant::ErrorCode::taken_nickname)
        .value("UNEXISTING_USER", core::constant::ErrorCode::unexisting_user)
        .value("CONNECTION_DOWN_WITH_USER", core::constant::ErrorCode::connection_down_with_user)
        .value("USER_LOGOUT", core::constant::ErrorCode::user_logout)
        .export_values();

    // EventListener interface - methods with error_code now take int instead
    // Use std::shared_ptr as holder: Client::start expects std::shared_ptr<EventListener>
    py::class_<core::EventListener, PyEventListener, std::shared_ptr<core::EventListener>>(m, "EventListener")
        .def(py::init<>())
        .def("onAuthorizationResult", [](core::EventListener& self, int ec_value) {
            self.onAuthorizationResult(std::error_code(ec_value, core::constant::error_category()));
        }, py::arg("ec_value"))
        .def("onIncomingScreenSharingStarted", &core::EventListener::onIncomingScreenSharingStarted)
        .def("onIncomingScreenSharingStopped", &core::EventListener::onIncomingScreenSharingStopped)
        .def("onIncomingScreen", &core::EventListener::onIncomingScreen)
        .def("onStartOutgoingCallResult", [](core::EventListener& self, int ec_value) {
            self.onStartOutgoingCallResult(std::error_code(ec_value, core::constant::error_category()));
        }, py::arg("ec_value"))
        .def("onStartScreenSharingError", &core::EventListener::onStartScreenSharingError)
        .def("onIncomingCameraSharingStarted", &core::EventListener::onIncomingCameraSharingStarted)
        .def("onIncomingCameraSharingStopped", &core::EventListener::onIncomingCameraSharingStopped)
        .def("onIncomingCamera", &core::EventListener::onIncomingCamera)
        .def("onLocalScreen", &core::EventListener::onLocalScreen)
        .def("onLocalCamera", &core::EventListener::onLocalCamera)
        .def("onStartCameraSharingError", &core::EventListener::onStartCameraSharingError)
        .def("onOutgoingCallAccepted", &core::EventListener::onOutgoingCallAccepted, py::arg("nickname") = "")
        .def("onOutgoingCallDeclined", &core::EventListener::onOutgoingCallDeclined)
        .def("onOutgoingCallTimeout", [](core::EventListener& self, int ec_value) {
            self.onOutgoingCallTimeout(std::error_code(ec_value, core::constant::error_category()));
        }, py::arg("ec_value"))
        .def("onIncomingCall", &core::EventListener::onIncomingCall)
        .def("onIncomingCallExpired", [](core::EventListener& self, int ec_value, const std::string& nickname) {
            self.onIncomingCallExpired(std::error_code(ec_value, core::constant::error_category()), nickname);
        }, py::arg("ec_value"), py::arg("nickname"))
        .def("onCallEndedByRemote", [](core::EventListener& self, int ec_value) {
            self.onCallEndedByRemote(std::error_code(ec_value, core::constant::error_category()));
        }, py::arg("ec_value"))
        .def("onCallParticipantConnectionDown", &core::EventListener::onCallParticipantConnectionDown)
        .def("onCallParticipantConnectionRestored", &core::EventListener::onCallParticipantConnectionRestored)
        .def("onConnectionDown", &core::EventListener::onConnectionDown)
        .def("onConnectionRestored", &core::EventListener::onConnectionRestored)
        .def("onConnectionRestoredAuthorizationNeeded", &core::EventListener::onConnectionRestoredAuthorizationNeeded);

    // Core (Client) class
    py::class_<core::Core>(m, "Client")
        .def(py::init<>())
        .def("init", [](core::Core& self, const std::string& host, const std::string& tcp_port, const std::string& udp_port,
                std::shared_ptr<core::EventListener> event_listener) {
            return self.start(host, host, tcp_port, udp_port, event_listener);
        },
            "Initialize calls client",
            py::arg("host"), py::arg("tcp_port"), py::arg("udp_port"), py::arg("event_listener"))
        .def("refresh_audio_devices", &core::Core::refreshAudioDevices,
            "Refresh audio devices")
        .def("mute_microphone", &core::Core::muteMicrophone,
            "Mute microphone", py::arg("is_mute"))
        .def("mute_speaker", &core::Core::muteSpeaker,
            "Mute speaker", py::arg("is_mute"))
        .def("set_input_volume", &core::Core::setInputVolume,
            "Set input volume", py::arg("volume"))
        .def("set_output_volume", &core::Core::setOutputVolume,
            "Set output volume", py::arg("volume"))
        .def("is_screen_sharing", &core::Core::isScreenSharing,
            "Check if screen sharing")
        .def("is_viewing_remote_screen", &core::Core::isViewingRemoteScreen,
            "Check if viewing remote screen")
        .def("is_camera_sharing", &core::Core::isCameraSharing,
            "Check if camera sharing")
        .def("is_viewing_remote_camera", &core::Core::isViewingRemoteCamera,
            "Check if viewing remote camera")
        .def("is_microphone_muted", &core::Core::isMicrophoneMuted,
            "Check if microphone is muted")
        .def("is_speaker_muted", &core::Core::isSpeakerMuted,
            "Check if speaker is muted")
        .def("is_authorized", &core::Core::isAuthorized,
            "Check if authorized")
        .def("is_outgoing_call", &core::Core::isOutgoingCall,
            "Check if outgoing call")
        .def("is_active_call", &core::Core::isActiveCall,
            "Check if active call")
        .def("is_connection_down", &core::Core::isConnectionDown,
            "Check if connection down")
        .def("get_input_volume", &core::Core::getInputVolume,
            "Get input volume")
        .def("get_output_volume", &core::Core::getOutputVolume,
            "Get output volume")
        .def("get_incoming_calls_count", &core::Core::getIncomingCallsCount,
            "Get incoming calls count")
        .def("get_callers", &core::Core::getCallers,
            "Get list of callers")
        .def("get_my_nickname", &core::Core::getMyNickname,
            "Get my nickname")
        .def("get_nickname_whom_outgoing_call", &core::Core::getNicknameWhomOutgoingCall,
            "Get nickname of person being called")
        .def("get_nickname_in_call_with", &core::Core::getNicknameInCallWith,
            "Get nickname of person in call with")
        .def("authorize", [](core::Core& self, const std::string& nickname) -> int {
            return static_cast<int>(self.authorize(nickname).value());
        }, "Authorize with nickname", py::arg("nickname"))
        .def("logout", [](core::Core& self) -> int {
            return static_cast<int>(self.logout().value());
        }, "Logout")
        .def("start_outgoing_call", [](core::Core& self, const std::string& friend_nickname) -> int {
            return static_cast<int>(self.startOutgoingCall(friend_nickname).value());
        }, "Start outgoing call", py::arg("friend_nickname"))
        .def("stop_outgoing_call", [](core::Core& self) -> int {
            return static_cast<int>(self.stopOutgoingCall().value());
        }, "Stop outgoing call")
        .def("accept_call", [](core::Core& self, const std::string& friend_nickname) -> int {
            return static_cast<int>(self.acceptCall(friend_nickname).value());
        }, "Accept incoming call", py::arg("friend_nickname"))
        .def("decline_call", [](core::Core& self, const std::string& friend_nickname) -> int {
            return static_cast<int>(self.declineCall(friend_nickname).value());
        }, "Decline incoming call", py::arg("friend_nickname"))
        .def("end_call", [](core::Core& self) -> int {
            return static_cast<int>(self.endCall().value());
        }, "End current call")
        .def("start_screen_sharing", [](core::Core& self, int screen_index) -> int {
            core::media::ScreenCaptureTarget target;
            target.index = screen_index;
            return static_cast<int>(self.startScreenSharing(target).value());
        }, "Start screen sharing", py::arg("screen_index") = 0)
        .def("stop_screen_sharing", [](core::Core& self) -> int {
            return static_cast<int>(self.stopScreenSharing().value());
        }, "Stop screen sharing")
        .def("start_camera_sharing", [](core::Core& self, const std::string& device_name) -> int {
            return static_cast<int>(self.startCameraSharing(device_name).value());
        }, "Start camera sharing", py::arg("device_name") = "")
        .def("stop_camera_sharing", [](core::Core& self) -> int {
            return static_cast<int>(self.stopCameraSharing().value());
        }, "Stop camera sharing")
        .def("stop", &core::Core::stop, "Stop client and cleanup resources");
}
