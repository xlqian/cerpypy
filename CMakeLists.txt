cmake_minimum_required(VERSION 2.8.12)
project(cerpypy)

include_directories("./rapidjson/include")

add_subdirectory(pybind11)
pybind11_add_module(cerpypy src/cerpypy_ext.cpp src/cerpypy.cpp src/json_maker.cpp src/pyobj_maker.cpp)
