#include <pybind11/pybind11.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

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

    class Meta {
    public:
	    static py::object
	    newClass(py::object cls,
                 std::string cls_name, 
                 py::tuple bases, 
                 py::dict attrs);
    };

    struct JsonMaker{
    	JsonMaker()=default;
    	JsonMaker(const std::vector<std::tuple<int, std::string, py::object, rapidjson::Type>>& input);
    	template<typename Allocator>
    	void make(const py::object& entry, rapidjson::Value& json, Allocator& allocator) const;
    	void make(const py::object& entry);

    	bool is_leaf{false};
    	rapidjson::Type objType{rapidjson::kObjectType};
    	rapidjson::Document _doc;
    	rapidjson::Value _json{rapidjson::kObjectType};
    	std::vector<std::tuple<int, std::string, py::object>> _makers;
    };

    void register_json_maker(const std::string& cls_name, const py::list& input);

    struct JsonMakerCaller{
    	JsonMakerCaller(const std::string& cls_name): cls_name(cls_name){}
    	void make(const py::object& entry) const ;
    	std::string cls_name;
    };
}

