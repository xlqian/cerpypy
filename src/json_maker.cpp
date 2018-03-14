#include "cerpypy.h"
#include <python2.7/Python.h>

namespace cerpypy {


static std::vector<std::string> keyCollection{};

static std::map<std::string, JsonMaker> makerMap{};

static std::vector<JsonMaker> makerCollection{};

JsonMaker::JsonMaker(const std::vector<std::tuple<int,
												  std::string,
												  py::object,
												  rapidjson::Type>>& input) {
	// register string
	int key_id;
	std::string maker_name;
	py::object getter;
	rapidjson::Type type;
	for (auto& p : input) {
		std::tie(key_id, maker_name, getter, type) = p;
		if (! PyCallable_Check(getter.ptr())) {
			 throw std::exception();
		}
		auto it = makerMap.find(maker_name);
		if(it==makerMap.cend()) {
			auto& maker = makerMap[maker_name];
			maker.is_leaf = true;
		}
		_makers.emplace_back(key_id, maker_name, getter);
	}
}

template<typename Allocator>
void JsonMaker::make(const py::object& entry, rapidjson::Value& json, Allocator& allocator) const {

	if (is_leaf) {
		json['a'] = "toto";
		//std::cout << "is_node" << std::endl;

	}

	for(const auto& p : _makers) {
		rapidjson::Value sub_value{rapidjson::kObjectType};
		makerMap[std::get<1>(p)].make(std::get<2>(p)(entry), sub_value, allocator);
		json.AddMember(rapidjson::StringRef(keyCollection[std::get<0>(p)].c_str()), sub_value.Move(), allocator);
	};
};

void JsonMaker::make(const py::object& entry) {
	if (is_leaf) {
		//std::cout << "is_node" << std::endl;
	}

	auto& allocator = _doc.GetAllocator();
	_doc.SetObject();

	for(const auto& p : _makers) {
		rapidjson::Value sub_value{rapidjson::kObjectType};
		makerMap[std::get<1>(p)].make(std::get<2>(p)(entry), sub_value, allocator);
		_doc.AddMember(rapidjson::StringRef(keyCollection[std::get<0>(p)].c_str()), sub_value.Move(), allocator);

	};
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	_doc.Accept(writer);
	//std::cout << buffer.GetString() << std::endl;
	//return buffer.GetString();
}


const static std::map<std::string, rapidjson::Type> str2rapidjsonType = {{"object", rapidjson::kObjectType},
																		 {"array", rapidjson::kArrayType}};

void register_json_maker(const std::string& cls_name, const py::list& input){

	std::vector<std::tuple<int,
						   std::string,
						   py::object,
						   rapidjson::Type>> maker_input;

		for (const auto& item : input){
		py::tuple tuple = py::cast<py::tuple>(item);

		const std::string& key = py::str(tuple[0]);
		const std::string& maker_name = py::str(tuple[1]);
		const py::object& getter = tuple[2];
		std::string object_type = py::str(tuple[3]);

		auto it = str2rapidjsonType.find(object_type);
		if (it == str2rapidjsonType.end()){
			 throw std::exception();
		}
		int key_id = keyCollection.size();
		keyCollection.push_back(key);

		maker_input.push_back(std::make_tuple(key_id, maker_name, getter, it->second));
    }
	makerMap.emplace(cls_name, JsonMaker{maker_input});
}

void JsonMakerCaller::make(const py::object& entry) const {
	auto it = makerMap.find(cls_name);
	if (it == makerMap.end()) {
		throw std::exception();
	}else {
	    it->second.make(entry);
	}

} ;


}
