cmake_minimum_required (VERSION 3.1)
project(clif)

cmake_policy(VERSION 3.1)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  add_definitions(-DCLIF_COMPILER_MSVC)
  set(CLIF_COMPILER_MSVC true)
  set(CLIF_EXPORT_STRING "__declspec(dllexport)")
  add_definitions(-DCLIF_EXPORT=${CLIF_EXPORT_STRING})
else() # "Clang" "GNU" "Intel" ...
  add_definitions(-DCLIF_EXPORT=)
endif()

message( "COMPILE_DEFINITIONS = ${DirDefs}" )

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -g -O3 -fno-omit-frame-pointer -std=c++11 -Wall -march=native")

find_package(OpenCV)
if (NOT OpenCV_FOUND)
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
  message(WARNING "WARNING: using internal cmake find macros!")
endif()

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
link_directories(${OpenCV_LIB_DIR})

#FIXME check wether actual built is static?
if (CLIF_COMPILER_MSVC)
  set(HDF5_USE_STATIC_LIBRARIES true)
endif()
find_package(HDF5 REQUIRED C CXX)
find_package(HDF5 REQUIRED)
include_directories(${HDF5_INCLUDE_DIR})
link_directories(${HDF5_LIBRARY_DIR})


find_package(Vigra REQUIRED)
include_directories(${Vigra_INCLUDE_DIRS})
#link_directories(${Vigra?})

find_package(Boost REQUIRED COMPONENTS filesystem system)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

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