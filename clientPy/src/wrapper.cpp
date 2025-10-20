#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>
#include "calls.h"

namespace py = pybind11;

class PyHandler : public calls::Handler, public py::trampoline_self_life_support {
public:
    using calls::Handler::Handler;

    void onAuthorizationResult(calls::ErrorCode ec) override {
        PYBIND11_OVERRIDE_PURE(
            void,
            calls::Handler,
            onAuthorizationResult,
            ec
        );
    }

    void onStartCallingResult(calls::ErrorCode ec) override {
        PYBIND11_OVERRIDE_PURE(
            void,
            calls::Handler,
            onStartCallingResult,
            ec
        );
    }

    void onAcceptCallResult(calls::ErrorCode ec, const std::string& nickname) override {
        PYBIND11_OVERRIDE_PURE(
            void,
            calls::Handler,
            onAcceptCallResult,
            ec,
            nickname
        );
    }

    void onMaximumCallingTimeReached() override {
        PYBIND11_OVERRIDE_PURE(
            void,
            calls::Handler,
            onMaximumCallingTimeReached
        );
    }

    void onCallingAccepted() override {
        PYBIND11_OVERRIDE_PURE(
            void,
            calls::Handler,
            onCallingAccepted
        );
    }

    void onCallingDeclined() override {
        PYBIND11_OVERRIDE_PURE(
            void,
            calls::Handler,
            onCallingDeclined
        );
    }

    void onIncomingCall(const std::string& friendNickname) override {
        PYBIND11_OVERRIDE_PURE(
            void,
            calls::Handler,
            onIncomingCall,
            friendNickname
        );
    }

    void onIncomingCallExpired(const std::string& friendNickname) override {
        PYBIND11_OVERRIDE_PURE(
            void,
            calls::Handler,
            onIncomingCallExpired,
            friendNickname
        );
    }

    void onNetworkError() override {
        PYBIND11_OVERRIDE_PURE(
            void,
            calls::Handler,
            onNetworkError
        );
    }

    void onConnectionRestored() override {
        PYBIND11_OVERRIDE_PURE(
            void,
            calls::Handler,
            onConnectionRestored
        );
    }

    void onRemoteUserEndedCall() override {
        PYBIND11_OVERRIDE_PURE(
            void,
            calls::Handler,
            onRemoteUserEndedCall
        );
    }
};

bool init_wrapper(const std::string& host,
    const std::string& port,
    std::unique_ptr<calls::Handler> handler) {
    return calls::init(host, port, std::move(handler));
}

PYBIND11_MODULE(callsClientPy, m) {
    m.doc() = "Calls Client Python Binding";

    py::enum_<calls::ErrorCode>(m, "ErrorCode")
        .value("OK", calls::ErrorCode::OK)
        .value("TIMEOUT", calls::ErrorCode::TIMEOUT)
        .value("TAKEN_NICKNAME", calls::ErrorCode::TAKEN_NICKNAME)
        .value("UNEXISTING_USER", calls::ErrorCode::UNEXISTING_USER)
        .export_values();

    py::class_<calls::Handler, PyHandler, py::smart_holder>(m, "Handler")
        .def(py::init<>())
        .def("onAuthorizationResult", &calls::Handler::onAuthorizationResult)
        .def("onStartCallingResult", &calls::Handler::onStartCallingResult)
        .def("onAcceptCallResult", &calls::Handler::onAcceptCallResult)
        .def("onMaximumCallingTimeReached", &calls::Handler::onMaximumCallingTimeReached)
        .def("onCallingAccepted", &calls::Handler::onCallingAccepted)
        .def("onCallingDeclined", &calls::Handler::onCallingDeclined)
        .def("onIncomingCall", &calls::Handler::onIncomingCall)
        .def("onIncomingCallExpired", &calls::Handler::onIncomingCallExpired)
        .def("onNetworkError", &calls::Handler::onNetworkError)
        .def("onConnectionRestored", &calls::Handler::onConnectionRestored)
        .def("onRemoteUserEndedCall", &calls::Handler::onRemoteUserEndedCall);

    m.def("init", &init_wrapper,
        "Initialize calls client",
        py::arg("host"), py::arg("port"), py::arg("handler"));

    m.def("run", &calls::run, "Run calls client");
    m.def("get_nickname", &calls::getNickname, "Get current nickname");
    m.def("get_callers", &calls::getCallers, "Get list of callers");
    m.def("authorize", &calls::authorize, "Authorize with nickname", py::arg("nickname"));
    m.def("start_calling", &calls::startCalling, "Start calling friend", py::arg("friend_nickname"));
    m.def("stop_calling", &calls::stopCalling, "Stop calling");
    m.def("accept_call", &calls::acceptCall, "Accept incoming call", py::arg("friend_nickname"));
    m.def("decline_call", &calls::declineCall, "Decline incoming call", py::arg("friend_nickname"));
    m.def("end_call", &calls::endCall, "End current call");
    m.def("get_incoming_calls_count", &calls::getIncomingCallsCount, "Get incoming calls count");
    m.def("refresh_audio_devices", &calls::refreshAudioDevices, "Refresh audio devices");
    m.def("mute_microphone", &calls::muteMicrophone, "Mute microphone", py::arg("is_mute"));
    m.def("mute_speaker", &calls::muteSpeaker, "Mute speaker", py::arg("is_mute"));
    m.def("is_microphone_muted", &calls::isMicrophoneMuted, "Check if microphone is muted");
    m.def("is_speaker_muted", &calls::isSpeakerMuted, "Check if speaker is muted");
    m.def("set_input_volume", &calls::setInputVolume, "Set input volume", py::arg("volume"));
    m.def("set_output_volume", &calls::setOutputVolume, "Set output volume", py::arg("volume"));
    m.def("get_input_volume", &calls::getInputVolume, "Get input volume");
    m.def("get_output_volume", &calls::getOutputVolume, "Get output volume");
    m.def("logout", &calls::logout, "Logout");
    m.def("reset", &calls::reset, "Reset client");
    m.def("stop", &calls::stop, "Stop client");
    m.def("is_running", &calls::isRunning, "Check if client is running");
    m.def("is_authorized", &calls::isAuthorized, "Check if authorized");
    m.def("is_calling", &calls::isCalling, "Check if calling");
    m.def("is_busy", &calls::isBusy, "Check if busy");
    m.def("get_nickname_whom_calling", &calls::getNicknameWhomCalling, "Get nickname of person being called");
    m.def("get_nickname_in_call_with", &calls::getNicknameInCallWith, "Get nickname of person in call with");
}