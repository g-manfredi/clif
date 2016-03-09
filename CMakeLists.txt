cmake_minimum_required (VERSION 2.8.8)
project(clif)
string(TOUPPER ${PROJECT_NAME} PNU)


#####################################################
## VARIABLE DOCUMENTATION
#####################################################
#Meta:
#  PNU              - project name in uppercase - prefixed to (nearly) all variables
#
#Dependencies (packages and respective cmake variables)
#  _PKG               - required cmake package names - passed to find_package() - will also be included from ...Config.cmake
#  _PKG_OPT           - optional cmake package names - passed to find_package() - will also be included from ...Config.cmake, ..._USE_${..._PKG_OPT} will be set to true if optional pkg is found
# _PRIVATE_PKG
# _PRIVATE_PKG_OPT
###
#  _PKG_INC           - variable names to add to include_directories (from the respecive find_package() calls)
#  _PKG_LINK          - variable names to add to link_directories
#  _PKG_LIB           - variable names to add to link_libraries
#
#actual directories/libraries from dependencies
#  _INC          - actual content from _PKG_INC vars
#  _LINK         - link dirs
#  _LIB          - libs

#  _PRIVATE_INC  - actual content from _PKG_INC vars
#  _PRIVATE_LINK - link dirs
#  _PRIVATE_LIB  - libs
#
#  _EXPORT_LIBS       - targets (libraries) for installation, export and to put into _LIBRARIES into ..Config.cmake
#  _EXPORT_BINS       - targets (executables) to install
#
#
#  _BUILD_TYPE        - SHARED or "" (set depending on compiler)
#
#  _PROJECT_HEADERS
#  _PROJECT_LIBRARIES

#####################################################
## GLOBAL
#####################################################

set(CLIF_MINOR_VERSION 1)
set(CLIF_MAJOR_VERSION 0)
set(CLIF_VERSION ${CLIF_MAJOR_VERSION}.${CLIF_MINOR_VERSION})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
include(CMakePackageConfigListHelpers)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

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
function(pkg_found PKG RET)
  string(TOLOWER ${PKG} LOW)
  string(TOUPPER ${PKG} UP)
  
  set(${RET} FALSE PARENT_SCOPE)
  
  if (${${PKG}_FOUND})
    set(${RET} TRUE PARENT_SCOPE)
  endif()
  if (${${LOW}_FOUND})
    set(${RET} TRUE PARENT_SCOPE)
  endif()
  if (${${UP}_FOUND})
    set(${RET} TRUE PARENT_SCOPE)
  endif()
endfunction()

#macro so find_package runs in global scope!
macro(check_find PACKAGE RET)
  find_package(${PACKAGE} QUIET)
  
  string(TOLOWER ${PACKAGE} PKG_LOW)
  
  #check if pkg was found (various conventions...)
  pkg_found(${PACKAGE} FOUND)
  if (FOUND)
    message("${PACKAGE} - found")
    set(${RET} TRUE)
    list(APPEND ${PNU}_DEFINES ${RET})
  else()
    #message("${${pkg_up}_FOUND}"
    #message("did not find ${PACKAGE}, adding cmake/${PKG_LOW} to CMAKE_MODULE_PATH")
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/find/${PKG_LOW})
    find_package(${PACKAGE} QUIET)
    pkg_found(${PACKAGE} FOUND)
    if (FOUND)
      message("${PACKAGE} - found using included Find${PACKAGE}.cmake")
      set(${RET} TRUE)
      list(APPEND ${PNU}_DEFINES ${RET})
    else()
      message("${PACKAGE} - missing")
      set(${RET} FALSE)
    endif()
  endif()
endmacro()

macro(clean_list LIST)
  if (DEFINED ${LIST})
    list(REMOVE_DUPLICATES ${LIST})
  endif()
endmacro()

#####################################################
## DEPENDENCIES
#####################################################

#eigen3
list(APPEND ${PNU}_PKG_OPT Eigen3)
list(APPEND ${PNU}_PKG_INC EIGEN3_INCLUDE_DIR)

