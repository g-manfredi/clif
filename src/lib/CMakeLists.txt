cmake_minimum_required (VERSION 2.6)
project(libclif)

function(createEnumStrings ENUMSTR)
  set(${ENUMSTR}Enums ${${ENUMSTR}} PARENT_SCOPE)
  set(${ENUMSTR}Strs "\"${${ENUMSTR}}\"")
  string(REPLACE "," "\",\"" ${ENUMSTR}Strs ${${ENUMSTR}Strs})
  set(${ENUMSTR}Strs "${${ENUMSTR}Strs},NULL" PARENT_SCOPE)
endfunction(createEnumStrings)

set(DataType     "INVALID,UINT8,UINT16")
set(DataOrg      "INVALID,PLANAR,INTERLEAVED,BAYER_2x2")
set(DataOrder    "INVALID,SINGLE,RGGB,BGGR,GBRG,GRBG,RGB")
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
list(APPEND CLIF_BUILD_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR})
list(APPEND LIBCLIF_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/enumtypes.hpp)
set(CLIF_BUILD_INCLUDE_DIRS ${CLIF_BUILD_INCLUDE_DIRS} PARENT_SCOPE)
include_directories(${CLIF_BUILD_INCLUDE_DIRS})

add_library(clif ${CLIF_BUILD_TYPE} clif.cpp subset3d.cpp calib.cpp hdf5.cpp attribute.cpp helpers.cpp datastore.cpp dataset.cpp core.cpp clif_cv.cpp clif_vigra.cpp enumtypes.cpp)
add_library(clif_vigra ${CLIF_BUILD_TYPE} clif_vigra.cpp)

target_link_libraries(clif PUBLIC ${CLIF_LINK_LIBRARIES})
target_link_libraries(clif PRIVATE ${CLIF_PRIVATE_LINK_LIBRARIES})
target_link_libraries(clif_vigra PUBLIC clif)

set(CLIF_LINK_LIBRARIES ${CLIF_LINK_LIBRARIES} clif clif_vigra PARENT_SCOPE)
set(CLIF_BUILD_INCLUDE_DIRS ${CLIF_BUILD_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
set(CLIF_BUILD_LINK_DIRS ${CLIF_BUILD_LINK_DIRS} ${CMAKE_CURRENT_BINARY_DIR} PARENT_SCOPE)

list(APPEND LIBCLIF_HEADERS attribute.hpp calib.hpp clif_cv.hpp clif.hpp clif_vigra.hpp core.hpp dataset.hpp datastore.hpp flexmav.hpp hdf5.hpp helpers.hpp matio.hpp stringtree.hpp subset3d.hpp)

foreach(header ${LIBCLIF_HEADERS})
  if (IS_ABSOLUTE ${header})
    list(APPEND CLIF_FULLPATH_HEADERS "${header}")
  else()
    list(APPEND CLIF_FULLPATH_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/${header}")
  endif()
endforeach()

set(CLIF_HEADERS "${CLIF_HEADERS}" PARENT_SCOPE)
set(CLIF_FULLPATH_HEADERS "${CLIF_FULLPATH_HEADERS}" PARENT_SCOPE)

install(TARGETS clif clif_vigra
        EXPORT clif
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

        
