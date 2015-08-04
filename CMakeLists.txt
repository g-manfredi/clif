cmake_minimum_required (VERSION 2.6)
project(clif)

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -g -std=c++11")

find_package(OpenCV REQUIRED)

add_subdirectory(external/cliini)
include_directories(external/cliini)

add_subdirectory(src/lib)
include_directories(src/lib)

add_subdirectory(src/bin)