#ucalib (contains ceres-solver)
list(APPEND ${PNU}_PRIVATE_PKG_OPT ucalib)
list(APPEND ${PNU}_PRIVATE_PKG_INC  UCALIB_INCLUDE_DIRS)
list(APPEND ${PNU}_PRIVATE_PKG_LINK UCALIB_LIBRARY_DIRS)
list(APPEND ${PNU}_PRIVATE_PKG_LIB  UCALIB_LIBRARIES)

#cliini 
list(APPEND ${PNU}_PRIVATE_PKG cliini)
list(APPEND ${PNU}_PRIVATE_PKG_INC  CLIINI_INCLUDE_DIRS)
list(APPEND ${PNU}_PRIVATE_PKG_LINK CLIINI_LIBRARY_DIRS)
list(APPEND ${PNU}_PRIVATE_PKG_LIB  CLIINI_LIBRARIES)


list(APPEND ${PNU}_PKG OpenCV)
list(APPEND ${PNU}_PKG_INC  OpenCV_INCLUDE_DIRS)
list(APPEND ${PNU}_PKG_LINK OpenCV_LIB_DIR)
list(APPEND ${PNU}_PKG_LIB  OpenCV_LIBRARIES)


list(APPEND ${PNU}_PKG Vigra)
list(APPEND ${PNU}_PKG_INC Vigra_INCLUDE_DIRS)
set(VIGRAIMPEX_VAR vigraimpex)
list(APPEND ${PNU}_PKG_LIB VIGRAIMPEX_VAR)
#FIXME ask Ullrich why theres no VIGRA_LIBRARIES?

list(APPEND ${PNU}_PKG_OPT hdmarker)
list(APPEND ${PNU}_PKG_INC  HDMARKER_INCLUDE_DIRS)
list(APPEND ${PNU}_PKG_LINK HDMARKER_LIBRARY_DIRS)
list(APPEND ${PNU}_PKG_LIB  HDMARKER_LIBRARIES)
  
#see https://github.com/glfw/glfw/issues/483
find_package(glfw QUIET)
if ("${GLFW3_INCLUDE_DIR}" STREQUAL "")
  set(glfw3_DIR /usr/lib/cmake/glfw/)
endif()
list(APPEND ${PNU}_PKG_OPT glfw3)
list(APPEND ${PNU}_PKG_INC  GLFW3_INCLUDE_DIR)
list(APPEND ${PNU}_PKG_LINK GLFW3_LIBRARY_DIR)
list(APPEND ${PNU}_PKG_LIB  GLFW3_LIBRARY)

list(APPEND ${PNU}_PKG_OPT GLEW)
list(APPEND ${PNU}_PKG_INC GLEW_INCLUDE_DIRS)
list(APPEND ${PNU}_PKG_LIB GLEW_LIBRARIES)

# FIXME boost vs h?
if (CLIF_COMPILER_MSVC)
	set(Boost_USE_STATIC_LIBS ON)
endif()
find_package(Boost REQUIRED COMPONENTS filesystem system)
list(APPEND CLIF_LINK_LIBRARIES ${Boost_LIBRARIES})
list(APPEND CLIF_INCLUDE_DIRS ${Boost_INCLUDE_DIRS})
list(APPEND CLIF_LIBRARY_DIRS ${Boost_LIBRARY_DIRS})

#####################################################
## EXPORT / INSTALL
#####################################################

# FIXME clif-qt should be optional
#list(APPEND ${PNU}_EXPORT_LIBS clif clif-qt)

#####################################################
## HACK/EXTRA
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

#compile
if (FNMATCH_USE_INTERNAL)
  add_subdirectory(external/fnmatch)
  include_directories(external/fnmatch)
  link_directories(external/fnmatch)
endif()

#cliini (is referenced as package above)
add_subdirectory(external/cliini)

#####################################################
## search packages
#####################################################
foreach(PACKAGE ${${PNU}_PKG_OPT})
  string(TOUPPER ${PACKAGE} PKG_UP)
  check_find(${PACKAGE} ${PNU}_WITH_${PKG_UP})
  if (${PNU}_WITH_${PKG_UP})
    list(APPEND ${PNU}_EXPORT_DEPS ${PACKAGE})
  endif()
