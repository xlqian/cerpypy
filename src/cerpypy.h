#include <pybind11/pybind11.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <python2.7/Python.h>
#include <string>
#include <iostream>
#include <memory>
#include <exception>
#include <vector>
#include <tuple>
#include <unordered_map>
#include <map>
#include <boost/optional.hpp>

namespace py = pybind11;

namespace cerpypy {
    const char * version();
    using GetterType = py::object;

    struct PyObjMaker{
    	PyObjMaker()=default;
    	PyObjMaker(const std::vector<std::tuple<int, std::string, py::object, PyTypeObject*>>& input);

    	void make(const py::handle& entry, py::handle& output) const;
    	py::dict make(const py::object& entry);

    	bool is_leaf{false};
    	PyTypeObject* objType{&PyDict_Type};
    	py::dict _obj;
    	//std::vector<std::tuple<int, std::string, py::object>> _makers;
    	std::vector<std::tuple<int, int, py::object, PyTypeObject*>> _makers;
    };

    struct JsonMaker{
    	JsonMaker()=default;
    	JsonMaker(const std::vector<std::tuple<int, std::string, py::object, rapidjson::Type>>& input);
    	template<typename Allocator>
    	void make(const py::object& entry, rapidjson::Value& json, Allocator& allocator) const;
    	std::string make(const py::object& entry);

    	bool is_leaf{false};
    	rapidjson::Type objType{rapidjson::kObjectType};
    	rapidjson::Document _doc;
    	rapidjson::Value _json{rapidjson::kObjectType};
    	//std::vector<std::tuple<int, std::string, py::object>> _makers;
    	std::vector<std::tuple<int, int, py::object, rapidjson::Type>> _makers;
    };

    void register_json_maker(const std::string& cls_name, const py::list& input);
    void register_dict_maker(const std::string& cls_name, const py::list& input);

    struct JsonMakerCaller{
    	JsonMakerCaller(const std::string& cls_name);
    	std::string make(const py::object& entry) const ;
    	std::string cls_name;
    };
}

