cmake_minimum_required (VERSION 2.8.8)
project(clif-meta)
string(TOUPPER ${PROJECT_NAME} PNU)

#####################################################
## OPTIONS
#####################################################

option(GPL_BUILD "create GPL'ed binaries" OFF)

# default to static on win shared on others
if (NOT DEFINED BUILD_SHARED_LIBS)
  if (WIN32)
    option(BUILD_SHARED_LIBS "build shared libraries" OFF)
  else()
    option(BUILD_SHARED_LIBS "build shared libraries" ON)
  endif()
endif()

#####################################################
## GLOBAL
#####################################################

include(external/flexdeplists/FlexDepLists.cmake)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

#####################################################
## COMPILER
#####################################################

if (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
  list(APPEND CMAKE_CXX_FLAGS "-std=c++11")
  # FIXME add to _DEFINES?
  add_definitions(-DFNM_EXTMATCH=0)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3 -fno-omit-frame-pointer -fPIC -std=c++11 -march=native")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /noOy /W2 /EHsc")
endif()
  
  
#####################################################
## SUB-PROJECTS
#####################################################
  
if (NOT DEFINED FNMATCH_USE_INTERNAL)
  find_path(FNMATCH_INCLUDE_DIR NAMES fnmatch.h)
  if (NOT FNMATCH_INCLUDE_DIR)
    set(FNMATCH_USE_INTERNAL TRUE CACHE BOOL "use internal fnmatch code")
  else()
    set(FNMATCH_USE_INTERNAL FALSE CACHE BOOL "use internal fnmatch code")
  endif()
endif()

if (FNMATCH_USE_INTERNAL)
  add_subdirectory(external/fnmatch)
endif()

add_subdirectory(external/cliini)
add_subdirectory(external/mm)

# TODO optional subproject handling
set(DEP_LISTS_SOFT_FAIL true)

add_subdirectory(external/mm-mesh)
add_subdirectory(external/ucalib)

set(DEP_LISTS_SOFT_FAIL false)

######################

add_subdirectory(src/lib)
add_subdirectory(src/bin)

set(DEP_LISTS_SOFT_FAIL true)

add_subdirectory(src/gui)
add_subdirectory(src/clifview)

set(DEP_LISTS_SOFT_FAIL false)