endforeach()

foreach(PACKAGE ${${PNU}_PRIVATE_PKG_OPT})
  string(TOUPPER ${PACKAGE} PKG_UP)
  check_find(${PACKAGE} ${PNU}_WITH_${PKG_UP})
endforeach()

foreach(PACKAGE ${${PNU}_PKG})
  string(TOUPPER ${PACKAGE} PKG_UP)
  check_find(${PACKAGE} ${PNU}_WITH_${PKG_UP})
  if (NOT ${${PNU}_WITH_${PKG_UP}})
    list(APPEND ${PNU}_MISSING_REQUIRED ${PACKAGE})
  else()
    list(APPEND ${PNU}_EXPORT_DEPS ${PACKAGE})
  endif()
endforeach()

foreach(PACKAGE ${${PNU}_PRIVATE_PKG})
  string(TOUPPER ${PACKAGE} PKG_UP)
  check_find(${PACKAGE} ${PNU}_WITH_${PKG_UP})
  if (NOT ${${PNU}_WITH_${PKG_UP}})
    list(APPEND ${PNU}_MISSING_REQUIRED ${PACKAGE})
  endif()
endforeach()

if (${PNU}_MISSING_REQUIRED)
  message("")
  message("missing REQUIRED packages:")
  foreach(PACKAGE ${${PNU}_MISSING_REQUIRED})
    message(${PACKAGE})
  endforeach()
  message("")
  message(FATAL_ERROR "required package(s) not found, exiting.")
endif()

#####################################################
## FEATURES depending on multiple packages
#####################################################

if (CLIF_WITH_EIGEN3)
# disable ATM
#  set(CLIF_WITH_LIBIGL true)
#  list(APPEND ${PNU}_DEFINES CLIF_WITH_LIBIGL)
endif()

if (CLIF_WITH_LIBIGL AND CLIF_WITH_GLFW3 AND CLIF_WITH_GLEW)  
  set(CLIF_WITH_LIBIGL_VIEWER true)
  list(APPEND ${PNU}_DEFINES CLIF_WITH_LIBIGL_VIEWER)
  #add_definitions(-DIGL_VIEWER_WITH_NANOGUI)
endif()

find_package(OpenCV REQUIRED)

#####################################################
## SET INCLUDES, LIBS, ... (public)
#####################################################

foreach(INCLUDE ${${PNU}_PKG_INC})
  if (NOT ("${${INCLUDE}}" MATCHES ".*-NOTFOUND"))
    list(APPEND ${PNU}_INC ${${INCLUDE}})
  else()
    # FIXME remove including in deps!
  endif()
endforeach()
clean_list(${PNU}_INC)

foreach(LIBDIR ${${PNU}_PKG_LINK})
  if (NOT ("${${LIBDIR}}" MATCHES ".*-NOTFOUND"))
    list(APPEND ${PNU}_LINK ${${LIBDIR}})
  else()
    # FIXME remove including in deps!
  endif()
endforeach()
clean_list(${PNU}_LINK)

foreach(LIB ${${PNU}_PKG_LIB})
  if (NOT ("${${LIB}}" MATCHES ".*-NOTFOUND"))
    list(APPEND ${PNU}_LIB ${${LIB}})
  else()
    # FIXME remove including in deps!
  endif()
endforeach()
clean_list(${PNU}_LIB)

#####################################################
## SET INCLUDES, LIBS, ... (private)
#####################################################

foreach(INCLUDE ${${PNU}_PRIVATE_PKG_INC})
  if (NOT ("${${INCLUDE}}" MATCHES ".*-NOTFOUND"))
    list(APPEND ${PNU}_PRIVATE_INC ${${INCLUDE}})
  else()
    # FIXME remove including in deps!
  endif()
endforeach()
clean_list(${PNU}_PRIVATE_INC)

foreach(LIBDIR ${${PNU}_PRIVATE_PKG_LINK})
  if (NOT ("${${LIBDIR}}" MATCHES ".*-NOTFOUND"))
    list(APPEND ${PNU}_PRIVATE_LINK ${${LIBDIR}})
  else()
    # FIXME remove including in deps!
  endif()
