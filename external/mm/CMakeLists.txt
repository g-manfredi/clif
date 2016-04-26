cmake_minimum_required (VERSION 2.8.8)
project(MetaMat)

#####################################################
## INIT
#####################################################
include(../flexdeplists/FlexDepLists.cmake)
dep_lists_init()

#####################################################
## ENVIRONMENT
#####################################################
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
  list(APPEND CMAKE_CXX_FLAGS "-std=c++11")
  # FIXME
  add_definitions(-DFNM_EXTMATCH=0)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3 -fno-omit-frame-pointer -fPIC -std=c++11 -march=native")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /noOy /W2 /EHsc")
endif()

# default to static on win shared on others
if (NOT DEFINED BUILD_SHARED_LIBS)
  if (WIN32)
    option(BUILD_SHARED_LIBS "build shared libraries" OFF)
  else()
    option(BUILD_SHARED_LIBS "build shared libraries" ON)
  endif()
endif()

#####################################################
## DEPENDENCIES
#####################################################
dep_lists_append(OpenCV OpenCV_INCLUDE_DIRS "" OpenCV_LIBS)
if (WIN32)
  # global FindHDF5.cmake gives wrong paths...
  dep_lists_append(HDF5 COMPONENTS C CXX FIND_FLAGS CONFIG)
else()
  dep_lists_append(HDF5 COMPONENTS C CXX FIND_FLAGS)
endif()
dep_lists_append(Boost Boost_INCLUDE_DIRS Boost_LIBRARY_DIRS Boost_LIBRARIES COMPONENTS filesystem system)

# add variable which stores vigra library
set(VIGRAIMPEX_VAR vigraimpex)
list(APPEND ${PNU}_EXPORT_VARS "VIGRAIMPEX_VAR")
dep_lists_append(Vigra Vigra_INCLUDE_DIRS "" VIGRAIMPEX_VAR)

dep_lists_pkg_search()

set(${PNU}_HEADERS mat.hpp basetype.hpp)
dep_lists_prepare_env()

#####################################################
## COMPILE
#####################################################
add_library(metamat mat.cpp basetype.cpp)
target_link_libraries(metamat PUBLIC ${${PNU}_LINK_LIBRARIES} ${${PNU}_LIB})
target_link_libraries(metamat PRIVATE ${${PNU}_PRIVATE_LINK_LIBRARIES} ${${PNU}_PRIVATE_LIB})
set(${PNU}_EXPORT_LIBS metamat)

#####################################################
## INSTALL
#####################################################
dep_lists_export_local()



