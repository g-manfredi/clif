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
set(ExtrType     "INVALID,LINE,CIRCLE")
set(CalibPattern "INVALID,CHECKERBOARD,HDMARKER")
set(DistModel    "INVALID,CV8,UCALIB")

createEnumStrings(DataType)
createEnumStrings(DataOrg)
createEnumStrings(DataOrder)
createEnumStrings(ExtrType)
createEnumStrings(CalibPattern)
createEnumStrings(DistModel)

file(READ "${CMAKE_SOURCE_DIR}/data/types.ini" CLIF_DEFAULT_TYPES_BUF)
string(REPLACE "\n" "\\n" CLIF_DEFAULT_TYPES_BUF "${CLIF_DEFAULT_TYPES_BUF}")
configure_file(types.cpp.in types.cpp)

configure_file(enumtypes.hpp.in enumtypes.hpp)

list(APPEND CLIF_BUILD_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
list(APPEND LIBCLIF_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/enumtypes.hpp)
set(CLIF_BUILD_INCLUDE_DIRS ${CLIF_BUILD_INCLUDE_DIRS} PARENT_SCOPE)
include_directories(${CLIF_BUILD_INCLUDE_DIRS})

set(SOURCES enumtypes.cpp clif.cpp subset3d.cpp calib.cpp hdf5.cpp attribute.cpp helpers.cpp datastore.cpp dataset.cpp core.cpp clif_cv.cpp mat.cpp preproc.cpp tree_derived.cpp cam.cpp ${CMAKE_CURRENT_BINARY_DIR}/types.cpp mat_helpers.cpp)

if (CLIF_WITH_LIBIGL)
  list(APPEND SOURCES mesh.cpp)
  list(APPEND LIBCLIF_HEADERS mesh.hpp)
endif()

get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "dir='${dir}'")
endforeach()


add_library(clif ${CLIF_BUILD_TYPE} ${SOURCES})

target_link_libraries(clif PUBLIC ${CLIF_LINK_LIBRARIES} ${${PNU}_LIB})
#message("link public: ${CLIF_LINK_LIBRARIES} ${${PNU}_LIB}")
target_link_libraries(clif PRIVATE ${CLIF_PRIVATE_LINK_LIBRARIES} ${${PNU}_PRIVATE_LIB})
#message("link private: ${CLIF_PRIVATE_LINK_LIBRARIES} ${${PNU}_PRIVATE_LIB}")

message("oublic link interface: ${INTERFACE_LINK_LIBRARIES}")


set(CLIF_LINK_LIBRARIES ${CLIF_LINK_LIBRARIES} clif PARENT_SCOPE)
set(CLIF_BUILD_INCLUDE_DIRS ${CLIF_BUILD_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
set(CLIF_BUILD_LINK_DIRS ${CLIF_BUILD_LINK_DIRS} ${CMAKE_CURRENT_BINARY_DIR} PARENT_SCOPE)

list(APPEND LIBCLIF_HEADERS attribute.hpp calib.hpp clif_cv.hpp clif.hpp core.hpp dataset.hpp datastore.hpp hdf5.hpp helpers.hpp matio.hpp stringtree.hpp subset3d.hpp mat.hpp types.hpp preproc.hpp tree_derived.hpp cam.hpp mat_helpers.hpp)

foreach(header ${LIBCLIF_HEADERS})
  if (IS_ABSOLUTE ${header})
    list(APPEND CLIF_FULLPATH_HEADERS "${header}")
  else()
    list(APPEND CLIF_FULLPATH_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/${header}")
  endif()
endforeach()

set(CLIF_HEADERS "${CLIF_HEADERS}" PARENT_SCOPE)
set(CLIF_FULLPATH_HEADERS "${CLIF_FULLPATH_HEADERS}" PARENT_SCOPE)

install(TARGETS clif
        EXPORT clif
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

        
