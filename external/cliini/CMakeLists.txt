cmake_minimum_required (VERSION 2.6)
project(cliini)
string(TOUPPER ${PROJECT_NAME} PNU)

#####################################################
## GLOBALS
#####################################################

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
include(CMakePackageConfigListHelpers)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

####################

if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
  add_definitions(-D${PNU}_COMPILER_MSVC)
  add_definitions(-D${PNU}_EXPORT_LIBRARY)
  add_definitions(-D${PNU}_BUILD_STATIC)
  set(${PNU}_COMPILER_MSVC true)
  set(${PNU}_BUILD_STATIC true)
  set(${PNU}_BUILD_TYPE)
  # FIXME shouldn't this be an option
  set(BUILD_SHARED_LIBS false)
  # following flags are for shared build
  #set(${PNU}_EXPORT "__declspec(dllexport)")
  #set(${PNU}_IMPORT "__declspec(dllimport)")
  #set(${PNU}_SHARED "#define CLIINI_SHARED")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /TP")
else()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -std=c99 -fPIC")
  add_definitions(-D_GNU_SOURCE) #for strdup()
endif()

if (BUILD_SHARED_LIBS)
  add_definitions(-D${BUILD_SHARED_LIBS})
endif()

configure_file(config.h.in include/cliini_config.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)


add_library(cliini cliini.c)
configure_file(cliini.h include/cliini.h COPYONLY)

if (NOT ${PNU}_COMPILER_MSVC)
  add_executable(cliini_test test.c)
  add_executable(cliini_initest testini.c)
  add_executable(cliini_types testini_types.c)
endif()

target_link_libraries(cliini ${${PNU}_LIBRARIES})

#set(${PNU}_LINK_LIBRARIES ${${PNU}_LINK_LIBRARIES} cliini PARENT_SCOPE)
#set(${PNU}_BUILD_INCLUDE_DIRS ${${PNU}_BUILD_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
#set(${PNU}_BUILD_LINK_DIRS ${${PNU}_BUILD_LINK_DIRS} ${CMAKE_CURRENT_BINARY_DIR} PARENT_SCOPE)

#set(${PNU}_HEADERS ${${PNU}_HEADERS} cliini.h PARENT_SCOPE)
#set(${PNU}_FULLPATH_HEADERS ${${PNU}_FULLPATH_HEADERS} ${CMAKE_CURRENT_SOURCE_DIR}/cliini.h PARENT_SCOPE)

#configure_file(cliini.h "include/clif/${_FILE}" COPYONLY)

if (NOT ${PNU}_COMPILER_MSVC)
  target_link_libraries(cliini_test cliini)
  target_link_libraries(cliini_initest cliini)
  target_link_libraries(cliini_types cliini)
endif()

#####################################################
## ...Config.cmake generation
#####################################################

set(CMAKECONFIG_INC "include") #in build dir - headers were already copied earlier
set(CMAKECONFIG_LIB "cliini") # our libs to link on import
set(CMAKECONFIG_LINK "lib") #lib dir
if (${PNU}_COMPILER_MSVC)
  set(CMAKECONFIG_LINK "lib/Release" "lib/Debug") #lib dir
endif()


#####################################################
## local config.cmake
#####################################################

set(CMAKECONFIG_CMAKE_DIR ${CMAKE_CURRENT_BINARY_DIR})

set(INSTALL_PREFIX_BACKUP ${CMAKE_INSTALL_PREFIX})
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR})
configure_package_config_file(cmake/projectConfig.cmake.in
                              "${CMAKECONFIG_CMAKE_DIR}/${PROJECT_NAME}Config.cmake"
                              INSTALL_DESTINATION "${CMAKECONFIG_CMAKE_DIR}"
                              PATH_VARS CMAKECONFIG_PKG CMAKECONFIG_PKG_INC CMAKECONFIG_PKG_LINK CMAKECONFIG_PKG_LIB CMAKECONFIG_INC CMAKECONFIG_LINK CMAKECONFIG_LIB)
set(CMAKE_INSTALL_PREFIX ${INSTALL_PREFIX_BACKUP})

#export(TARGETS cliini FILE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.cmake)
export(PACKAGE cliini)

#install(TARGETS cliini
#        EXPORT clif
#        RUNTIME DESTINATION bin
#        LIBRARY DESTINATION lib
#        ARCHIVE DESTINATION lib)
