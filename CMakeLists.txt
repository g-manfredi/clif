cmake_minimum_required (VERSION 3.1)
project(clif)

cmake_policy(VERSION 3.1)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  # using Clang
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  # using GCC
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  # using Intel C++
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  add_definitions(-DCLIF_COMPILER_MSVC)
  set(CLIF_EXPORT_STRING "__declspec(dllexport)")
  add_definitions(-DCLIF_EXPORT=${CLIF_EXPORT_STRING})
message(DEBUG "${COMPILE_DEFINITIONS}")
message(DEBUG "using msvc compiler")

get_directory_property( DirDefs DIRECTORY ${CMAKE_SOURCE_DIR} COMPILE_DEFINITIONS )
foreach( d ${DirDefs} )
    message( STATUS "Found Define: " ${d} )
endforeach()
message( STATUS "DirDefs: " ${DirDefs} )

endif()
message(DEBUG "compiler?")

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -g -O3 -fno-omit-frame-pointer -std=c++11 -Wall -march=native")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MT")

find_package(OpenCV)
if (NOT OpenCV_FOUND)
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
  message(WARNING "WARNING: using internal cmake find macros!")
endif()

find_package(OpenCV REQUIRED)
#include_directories(${OpenCV_DIR}/../../../include)
#link_directories(${OpenCV_LIB_DIR})


include_directories(${OpenCV_INCLUDE_DIRS})
link_directories(${OpenCV_LIB_DIR})

message(DEBUG "cv: inc: ${OpenCV_INCLUDE_DIRS}")
message(DEBUG "cv: lib: ${OpenCV_LIB_DIR}")

set(HDF5_USE_STATIC_LIBRARIES true)
add_definitions(-DHDF5_USE_STATIC_LIBRARIES)
find_package(HDF5 REQUIRED C CXX)
find_package(HDF5 REQUIRED)


message(DEBUG "hdf5 libs ${HDF5_CXX_LIBRARIES}")
message(DEBUG "hdf5 libs ${HDF5_C_LIBRARIES}")
message(DEBUG "hdf5 libs ${HDF5_LIBRARIES}")

include_directories(${HDF5_INCLUDE_DIR})
link_directories(${HDF5_LIBRARY_DIR})

message(DEBUG "hdf inc: ${HDF5_INCLUDE_DIR}")


#message(DEBUG "cv inc: ${OpenCV_DIR}/../../../include")
#OpenCV_INCLUDE_DIRS contains crap for some reason 
#message(DEBUG "cv libs: ${OpenCV_LIB_DIR}")

find_package(Vigra REQUIRED)
include_directories(${Vigra_INCLUDE_DIRS})
#link_directories(${Vigra?})

#set(CLIF_LINK_LIBRARIES ${CLIF_LINK_LIBRARIES} vigraimpex)

#include_directories(${Boost_INCLUDE_DIRS})
#link_directories(${Boost_LIBRARY_DIRS})
#TODO ask Ullrich: VigraConfig.cmake misses Vigra_LIBRARIES! 

find_package(Boost REQUIRED COMPONENTS filesystem system)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

message(DEBUG "boost libdir: ${Boost_LIBRARY_DIRS} - ${Boost_LIBRARY_DIR}")

find_path(FNMATCH_INCLUDE_DIR NAMES fnmatch.h)
if (NOT FNMATCH_INCLUDE_DIR)
  set(FNMATCH_USE_INTERNAL TRUE)
  add_subdirectory(external/fnmatch)
  include_directories(external/fnmatch)
  link_directories(external/fnmatch)
  set(CLIF_FNMATCH_LIBRARIES ${CLIF_FNMATCH_LIBRARIES} fnmatch)
  set(CLIF_LINK_LIBRARIES ${CLIF_LINK_LIBRARIES} ${CLIF_FNMATCH_LIBRARIES})
endif()

add_subdirectory(external/cliini)
include_directories(external/cliini)

add_subdirectory(src/lib)
include_directories(src/lib)

add_subdirectory(src/bin)
add_subdirectory(src/gui)

message("")
message("build info:")
message("  optional components:")

if(CLIF_BUILD_QT)
  message("    qt-widgets   -   yes")
else()
  message("    qt-widgets   -   no - ${CLIF_BUILD_QT_REASON}")
endif()

if(FNMATCH_USE_INTERNAL)
  message("    internal fnmatch   -   yes (no fnmatch.h header found)")
else()
  message("    internal fnmatch   -   no")
endif()


message("")