endforeach()
clean_list(${PNU}_PRIVATE_LINK)

foreach(LIB ${${PNU}_PRIVATE_PKG_LIB})
  if (NOT ("${${LIB}}" MATCHES ".*-NOTFOUND"))
    list(APPEND ${PNU}_PRIVATE_LIB ${${LIB}})
  else()
    # FIXME remove including in deps!
  endif()
endforeach()
clean_list(${PNU}_PRIVATE_LIB)


#####################################################
## actually inc/link DIRS (from above)
#####################################################
include_directories(${${PNU}_INC} ${${PNU}_PRIVATE_INC})
link_directories(${${PNU}_LINK} ${${PNU}_PRIVATE_LINK})

######################

set(${PNU}_BUILD_TYPE "SHARED")
if (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
  # using Clang
  list(APPEND CMAKE_CXX_FLAGS "-std=c++11")
  #FIXME
  add_definitions(-DFNM_EXTMATCH=0)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
  # using GCC
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fno-omit-frame-pointer -fPIC -std=c++11 -march=native -fopenmp")
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


#This is to avoid that stupid windows links dlls

if (NOT COMPILE_EXTERNAL)
	find_package(HDF5 REQUIRED C CXX)
elseif()
	if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
		set(HDF5_USE_STATIC_LIBRARIES true)
		if (CMAKE_BUILD_TYPE MATCHES Debug)
		message(STATUS "DEBUG")
			set(HDF5_C_LIBRARIES ${HDF5_LIBRARIES})
		else()
		message(STATUS "Release")
			set(HDF5_C_LIBRARIES ${HDF5_LIBRARIES})
		endif()
	endif()
endif()

list(APPEND CLIF_LINK_LIBRARIES ${HDF5_LIBRARIES} ${HDF5_CXX_LIBRARIES})
list(APPEND CLIF_INCLUDE_DIRS ${HDF5_INCLUDE_DIR})
list(APPEND CLIF_LIBRARY_DIRS ${HDF5_LIBRARY_DIR})

find_package(Vigra REQUIRED)
#FIXME ask Ullrich why theres not VIGRA_LIBRARIES?
list(APPEND CLIF_LINK_LIBRARIES vigraimpex)
list(APPEND CLIF_INCLUDE_DIRS ${Vigra_INCLUDE_DIRS})
#FIXME ask Ullrich why theres not VIGRA_LIBRARY_DIRS?
#list(APPEND CLIF_LIBRARY_DIRS !!!FIXME!!!)


find_package(Qt5Widgets QUIET)

if (Qt5Widgets_FOUND)
  set(CLIF_BUILD_QT TRUE)
  # FIXME use CLIF_DEFINES
  add_definitions(-DCLIF_BUILD_QT)
  if (COMPILE_EXTERNAL)#This is necessary because windows somehow don't want to add the debug part when it gets compiled by openlf
	list(APPEND CLIF_LINK_LIBRARIES ${Qt5Widgets_LIBRARIES} debug;Qt5::Widgets)
  else()
	list(APPEND CLIF_LINK_LIBRARIES ${Qt5Widgets_LIBRARIES})
  endif()
  list(APPEND CLIF_INCLUDE_DIRS ${Qt5Widgets_INCLUDE_DIRS})
  list(APPEND CLIF_LIBRARY_DIRS ${Qt5Widgets_LIBRARY_DIRS})
  #message("QTQTQTQT: ${CLIF_LINK_LIBRARIES}")
else()
  set(CLIF_BUILD_QT_REASON "[Qt5Widgets not found]")
endif()

# if (NOT DEFINED FNMATCH_USE_INTERNAL)
#   find_path(FNMATCH_INCLUDE_DIR NAMES fnmatch.h)
#   if (NOT FNMATCH_INCLUDE_DIR)
#     set(FNMATCH_USE_INTERNAL TRUE CACHE BOOL "use internal fnmatch code")
#   else()
#     set(FNMATCH_USE_INTERNAL FALSE CACHE BOOL "use internal fnmatch code")
#   endif()
# endif()
# 
# #compile
# if (FNMATCH_USE_INTERNAL)
#   list(APPEND CLIF_BUILD_COMPILE_DIRS external/fnmatch)
# endif()
list(APPEND CLIF_BUILD_COMPILE_DIRS src/lib src/bin src/gui src/clifview)

foreach(define ${${PNU}_DEFINES})
  set(${PNU}_DEFINES_STRING "${${PNU}_DEFINES_STRING}\n#define ${define}")
endforeach()

configure_file(config.h.in config.h)
list(APPEND CLIF_HEADERS config.h)
list(APPEND CLIF_FULLPATH_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/config.h) 
list(APPEND CLIF_BUILD_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/include external/libigl/include)


# include_directories(${CLIF_BUILD_INCLUDE_DIRS} ${CLIF_INCLUDE_DIRS} ${CLIF_PRIVATE_INCLUDE_DIRS})
# link_directories(${CLIF_BUILD_LINK_DIRS} ${CLIF_LIBRARY_DIRS} ${CLIF_PRIVATE_LIBRARY_DIRS})
# set(CLIF_LIBRARIES ${CLIF_LINK_LIBRARIES})
# add_subdirectory(${dir})
# clean_list(CLIF_BUILD_INCLUDE_DIRS)
# clean_list(CLIF_INCLUDE_DIRS)
# clean_list(CLIF_BUILD_LINK_DIRS)
# clean_list(CLIF_LIBRARY_DIRS)
#set(CLIF_LIBRARIES ${CLIF_LINK_LIBRARIES})

#   clean_list(CLIF_BUILD_INCLUDE_DIRS)
#   clean_list(CLIF_INCLUDE_DIRS)
#   clean_list(CLIF_BUILD_LINK_DIRS)
#   clean_list(CLIF_LIBRARY_DIRS)


# FIXME remove all content and move to _LIB, _LINK, _INC
link_directories(${CLIF_LIBRARY_DIRS})
 link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
include_directories(${CLIF_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR}/include ${CMAKE_CURRENT_BINARY_DIR}/include/clif)
list(APPEND ${PNU}_LIB ${CLIF_LINK_LIBRARIES})

foreach(dir ${CLIF_BUILD_COMPILE_DIRS})
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/${dir})
  link_directories(${CMAKE_CURRENT_SOURCE_DIR}/${dir})
