cmake_minimum_required (VERSION 2.8.8)
project(clif-meta)
string(TOUPPER ${PROJECT_NAME} PNU)

#####################################################
## OPTIONS
#####################################################

option(GPL_BUILD "create GPL'ed binaries" OFF)

#####################################################
## GLOBAL
#####################################################

set(CLIF_MINOR_VERSION 1)
set(CLIF_MAJOR_VERSION 0)
set(CLIF_VERSION ${CLIF_MAJOR_VERSION}.${CLIF_MINOR_VERSION})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
include(CMakePackageConfigListHelpers)
include(FlexDepLists)

# FIXME add Debug, etc. on win!
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
	string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endforeach()

#####################################################
## COMPILER
#####################################################

set(${PNU}_BUILD_TYPE "SHARED")

set(${PNU}_BUILD_TYPE "SHARED")
if (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
  # using Clang
  list(APPEND CMAKE_CXX_FLAGS "-std=c++11")
  #FIXME
  add_definitions(-DFNM_EXTMATCH=0)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
  # using GCC
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3 -fno-omit-frame-pointer -fPIC -std=c++11 -march=native -fopenmp")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
  # using Visual Studio C++
  set(${PNU}_BUILD_TYPE "STATIC")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /noOy /W2 /EHsc")
endif()
  
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

if (${PNU}_BUILD_TYPE STREQUAL "STATIC")
  set(BUILD_SHARED_LIBS OFF)
else()
  set(BUILD_SHARED_LIBS ON)
endif()

#####################################################
## COMPILER
#####################################################

dep_lists_init()

# dep_lists_append(OpenCV OpenCV_INCLUDE_DIRS "" OpenCV_LIBS)
# dep_lists_append(MetaMat)
# dep_lists_append(cliini)
# 
# set(VIGRAIMPEX_VAR vigraimpex)
# dep_lists_append(Vigra Vigra_INCLUDE_DIRS VIGRAIMPEX_VAR)
# 
# dep_lists_append(hdmarker OPTIONAL)
# dep_lists_append(mm-mesh OPTIONAL)
# dep_lists_append(ucalib OPTIONAL)
# dep_lists_append(Qt5Widgets OPTIONAL Qt5Widgets_INCLUDE_DIRS Qt5Widgets_LIBRARY_DIRS Qt5Widgets_LIBRARIES)

#####################################################
## SUBPROJECTS
#####################################################

# FIXME how to handle order for packages which are in a subdirectory (fnmatch) and where other packages depend upon (cliini)
# FIXME detailed (package based) dependency handling?

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
  include_directories(external/fnmatch)
  link_directories(external/fnmatch)
endif()

#included external projects (are referenced as package above)
add_subdirectory(external/cliini)
add_subdirectory(external/mm)

# TODO optional subproject handling
set(DEP_LISTS_SOFT_FAIL true)

add_subdirectory(external/mm-mesh)
add_subdirectory(external/ucalib)

set(DEP_LISTS_SOFT_FAIL false)

######################

add_subdirectory(src/lib)
add_subdirectory(src/gui)
add_subdirectory(src/bin)
add_subdirectory(src/clifview)
