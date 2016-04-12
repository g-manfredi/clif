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


#####################################################
## DEPENDENCIES
#####################################################
# eigen3
list(APPEND ${PNU}_PKG_OPT Eigen3)
list(APPEND ${PNU}_PKG_INC EIGEN3_INCLUDE_DIR)

#vigra
#FIXME ask Ullrich why theres no VIGRA_LIBRARIES?
list(APPEND ${PNU}_PKG Vigra)
list(APPEND ${PNU}_PKG_INC Vigra_INCLUDE_DIRS)
set(VIGRAIMPEX_VAR vigraimpex)
list(APPEND ${PNU}_PKG_LIB VIGRAIMPEX_VAR)

#opencv
list(APPEND ${PNU}_PKG OpenCV)
list(APPEND ${PNU}_PKG_INC OpenCV_INCLUDE_DIRS)
list(APPEND ${PNU}_PKG_LIB OpenCV_LIBS)

#boost
if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
	set(Boost_USE_STATIC_LIBS ON)
endif()
find_package(Boost REQUIRED COMPONENTS filesystem system)
list(APPEND ${PNU}_LINK_LIBRARIES ${Boost_LIBRARIES})
list(APPEND ${PNU}_INCLUDE_DIRS ${Boost_INCLUDE_DIRS})
list(APPEND ${PNU}_LIBRARY_DIRS ${Boost_LIBRARY_DIRS})

dep_lists_pkg_search(${PNU})
dep_lists_inc_link(${PNU})

######################

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
add_custom_command(TARGET metamat PRE_BUILD
                     COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/include/mm/)
foreach(H ${HEADERS})
  add_custom_command(TARGET metamat PRE_BUILD
                     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${H} ${CMAKE_CURRENT_BINARY_DIR}/include/mm/)
endforeach()
add_dependencies(metamat ${CMAKE_CURRENT_SOURCE_DIR}/mat.hpp ${CMAKE_CURRENT_SOURCE_DIR}/basetype.hpp)

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



