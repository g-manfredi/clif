cmake_minimum_required (VERSION 2.6)
project(cliini)

include(../flexdeplists/FlexDepLists.cmake)
dep_lists_init()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  add_definitions(-D${PNU}_COMPILER_MSVC)
  add_definitions(-D${PNU}_EXPORT_LIBRARY)
  add_definitions(-D${PNU}_BUILD_STATIC)
  set(${PNU}_COMPILER_MSVC true)
  set(${PNU}_BUILD_STATIC true)
  set(${PNU}_BUILD_TYPE)
  # FIXME shouldn't this be an option
  set(BUILD_SHARED_LIBS false)
  # following flags are for shared build
  set(${PNU}_EXPORT "__declspec(dllexport)")
  set(${PNU}_IMPORT "__declspec(dllimport)")
  # we are buillding static!
  #set(${PNU}_SHARED "#define CLIINI_SHARED")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /TP")
else()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -std=c99 -fPIC")
  add_definitions(-D_GNU_SOURCE) #for strdup()
endif()

if (BUILD_SHARED_LIBS)
  add_definitions(-D${BUILD_SHARED_LIBS})
endif()

configure_file(config.h.in cliini_config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if (FNMATCH_USE_INTERNAL)
  dep_lists_append(fnmatch)
endif()

dep_lists_pkg_search()

set(CLIINI_HEADERS cliini.h ${CMAKE_CURRENT_BINARY_DIR}/cliini_config.h)

dep_lists_prepare_env()

add_library(cliini cliini.c)

set(CLIINI_EXPORT_LIBS cliini)

dep_lists_export_local(DIRECT_HEADER_INCLUDE)
