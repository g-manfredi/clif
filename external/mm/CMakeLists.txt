cmake_minimum_required (VERSION 2.8.8)
project(MetaMat)
string(TOUPPER ${PROJECT_NAME} PNU)

# FIXME handle common includes
include(CMakePackageConfigListHelpers)
include(FlexDepLists)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

# FIXME add Debug, etc. on win!
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
	string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endforeach()

set(${PNU}_BUILD_TYPE "SHARED")
if (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
  # using Clang
  list(APPEND CMAKE_CXX_FLAGS "-std=c++11")
  #FIXME
  add_definitions(-DFNM_EXTMATCH=0)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
  # using GCC
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3 -fno-omit-frame-pointer -fPIC -std=c++11 -march=native")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
  # using Visual Studio C++
  set(${PNU}_BUILD_TYPE "STATIC")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /noOy /W2 /EHsc")
endif()

if (${PNU}_BUILD_TYPE STREQUAL "STATIC")
  set(BUILD_SHARED_LIBS OFF)
else()
  set(BUILD_SHARED_LIBS ON)
endif()

#####################################################
## DEPENDENCIES
#####################################################
dep_lists_init()

dep_lists_append(Eigen3 OPTIONAL EIGEN3_INCLUDE_DIR)
dep_lists_append(OpenCV OpenCV_INCLUDE_DIRS "" OpenCV_LIBS)
dep_lists_append(Eigen3 OPTIONAL)

# FIXME use global static/shared switch boost
if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
	set(Boost_USE_STATIC_LIBS ON)
endif()
# FIXME implement components: COMPONENTS filesystem system
dep_lists_append(Boost Boost_INCLUDE_DIRS Boost_LIBRARY_DIRS Boost_LIBRARIES COMPONENTS filesystem system)

set(VIGRAIMPEX_VAR vigraimpex)
dep_lists_append(Vigra Vigra_INCLUDE_DIRS "" VIGRAIMPEX_VAR)

dep_lists_pkg_search()
dep_lists_prepare_env()

######################

include_directories(${CMAKE_CURRENT_BINARY_DIR})


#####################################################
## COMPILE
#####################################################
# headers are included so they form a dependency for file(COPY later)
add_library(metamat mat.cpp basetype.cpp)
target_link_libraries(metamat PUBLIC ${${PNU}_LINK_LIBRARIES} ${${PNU}_LIB})
target_link_libraries(metamat PRIVATE ${${PNU}_PRIVATE_LINK_LIBRARIES} ${${PNU}_PRIVATE_LIB})

set(${PNU}_EXPORT_LIBS metamat)

#####################################################
## EXPORT
#####################################################

#copy headers and generate XXXConfig.cmake
set(${PNU}_HEADERS mat.hpp basetype.hpp)
dep_lists_export_local()



