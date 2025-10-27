#include "updater.h"
#include "pybind11/pybind11.h"

namespace py = pybind11;

PYBIND11_MODULE(serverUpdaterPy, m, py::mod_gil_not_used()) {
	m.doc() = "a module which contains only one function to run serverUpdater";

	m.def("runServerUpdater", &runServerUpdater, "A function that runs the server");
}