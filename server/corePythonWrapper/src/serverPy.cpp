#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>
#include "server.h"

namespace py = pybind11;

PYBIND11_MODULE(callsServerPy, m) {
    m.doc() = "Calls Server Python Binding";

    py::class_<callifornia::Server>(m, "Server")
        .def(py::init<const std::string&>(),
            "Create Server instance",
            py::arg("port"))
        .def("run", &callifornia::Server::run,
            "Start the server (blocking)")
        .def("stop", &callifornia::Server::stop,
            "Stop the server");
}