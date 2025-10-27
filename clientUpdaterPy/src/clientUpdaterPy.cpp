#include "clientUpdater.h"
#include <pybind11/pybind11.h>
#include <pybind11/functional.h>

namespace py = pybind11;

namespace updater {

    PYBIND11_MODULE(clientUpdaterPy, m) {
        py::enum_<CheckResult>(m, "CheckResult")
            .value("UPDATE_NOT_NEEDED", CheckResult::UPDATE_NOT_NEEDED)
            .value("REQUIRED_UPDATE", CheckResult::REQUIRED_UPDATE)
            .value("POSSIBLE_UPDATE", CheckResult::POSSIBLE_UPDATE)
            .export_values();

        py::enum_<OperationSystemType>(m, "OperationSystemType")
            .value("WINDOWS", OperationSystemType::WINDOWS)
            .value("LINUX", OperationSystemType::LINUX)
            .value("MAC", OperationSystemType::MAC)
            .export_values();

        py::class_<ClientUpdater>(m, "ClientUpdater")
            .def(py::init<
                std::function<void(CheckResult)>&&,
                std::function<void()>&&,
                std::function<void()>&&
            >())
            .def("connect", &ClientUpdater::connect,
                py::arg("host"), py::arg("port"))
            .def("disconnect", &ClientUpdater::disconnect)
            .def("checkUpdates", &ClientUpdater::checkUpdates,
                py::arg("currentVersionNumber"))
            .def("startUpdate", &ClientUpdater::startUpdate,
                py::arg("type"))
            .def("declineUpdate", &ClientUpdater::declineUpdate);
    }

}