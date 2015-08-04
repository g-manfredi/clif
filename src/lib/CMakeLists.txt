cmake_minimum_required (VERSION 2.6)
project(clif)

find_package(HDF5 REQUIRED CXX)

add_library(clif SHARED clif.cpp)

target_link_libraries(clif ${HDF5_CXX_LIBRARIES} ${OPENCV_LIBRARIES})