endforeach()

foreach(dir ${CLIF_BUILD_COMPILE_DIRS})
  add_subdirectory(${dir})
endforeach()

message("")
message("build info:")
message("  optional components:")

if(CLIF_BUILD_QT)
  message("    qt-widgets         -   yes")
else()
  message("    qt-widgets         -   no - ${CLIF_BUILD_QT_REASON}")
endif()

if(FNMATCH_USE_INTERNAL)
  message("    internal fnmatch   -   yes (no fnmatch.h header found)")
else()
  message("    internal fnmatch   -   no")
endif()

if(CLIF_WITH_HDMARKER)
  message("    hdmarker detection -   yes")
else()
  message("    hdmarker detection -   no")
endif()

if(CLIF_WITH_UCALIB)
  message("    ucalib calibration -   yes")
else()
  message("    ucalib calibration -   no")
endif()

if(CLIF_WITH_LIBIGL)
  message("    mesh (libigl)      -   yes")
else()
  message("    mesh (libigl)      -   no")
endif()

if(CLIF_WITH_EIGEN3)
  message("      eigen3           -   yes")
else()
  message("      eigen3           -   no")
endif()

if(CLIF_WITH_LIBIGL_VIEWER)
  message("    mesh viewer        -   yes")
else()
  message("    mesh viewer        -   no")
endif()

if(CLIF_WITH_GLFW3)
  message("      glfw3            -   yes")
else()
  message("      glfw3            -   no")
endif()

if(CLIF_WITH_GLFW3)
  message("      GLEW             -   yes")
else()
  message("      GLEW             -   no")
endif()

message("")

#copy headers into clif subdir in build

foreach(header ${CLIF_FULLPATH_HEADERS})
#message(${header})
  get_filename_component(_FILE ${header} NAME)
  configure_file(${header} "include/clif/${_FILE}" COPYONLY)
  #message(${_FILE})
