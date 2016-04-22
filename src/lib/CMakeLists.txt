cmake_minimum_required (VERSION 2.6)
project(clif)

string(TOUPPER ${PROJECT_NAME} PNU)

#CMP0020 - 2.8.11, CMP0022 - 2.8.12
cmake_policy(VERSION 2.8.12)

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

include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include/clif)

configure_file(enumtypes.hpp.in enumtypes.hpp)

foreach(define ${${PNU}_DEFINES})
  set(${PNU}_DEFINES_STRING "${${PNU}_DEFINES_STRING}\n#define ${define}")
endforeach()

# FIXME move to prepare env?
if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
  # FIXME use CLIF_DEFINES
  add_definitions(-DCLIF_COMPILER_MSVC)
  add_definitions(-DCLIF_EXPORT_LIBRARY)
  add_definitions(-DCLIF_BUILD_STATIC)
  set(CLIF_COMPILER_MSVC true)
  set(CLIF_EXPORT "__declspec(dllexport)")
  set(CLIF_IMPORT "__declspec(dllimport)")
  set(CMAKE_DEBUG_POSTFIX "d")
endif()

configure_file(config.hpp.in config.hpp)

file(GLOB ${PNU}_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)
list(APPEND ${PNU}_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/enumtypes.hpp)
list(APPEND ${PNU}_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/config.hpp)

dep_lists_init()

if (FNMATCH_USE_INTERNAL)
  dep_lists_append(fnmatch)
endif()

dep_lists_append(OpenCV OpenCV_INCLUDE_DIRS "" OpenCV_LIBS)
dep_lists_append(MetaMat)
dep_lists_append(cliini)

dep_lists_append(hdmarker OPTIONAL)
dep_lists_append(ucalib OPTIONAL)

dep_lists_pkg_search()
dep_lists_prepare_env()

set(SOURCES enumtypes.cpp clif.cpp subset3d.cpp calib.cpp hdf5.cpp attribute.cpp helpers.cpp datastore.cpp dataset.cpp core.cpp clif_cv.cpp preproc.cpp tree_derived.cpp cam.cpp ${CMAKE_CURRENT_BINARY_DIR}/types.cpp mat_helpers.cpp)

add_library(clif ${SOURCES})

target_link_libraries(clif PUBLIC ${${PNU}_LIB})
target_link_libraries(clif PRIVATE ${${PNU}_PRIVATE_LIB})

set(${PNU}_EXPORT_LIBS clif)

dep_lists_export_local()
        
message("build info: for ${PROJECT_NAME}:")
message("  intern/extern:")
msg_yesno("    internal fnmatch" FNMATCH_USE_INTERNAL COLOR Green)
message("  optional components:")
msg_yesno("    hdmarker detection" CLIF_WITH_HDMARKER)
msg_yesno("    ucalib calibration" CLIF_WITH_UCALIB)

message("")

