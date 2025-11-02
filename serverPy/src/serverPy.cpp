#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>
#include "callsServer.h"

namespace py = pybind11;

PYBIND11_MODULE(callsServerPy, m) {
    m.doc() = "Calls Server Python Binding";

    py::class_<CallsServer>(m, "CallsServer")
        .def(py::init<const std::string&>(),
            "Create CallsServer instance",
            py::arg("port"))
        .def("run", &CallsServer::run,
            "Start the server (blocking)")
        .def("stop", &CallsServer::stop,
            "Stop the server");
}