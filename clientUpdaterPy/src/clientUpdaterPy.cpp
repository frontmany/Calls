#include "updater.h"
#include <pybind11/pybind11.h>
#include <pybind11/functional.h>

namespace py = pybind11;

class PyCallbacksInterface : public updater::CallbacksInterface, public py::trampoline_self_life_support
{
public:
    using updater::CallbacksInterface::CallbacksInterface;

    void onUpdatesCheckResult(updater::UpdatesCheckResult updatesCheckResult) override
    {
        PYBIND11_OVERRIDE_PURE(
            void,
            updater::CallbacksInterface,
            onUpdatesCheckResult,
            updatesCheckResult
        );
    }

    void onLoadingProgress(double progress) override
    {
        PYBIND11_OVERRIDE_PURE(
            void,
            updater::CallbacksInterface,
            onLoadingProgress,
            progress
        );
    }

    void onUpdateLoaded(bool emptyUpdate) override
    {
        PYBIND11_OVERRIDE_PURE(
            void,
            updater::CallbacksInterface,
            onUpdateLoaded,
            emptyUpdate
        );
    }

    void onError() override
    {
        PYBIND11_OVERRIDE_PURE(
            void,
            updater::CallbacksInterface,
            onError
        );
    }
};

PYBIND11_MODULE(clientUpdaterPy, m)
{
    m.doc() = "Client Updater Python Binding";

    py::enum_<updater::UpdatesCheckResult>(m, "UpdatesCheckResult")
        .value("UPDATE_NOT_NEEDED", updater::UpdatesCheckResult::UPDATE_NOT_NEEDED)
        .value("REQUIRED_UPDATE", updater::UpdatesCheckResult::REQUIRED_UPDATE)
        .value("POSSIBLE_UPDATE", updater::UpdatesCheckResult::POSSIBLE_UPDATE)
        .export_values();

    py::enum_<updater::OperationSystemType>(m, "OperationSystemType")
        .value("WINDOWS", updater::OperationSystemType::WINDOWS)
        .value("LINUX", updater::OperationSystemType::LINUX)
        .value("MAC", updater::OperationSystemType::MAC)
        .export_values();

    py::enum_<updater::State>(m, "State")
        .value("DISCONNECTED", updater::State::DISCONNECTED)
        .value("AWAITING_SERVER_RESPONSE", updater::State::AWAITING_SERVER_RESPONSE)
        .value("AWAITING_UPDATES_CHECK", updater::State::AWAITING_UPDATES_CHECK)
        .value("AWAITING_START_UPDATE", updater::State::AWAITING_START_UPDATE)
        .export_values();

    py::class_<updater::CallbacksInterface, PyCallbacksInterface, py::smart_holder>(m, "CallbacksInterface")
        .def(py::init<>())
        .def("onUpdatesCheckResult", &updater::CallbacksInterface::onUpdatesCheckResult)
        .def("onLoadingProgress", &updater::CallbacksInterface::onLoadingProgress)
        .def("onUpdateLoaded", &updater::CallbacksInterface::onUpdateLoaded)
        .def("onError", &updater::CallbacksInterface::onError);

    m.def("init", &updater::init,
        "Initialize client updater with callbacks handler",
        py::arg("callbacksHandler"));
    m.def("connect", &updater::connect,
        "Connect to update server",
        py::arg("host"), py::arg("port"));
    m.def("disconnect", &updater::disconnect,
        "Disconnect from update server");
    m.def("checkUpdates", &updater::checkUpdates,
        "Check for updates",
        py::arg("currentVersionNumber"));
    m.def("startUpdate", &updater::startUpdate,
        "Start update process",
        py::arg("type"));
    m.def("isConnected", &updater::isConnected,
        "Check if connected to server");
    m.def("isAwaitingServerResponse", &updater::isAwaitingServerResponse,
        "Check if awaiting server response");
    m.def("isAwaitingCheckUpdatesFunctionCall", &updater::isAwaitingCheckUpdatesFunctionCall,
        "Check if awaiting check updates function call");
    m.def("isAwaitingStartUpdateFunctionCall", &updater::isAwaitingStartUpdateFunctionCall,
        "Check if awaiting start update function call");
    m.def("getServerHost", &updater::getServerHost,
        "Get server host");
    m.def("getServerPort", &updater::getServerPort,
        "Get server port");
}