#include <pybind11/pybind11.h>
#include "cerpypy.h"

namespace py = pybind11;
 
PYBIND11_MODULE(cerpypy, m) 
{
    m.def("version", &cerpypy::version, "cerpypy current version");
    m.def("register_json_maker", &cerpypy::register_json_maker, "register the json maker");
    m.def("register_pyobj_maker", &cerpypy::register_pyobj_maker, "register the pyobj maker");


    py::class_<cerpypy::JsonMakerCaller>(m, "JsonMakerCaller")
        .def(py::init<std::string>())
		.def("make", &cerpypy::JsonMakerCaller::make);

    py::class_<cerpypy::PyObjMakerCaller>(m, "PyObjMakerCaller")
        .def(py::init<std::string>())
		.def("make", &cerpypy::PyObjMakerCaller::make);
}
