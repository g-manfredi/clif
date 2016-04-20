cmake_minimum_required (VERSION 2.8.8)
project(clif)
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

dep_lists_append(OpenCV OpenCV_INCLUDE_DIRS "" OpenCV_LIBS)
dep_lists_append(MetaMat)
dep_lists_append(cliini)

set(VIGRAIMPEX_VAR vigraimpex)
dep_lists_append(Vigra Vigra_INCLUDE_DIRS VIGRAIMPEX_VAR)

dep_lists_append(hdmarker OPTIONAL)
dep_lists_append(mm-mesh OPTIONAL)
dep_lists_append(ucalib OPTIONAL)
dep_lists_append(Qt5Widgets OPTIONAL Qt5Widgets_INCLUDE_DIRS Qt5Widgets_LIBRARY_DIRS Qt5Widgets_LIBRARIES)

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

dep_lists_pkg_search()
dep_lists_prepare_env()

# find_package(Qt5Widgets QUIET)
# 
# if (Qt5Widgets_FOUND)
#   set(CLIF_BUILD_QT TRUE)
#   # FIXME use CLIF_DEFINES
#   add_definitions(-DCLIF_BUILD_QT)
#   if (COMPILE_EXTERNAL)#This is necessary because windows somehow don't want to add the debug part when it gets compiled by openlf
# 	list(APPEND CLIF_LINK_LIBRARIES ${Qt5Widgets_LIBRARIES} debug;Qt5::Widgets)
#   else()
# 	list(APPEND CLIF_LINK_LIBRARIES ${Qt5Widgets_LIBRARIES})
#   endif()
#   list(APPEND CLIF_INCLUDE_DIRS ${Qt5Widgets_INCLUDE_DIRS})
#   list(APPEND CLIF_LIBRARY_DIRS ${Qt5Widgets_LIBRARY_DIRS})
#   #message("QTQTQTQT: ${CLIF_LINK_LIBRARIES}")
# else()
#   set(CLIF_BUILD_QT_REASON "[Qt5Widgets not found]")
# endif()

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



message("build info: for ${PROJECT_NAME}:")
message("  intern/extern:")
msg_yesno("    internal fnmatch" FNMATCH_USE_INTERNAL COLOR Green)
message("  optional components:")
msg_yesno("    qt-widgets" CLIF_WITH_QT5WIDGETS)
msg_yesno("    hdmarker detection" CLIF_WITH_HDMARKER)
msg_yesno("    ucalib calibration" CLIF_WITH_UCALIB)
msg_yesno("    mesh libigl" MM-MESH_WITH_LIBIGL)
msg_yesno("    mesh viewer" MM-MESH_WITH_VIEWER)

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


