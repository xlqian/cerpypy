#include <pybind11/pybind11.h>
#include "cerpypy.h"

namespace py = pybind11;
 
PYBIND11_MODULE(cerpypy, m) 
{
    m.def("version", &cerpypy::version, "cerpypy current version");
    m.def("register_json_maker", &cerpypy::register_json_maker, "register the json maker");


    py::class_<cerpypy::JsonMakerCaller>(m, "JsonMakerCaller")
        .def(py::init<std::string>())
		.def("make", &cerpypy::JsonMakerCaller::make);
}
