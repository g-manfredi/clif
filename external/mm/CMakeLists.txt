cmake_minimum_required (VERSION 2.8.8)
project(MetaMat)
string(TOUPPER ${PROJECT_NAME} PNU)
string(TOLOWER ${PROJECT_NAME} PNL)

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
dep_lists_cleanup()

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

find_package(Boost COMPONENTS filesystem system)
message("boost libs: ${Boost_LIBRARIES}")

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

set(HEADERS mat.hpp basetype.hpp)
# FIXME export

#export helper
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
include(CMakePackageConfigListHelpers)


#####################################################
## ...Config.cmake generation
#####################################################

set(CMAKECONFIG_PKG ${${PNU}_EXPORT_DEPS})
set(CMAKECONFIG_PKG_INC ${${PNU}_PKG_INC})
set(CMAKECONFIG_PKG_LINK ${${PNU}_PKG_LINK})
set(CMAKECONFIG_PKG_LIB ${${PNU}_PKG_LIB})

set(CMAKECONFIG_INC "include") #in build dir - headers were already copied earlier
list(APPEND CMAKECONFIG_INC ${CLIF_INSTALL_INCLUDE_DIR} ${CLIF_INCLUDE_DIRS}) # FIXME remove this!

set(CMAKECONFIG_LIB ${${PNU}_EXPORT_LIBS}) # our libs to link on import
list(APPEND CMAKECONFIG_LIB ${${PNU}_INSTALL_LIBRARIES}) # FIXME remove this! (clif libraries)
list(APPEND CMAKECONFIG_LIB ${${PNU}_LINK_LIBRARIES}) # FIXME remove this! (direct dependecies)


set(CMAKECONFIG_LINK "lib")
list(APPEND CMAKECONFIG_LINK ${${PNU}_INSTALL_LIBRARY_DIR}) # FIXME remove this!

#####################################################
## local config.cmake
#####################################################

#copy our own FindXXX.cmake macros
file(COPY cmake/find DESTINATION ./)

set(CMAKECONFIG_CMAKE_DIR ${CMAKE_CURRENT_BINARY_DIR})

#set(CMAKECONFIG_LINK ${CMAKE_CURRENT_BINARY_DIR}/lib)

set(INSTALL_PREFIX_BACKUP ${CMAKE_INSTALL_PREFIX})
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR})
configure_package_config_file(cmake/projectConfig.cmake.in
                              "${CMAKECONFIG_CMAKE_DIR}/${PROJECT_NAME}Config.cmake"
                              INSTALL_DESTINATION "${CMAKECONFIG_CMAKE_DIR}"
                              PATH_VARS CMAKECONFIG_PKG CMAKECONFIG_PKG_INC CMAKECONFIG_PKG_LINK CMAKECONFIG_PKG_LIB CMAKECONFIG_INC CMAKECONFIG_LINK CMAKECONFIG_LIB)
set(CMAKE_INSTALL_PREFIX ${INSTALL_PREFIX_BACKUP})

#export into cmakes local package cache - should be directly visible to cmake of local user
export(PACKAGE ${PROJECT_NAME})



