cmake_minimum_required (VERSION 2.6)
project(clif)

find_package(HDF5 REQUIRED CXX)

find_package(PkgConfig REQUIRED)
#TODO make optional
pkg_search_module(MATIO REQUIRED matio)

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -g -std=c++11 -Wall")

add_library(clif SHARED clif.cpp matio.cpp clif3dsubset.cpp)

function(createEnumStrings ENUMSTR)
  set(${ENUMSTR}Enums ${${ENUMSTR}} PARENT_SCOPE)
  set(${ENUMSTR}Strs "\"${${ENUMSTR}}\"")
  string(REPLACE "," "\",\"" ${ENUMSTR}Strs ${${ENUMSTR}Strs})
  set(${ENUMSTR}Strs "${${ENUMSTR}Strs},NULL" PARENT_SCOPE)
endfunction(createEnumStrings)

set(DataType "UINT8,UINT16")
set(DataOrg "PLANAR,INTERLEAVED,BAYER_2x2")
set(DataOrder "RGGB,BGGR,GBRG,GRBG,RGB")
set(ExtrType "LINE")

createEnumStrings(DataType)
createEnumStrings(DataOrg)
createEnumStrings(DataOrder)
createEnumStrings(ExtrType)

configure_file(enumtypes.hpp.in enumtypes.hpp)

target_link_libraries(clif ${HDF5_CXX_LIBRARIES} ${OPENCV_LIBRARIES} ${MATIO_LIBRARIES})
