#include "cerpypy.h"
#include <python2.7/Python.h>

namespace cerpypy {


static std::vector<std::string> keyCollection{};

//static std::map<std::string, JsonMaker> makerMap{};

static std::map<std::string, size_t> maker_name_idx_map{};

static std::vector<std::pair<std::string, JsonMaker>> makerCollection{};


const static auto int_lambda = [&](PyObject* p, rapidjson::Value& v){
	v.Set(PyLong_AsLong(p));
};

const static auto float_lambda = [&](PyObject* p, rapidjson::Value& v){
	v.Set(PyFloat_AsDouble(p));
};

const static auto string_lambda = [&](PyObject* p, rapidjson::Value& v){
	v.SetString(rapidjson::StringRef(PyString_AsString(p)));
};


const static std::map<PyTypeObject*, std::function<void(PyObject*, rapidjson::Value&)>>
		lambda_map{{&PyString_Type, string_lambda},
				   {&PyFloat_Type, float_lambda},
	               {&PyInt_Type, int_lambda}};

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
			maker_name_idx_map.emplace(maker_name, maker_id);
		}
		_makers.emplace_back(key_id, maker_id, py::reinterpret_borrow<py::object>(getter), type);
	}
}

template<typename Allocator>
void JsonMaker::make(const py::object& entry, rapidjson::Value& json, Allocator& allocator) const {


	for(const auto& p : _makers) {
		const std::string& key = keyCollection[std::get<0>(p)];

		const auto& maker = makerCollection[std::get<1>(p)].second;
		const auto& sub_entry = std::get<2>(p)(entry);
		auto value_type = std::get<3>(p);
		if (!maker.is_leaf){

			rapidjson::Value sub_value{value_type};
			if (value_type == rapidjson::kObjectType) {
				maker.make(sub_entry, sub_value, allocator);
				json.AddMember(rapidjson::StringRef(key.c_str()), sub_value.Move(), allocator);
			}else if (value_type == rapidjson::kArrayType) {
				std::cout << "array type" << std::endl;
			}
		}else{
			PyTypeObject* sub_entry_type = sub_entry.ptr()->ob_type;
			rapidjson::Value v;
			auto it = lambda_map.find(sub_entry_type);
			assert(it!=lambda_map.end());
			it->second(sub_entry.ptr(), v);
			json.AddMember(rapidjson::StringRef(key.c_str()), v.Move(), allocator);
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
		const auto& sub_entry = std::get<2>(p)(entry);
		auto value_type = std::get<3>(p);
		if (!maker.is_leaf){
			rapidjson::Value sub_value{rapidjson::kObjectType};
			maker.make(sub_entry, sub_value, allocator);
			_doc.AddMember(rapidjson::StringRef(key.c_str()), sub_value.Move(), allocator);
		}else {
			rapidjson::Value v{value_type};
			if (value_type == rapidjson::kObjectType) {
				PyTypeObject* sub_entry_type = sub_entry.ptr()->ob_type;
				auto it = lambda_map.find(sub_entry_type);
				assert(it!=lambda_map.end());
				it->second(sub_entry.ptr(), v);
				_doc.AddMember(rapidjson::StringRef(key.c_str()), v.Move(), allocator);
			} else if (value_type == rapidjson::kArrayType) {
				for(auto& item : sub_entry) {
					PyTypeObject* item_type = item.ptr()->ob_type;
					rapidjson::Value item_v;
					auto it = lambda_map.find(item_type);
					assert(it!=lambda_map.end());
					it->second(item.ptr(), item_v);
					v.PushBack(item_v.Move(), allocator);
				}
				_doc.AddMember(rapidjson::StringRef(key.c_str()), v.Move(), allocator);

			}
		}
	};
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	_doc.Accept(writer);
	return  buffer.GetString();
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
		const std::string& object_type = py::str(tuple[3]);

		auto it = str2rapidjsonType.find(object_type);
		if (it == str2rapidjsonType.end()){
			 throw std::exception();
		}
		int key_id = keyCollection.size();
		keyCollection.push_back(key);

		maker_input.push_back(std::make_tuple(key_id, maker_name, py::reinterpret_borrow<py::object>(getter), it->second));
    }
	makerCollection.emplace_back(cls_name, JsonMaker{maker_input});
	maker_name_idx_map.emplace(cls_name, makerCollection.size() - 1);

}

std::string JsonMakerCaller::make(const py::object& entry) const {
	auto i = maker_name_idx_map[cls_name];
	return makerCollection[i].second.make(entry);
} ;


}
