#include "cerpypy.h"
#include <python2.7/Python.h>

namespace cerpypy {


static std::vector<std::string> keyCollection{};

//static std::map<std::string, JsonMaker> makerMap{};

static std::vector<std::pair<std::string, JsonMaker>> makerCollection{};

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

		bool found = false;
		size_t maker_id = 0;
		for(const auto& p : makerCollection) {
			if(maker_name == p.first) {
				found = true;
				break;
			}
			++maker_id;
		}

		if (!found) {
			auto jm = JsonMaker();
			jm.is_leaf = true;
			makerCollection.emplace_back(maker_name, std::move(jm));
		}

		_makers.emplace_back(key_id, maker_id, py::reinterpret_borrow<py::object>(getter));
	}
}

template<typename Allocator>
void JsonMaker::make(const py::object& entry, rapidjson::Value& json, Allocator& allocator) const {


	for(const auto& p : _makers) {
		const std::string& key = keyCollection[std::get<0>(p)];

		const auto& maker = makerCollection[std::get<1>(p)].second;
		if (!maker.is_leaf){
			rapidjson::Value sub_value{rapidjson::kObjectType};
			const auto& sub_entry = std::get<2>(p)(entry);
			maker.make(sub_entry, sub_value, allocator);
			json.AddMember(rapidjson::StringRef(key.c_str()), sub_value.Move(), allocator);
		}else{
			const auto& sub_entry = std::get<2>(p)(entry);
			if (PyInt_Check(sub_entry.ptr())){
				json.AddMember(rapidjson::StringRef(keyCollection[std::get<0>(p)].c_str()),  PyLong_AsLong(sub_entry.ptr()), allocator);
			}else if (PyFloat_Check(sub_entry.ptr())){
				json.AddMember(rapidjson::StringRef(keyCollection[std::get<0>(p)].c_str()),  PyFloat_AsDouble(sub_entry.ptr()), allocator);
			}else if (PyString_Check(sub_entry.ptr())){
				json.AddMember(rapidjson::StringRef(keyCollection[std::get<0>(p)].c_str()),  rapidjson::StringRef(PyString_AsString(sub_entry.ptr())), allocator);
			}
		}
	};
};

std::string JsonMaker::make(const py::object& entry) {
	/*
	 * Need to handle this case
	if (is_leaf) {
		std::cout << "is_node" << std::endl;
	}
	*/
	auto& allocator = _doc.GetAllocator();
	_doc.SetObject();

	for(const auto& p : _makers) {
		const std::string& key = keyCollection[std::get<0>(p)];
		const auto& maker = makerCollection[std::get<1>(p)].second;
		if (!maker.is_leaf){
			rapidjson::Value sub_value{rapidjson::kObjectType};
			const auto& sub_entry = std::get<2>(p)(entry);
			maker.make(sub_entry, sub_value, allocator);
			_doc.AddMember(rapidjson::StringRef(keyCollection[std::get<0>(p)].c_str()), sub_value.Move(), allocator);
		}else {
			const auto& sub_entry = std::get<2>(p)(entry);
			if (PyInt_Check(sub_entry.ptr())){
				_doc.AddMember(rapidjson::StringRef(keyCollection[std::get<0>(p)].c_str()),  PyLong_AsLong(sub_entry.ptr()), allocator);
			}else if (PyFloat_Check(sub_entry.ptr())){
				_doc.AddMember(rapidjson::StringRef(keyCollection[std::get<0>(p)].c_str()),  PyFloat_AsDouble(sub_entry.ptr()), allocator);
			}else if (PyString_Check(sub_entry.ptr())){
				_doc.AddMember(rapidjson::StringRef(keyCollection[std::get<0>(p)].c_str()),  rapidjson::StringRef(PyString_AsString(sub_entry.ptr())), allocator);
			}
		}
	};
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	_doc.Accept(writer);
	//std::cout << buffer.GetString() << std::endl;
	return  buffer.GetString();
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
		py::tuple tuple = py::reinterpret_borrow<py::tuple>(item);

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

		maker_input.push_back(std::make_tuple(key_id, maker_name, py::reinterpret_borrow<py::object>(getter), it->second));
    }
	makerCollection.emplace_back(cls_name, JsonMaker{maker_input});
}

std::string JsonMakerCaller::make(const py::object& entry) const {
	for(auto& p : makerCollection) {
		if(cls_name == p.first) {
			return p.second.make(entry);
		}
	}
	throw std::exception();
} ;


}
