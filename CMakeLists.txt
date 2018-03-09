cmake_minimum_required(VERSION 2.8.12)
project(cerpypy)

add_subdirectory(pybind11)
pybind11_add_module(cerpypy cerpypy_ext.cpp)
