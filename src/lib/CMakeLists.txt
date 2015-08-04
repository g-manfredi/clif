cmake_minimum_required (VERSION 2.6)
project(clif)

find_package(HDF5 REQUIRED CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -g -std=c++11")

add_library(clif SHARED clif.cpp)

set(DataTypeEnums "UINT8,UINT16")
set(DataOrgEnums "BAYER_2x2")
set(DataOrderEnums "RGGB,BGGR,GBRG,GRBG,RGB")


set(DataTypeStrs "\"${DataTypeEnums}\"")
string(REPLACE "," "\",\"" DataTypeStrs ${DataTypeStrs})
set(DataOrgStrs "\"${DataOrgEnums}\"")
string(REPLACE "," "\",\"" DataOrgStrs ${DataOrgStrs})
set(DataOrderStrs "\"${DataOrderEnums}\"")
string(REPLACE "," "\",\"" DataOrderStrs ${DataOrderStrs})


configure_file(enumtypes.hpp.in enumtypes.hpp)

target_link_libraries(clif ${HDF5_CXX_LIBRARIES} ${OPENCV_LIBRARIES})
