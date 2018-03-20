#include "cerpypy.h"
#include <python2.7/Python.h>

namespace py = pybind11;

namespace cerpypy {


static std::vector<py::object> keyCollection{};

static std::vector<std::pair<std::string, PyObjMaker>> makerCollection{};

static std::map<std::string, size_t> maker_name_idx_map{};

PyObjMaker::PyObjMaker(const std::vector<std::tuple<int,
		std::string,
		py::object,
		PyTypeObject*>>& input) {

	// _obj = py::reinterpret_borrow<py::object>(PyDict_New());
	// register string
	int key_id;
	std::string maker_name;
	py::object getter;
	PyTypeObject* type;

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
			auto jm = PyObjMaker();
			jm.is_leaf = true;
			makerCollection.emplace_back(maker_name, std::move(jm));
			maker_name_idx_map.emplace(maker_name, maker_id);
		}
		_makers.emplace_back(key_id, maker_id, py::reinterpret_borrow<py::object>(getter), type);
	}
}

void PyObjMaker::make(const py::handle& entry, py::handle& output) const {


	for(const auto& p : _makers) {
		const py::object& key = keyCollection[std::get<0>(p)];

		const auto& maker = makerCollection[std::get<1>(p)].second;
		const auto& sub_entry = std::get<2>(p)(entry);
		auto value_type = std::get<3>(p);

		if (!maker.is_leaf){
			if (value_type == &PyDict_Type) {
				py::dict sub_value;
				maker.make(sub_entry, sub_value);
				output[key] =  py::reinterpret_borrow<py::object>(sub_value);
			}else if (value_type == &PyDict_Type) {
				std::cout << "array type" << std::endl;
			}
		}else{
			if (value_type == &PyDict_Type) {
				//PyTypeObject* sub_entry_type = sub_entry.ptr()->ob_type;
				//rapidjson::Value v{rapidjson::kObjectType};
				//auto it = lambda_map.find(sub_entry_type);
				//assert(it!=lambda_map.end());
				//it->second(sub_entry.ptr(), v);
				output[key] = sub_entry;
			}else if (value_type == &PyList_Type) {
				size_t size = PyList_Size(sub_entry.ptr());
				py::list sub_value{size};
				for(auto& item : sub_entry) {
					//PyTypeObject* item_type = item.ptr()->ob_type;
					//rapidjson::Value item_v;
					//auto it = lambda_map.find(item_type);
					//assert(it!=lambda_map.end());
					//it->second(item.ptr(), item_v);
					sub_value.append(py::reinterpret_borrow<py::object>(item));
				}
				output[key] = py::reinterpret_borrow<py::object>(sub_value);
			}

		}
	};
};

py::dict PyObjMaker::make(const py::handle& entry) {
	/*
	 * Need to handle this case
	if (is_leaf) {
		std::cout << "is_node" << std::endl;
	}
	 */
	_obj.clear();

	for(const auto& p : _makers) {
		const py::object& key = keyCollection[std::get<0>(p)];
		const auto& maker = makerCollection[std::get<1>(p)].second;
		const auto& sub_entry = std::get<2>(p)(entry);
		auto value_type = std::get<3>(p);

		if (!maker.is_leaf){
			if (value_type == &PyDict_Type) {
				py::dict sub_value;
				maker.make(sub_entry, sub_value);
				_obj[key] = sub_value;
			}else if (value_type == &PyList_Type) {
				py::list sub_value;
				for(const auto& item : sub_entry) {
					py::dict item_v;
					maker.make(item, item_v);
					sub_value.append(py::reinterpret_borrow<py::object>(item_v));
				}
				_obj[key] = py::reinterpret_borrow<py::object>(sub_value);
			}
		}else {
			if (value_type == &PyDict_Type) {
				//PyTypeObject* sub_entry_type = sub_entry.ptr()->ob_type;
				//auto it = lambda_map.find(sub_entry_type);
				//assert(it!=lambda_map.end());
				//it->second(sub_entry.ptr(), v);
				_obj[key] = sub_entry;
			} else if (value_type == &PyList_Type) {
				size_t size = PyList_Size(sub_entry.ptr());
				py::list sub_value{size};
				for(auto& item : sub_entry) {
					//PyTypeObject* item_type = item.ptr()->ob_type;
					//rapidjson::Value item_v;
					//auto it = lambda_map.find(item_type);
					//assert(it!=lambda_map.end());
					//it->second(item.ptr(), item_v);
					sub_value.append(py::reinterpret_borrow<py::object>(item));
				}
				_obj[key] = py::reinterpret_borrow<py::object>(sub_value);
			}
		}
	};

	return  _obj;
}
const static std::map<std::string,  PyTypeObject*> str2pythonType = {{"object", &PyDict_Type},
		{"array", &PyList_Type}};


void register_pyobj_maker(const std::string& cls_name, const py::list& input) {
	std::vector<std::tuple<int,
	std::string,
	py::object,
	PyTypeObject*>> maker_input;

	for (const auto& item : input){
		py::tuple tuple = py::reinterpret_borrow<py::tuple>(item);

		const std::string& maker_name = py::str(tuple[1]);
		const py::object& getter = tuple[2];
		const std::string& object_type = py::str(tuple[3]);

		auto it = str2pythonType.find(object_type);
		if (it == str2pythonType.end()){
			throw std::exception();
		}
		int key_id = keyCollection.size();
		keyCollection.push_back(py::reinterpret_borrow<py::object>(tuple[0]));

		maker_input.push_back(std::make_tuple(key_id, maker_name, py::reinterpret_borrow<py::object>(getter), it->second));
	}
	makerCollection.emplace_back(cls_name, PyObjMaker{maker_input});
	maker_name_idx_map.emplace(cls_name, makerCollection.size() - 1);
}

PyObjMakerCaller::PyObjMakerCaller(const std::string& cls_name): cls_name(cls_name){
}


py::object PyObjMakerCaller::make(const py::object& entry) const {
	int i = maker_name_idx_map[cls_name];
	return makerCollection[i].second.make(entry);
} ;


}
