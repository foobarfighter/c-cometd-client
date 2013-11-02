cmake_minimum_required(VERSION 2.6)
project(libcometd)
enable_testing()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(GLIB2)
find_package(JsonGlib)
find_package(CURL)

set(CMAKE_CXX_FLAGS "-g -Wall")
add_subdirectory(deps)
add_subdirectory(src)
add_subdirectory(tests)
