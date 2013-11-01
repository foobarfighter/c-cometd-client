cmake_minimum_required(VERSION 2.6)
project(libcometd)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(StaticDeps)
find_package(GLIB2)
find_package(JsonGlib)

set(CMAKE_CXX_FLAGS "-g -Wall")
add_subdirectory(deps)
add_subdirectory(src)
