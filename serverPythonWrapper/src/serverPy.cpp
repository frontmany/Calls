#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>
#include "server.h"

namespace py = pybind11;

PYBIND11_MODULE(callsServerPy, m) {
    m.doc() = "Calls Server Python Binding";

    py::class_<server::Server>(m, "Server")
        .def(py::init<const std::string&, const std::string&>(),
            "Create Server instance",
            py::arg("tcp_port"),
            py::arg("udp_port"))
        .def("run", &server::Server::run,
            "Start the server (blocking)")
        .def("stop", &server::Server::stop,
            "Stop the server");
}