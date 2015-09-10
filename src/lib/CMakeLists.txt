cmake_minimum_required (VERSION 2.6)
project(clif)

find_package(HDF5 REQUIRED CXX)

find_package(Boost REQUIRED COMPONENTS filesystem system)

find_package(PkgConfig REQUIRED)
#TODO make optional
pkg_search_module(MATIO REQUIRED matio)

#set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -g -O3 -std=c++11 -Wall")


function(createEnumStrings ENUMSTR)
  set(${ENUMSTR}Enums ${${ENUMSTR}} PARENT_SCOPE)
  set(${ENUMSTR}Strs "\"${${ENUMSTR}}\"")
  string(REPLACE "," "\",\"" ${ENUMSTR}Strs ${${ENUMSTR}Strs})
  set(${ENUMSTR}Strs "${${ENUMSTR}Strs},NULL" PARENT_SCOPE)
endfunction(createEnumStrings)

set(DataType     "INVALID,UINT8,UINT16")
set(DataOrg      "INVALID,PLANAR,INTERLEAVED,BAYER_2x2")
set(DataOrder    "INVALID,RGGB,BGGR,GBRG,GRBG,RGB")
set(ExtrType     "INVALID,LINE")
set(CalibPattern "INVALID,CHECKERBOARD,HDMARKER")
set(DistModel    "INVALID,CV8,LINES")

createEnumStrings(DataType)
createEnumStrings(DataOrg)
createEnumStrings(DataOrder)
createEnumStrings(ExtrType)
createEnumStrings(CalibPattern)
createEnumStrings(DistModel)

configure_file(enumtypes.hpp.in enumtypes.hpp)

add_library(clif SHARED clif.cpp matio.cpp subset3d.cpp clifcalib.cpp)
#add_library(clifcalib SHARED clifcalib.cpp)

target_link_libraries(clif ${HDF5_CXX_LIBRARIES} ${Boost_LIBRARIES} ${OPENCV_LIBRARIES} ${MATIO_LIBRARIES})
#target_link_libraries(clifcalib clif)