endforeach()

#####################################################
## EXPORT
#####################################################

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
list(APPEND CMAKECONFIG_LIB ${CLIF_INSTALL_LIBRARIES}) # FIXME remove this! (clif libraries)
list(APPEND CMAKECONFIG_LIB ${CLIF_LINK_LIBRARIES}) # FIXME remove this! (direct dependecies)


set(CMAKECONFIG_LINK "lib")
list(APPEND CMAKECONFIG_LINK ${CLIF_INSTALL_LIBRARY_DIR}) # FIXME remove this!

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

####

# for local built tree use 
# set(CLIF_INSTALL_INCLUDE_DIR "include" "include/clif" ${CLIF_INCLUDE_DIRS})
# foreach(dir ${CLIF_BUILD_LINK_DIRS})
#   if (IS_ABSOLUTE "${dir}")
#     set(CLIF_INSTALL_LIBRARY_DIR ${CLIF_INSTALL_LIBRARY_DIR} "${dir}")
#   else()
#     set(CLIF_INSTALL_LIBRARY_DIR ${CLIF_INSTALL_LIBRARY_DIR} "${CMAKE_CURRENT_BINARY_DIR}/${dir}")
#   endif()
# endforeach()
# set(CLIF_INSTALL_LIBRARIES clif cliini clif-qt)
# set(CLIF_INSTALL_CMAKE_DIR ${CMAKE_CURRENT_BINARY_DIR})
# set(INSTALL_PREFIX_BACKUP ${CMAKE_INSTALL_PREFIX})
# set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR})
# configure_package_config_file(cmake/clifConfig.cmake.in
#                               "${CLIF_INSTALL_CMAKE_DIR}/clifConfig.cmake"
#                               INSTALL_DESTINATION "${CLIF_INSTALL_CMAKE_DIR}"
#                               PATH_VARS CLIF_INSTALL_INCLUDE_DIR CLIF_INSTALL_LIBRARY_DIR CLIF_INSTALL_LIBRARIES CLIF_INSTALL_CMAKE_DIR)
# set(CMAKE_INSTALL_PREFIX ${INSTALL_PREFIX_BACKUP})

# for the installed package
# set(CLIF_INSTALL_INCLUDE_DIR "include" "include/clif" ${CLIF_INCLUDE_DIRS})
# set(CLIF_INSTALL_LIBRARY_DIR "lib")
# set(CLIF_INSTALL_LIBRARIES clif clif-qt)
# set(CLIF_INSTALL_CMAKE_DIR "lib/cmake/clif")
# configure_package_config_file(cmake/projectConfig.cmake.in
#                               "config/projectConfig.cmake"
#                               INSTALL_DESTINATION ${CLIF_INSTALL_CMAKE_DIR}
#                               PATH_VARS CLIF_INSTALL_INCLUDE_DIR CLIF_INSTALL_LIBRARY_DIR CLIF_INSTALL_LIBRARIES CLIF_INSTALL_CMAKE_DIR)

# Install the clifConfig.cmake and clifConfigVersion.cmake
#install(FILES
  #"${PROJECT_BINARY_DIR}/config/projectConfig.cmake"
  #DESTINATION ${CLIF_INSTALL_CMAKE_DIR} COMPONENT dev)

#export targets for installation
#install(EXPORT clif DESTINATION ${CLIF_INSTALL_CMAKE_DIR} COMPONENT dev)
install(FILES ${CLIF_FULLPATH_HEADERS} DESTINATION include/clif )

# FIXME check this on old cmake!
#local export into binary dir (may be used with cmake .. -DCMAKE_PREFIX_PATH=<...> or with cmake-gui)
#if (${CMAKE_VERSION} VERSION_LESS 3.0.0)
  #TODO use a generated list of targets!
  #export(TARGETS clif FILE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.cmake)
#else()
  #message(${CMAKE_VERSION})
  #export(EXPORT clif FILE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.cmake)
  # FIXME where do we get a list of dependend imported targets?
  #export(TARGETS clif FILE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.cmake)
#endif()

#export into cmakes local package cache - should be directly visible to cmake of local user
export(PACKAGE clif)



