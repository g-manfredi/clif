cmake_minimum_required (VERSION 2.8.8)
project(clif)

set(CLIF_MINOR_VERSION 1)
set(CLIF_PATCH_VERSION 0)
set(CLIF_VERSION ${FOOBAR_MAJOR_VERSION}.${FOOBAR_MINOR_VERSION}.${FOOBAR_PATCH_VERSION})

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
  # using Clang
  message("use clang")
  list(APPEND CMAKE_CXX_FLAGS "-std=c++11")
  #FIXME
  add_definitions(-DFNM_EXTMATCH=0)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
  # using GCC
  message("use gcc")
  set(CLIF_BUILD_TYPE "SHARED")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3 -fno-omit-frame-pointer -fPIC -std=c++11 -march=native -fopenmp")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
  # using Visual Studio C++
  message("use msvc")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /noOy /W2 /EHsc")
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  add_definitions(-DCLIF_COMPILER_MSVC)
  add_definitions(-DCLIF_EXPORT_LIBRARY)
  add_definitions(-DCLIF_BUILD_STATIC)
  set(CLIF_COMPILER_MSVC true)
  set(CLIF_BUILD_STATIC true)
  set(CLIF_BUILD_TYPE)
  set(CLIF_EXPORT "__declspec(dllexport)")
  set(CLIF_IMPORT "__declspec(dllimport)")
endif()

find_package(OpenCV QUIET)
if (NOT OpenCV_FOUND)
  list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/opencv)
endif()

find_package(Vigra QUIET)
if (NOT Vigra_FOUND)
  list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/vigra)
endif()

find_package(hdmarker QUIET)
if (HDMARKER_FOUND)
  list(APPEND CLIF_PRIVATE_LINK_LIBRARIES ${HDMARKER_LIBRARIES})
  list(APPEND CLIF_PRIVATE_INCLUDE_DIRS ${HDMARKER_INCLUDE_DIRS})
  list(APPEND CLIF_PRIVATE_LIBRARY_DIRS ${HDMARKER_LIBRARY_DIRS})
  
  set(CLIF_WITH_HDMARKER true)
  add_definitions(-DCLIF_WITH_HDMARKER)
endif()

find_package(ucalib QUIET)
if (UCALIB_FOUND)
  list(APPEND CLIF_PRIVATE_LINK_LIBRARIES ${UCALIB_LIBRARIES})
  list(APPEND CLIF_PRIVATE_INCLUDE_DIRS ${UCALIB_INCLUDE_DIRS})
  list(APPEND CLIF_PRIVATE_LIBRARY_DIRS ${UCALIB_LIBRARY_DIRS})
  
  set(CLIF_WITH_UCALIB true)
  add_definitions(-DCLIF_WITH_UCALIB)
endif()

find_package(OpenCV REQUIRED)
list(APPEND CLIF_LINK_LIBRARIES ${OpenCV_LIBRARIES})
list(APPEND CLIF_INCLUDE_DIRS ${OpenCV_INCLUDE_DIRS})
list(APPEND CLIF_LIBRARY_DIRS ${OpenCV_LIB_DIR})

#FIXME check wether actual built is static?
if (CLIF_COMPILER_MSVC)
  set(HDF5_USE_STATIC_LIBRARIES true)
endif()
find_package(HDF5 REQUIRED C CXX)
find_package(HDF5 REQUIRED)
list(APPEND CLIF_LINK_LIBRARIES ${HDF5_LIBRARIES} ${HDF5_CXX_LIBRARIES})
list(APPEND CLIF_INCLUDE_DIRS ${HDF5_INCLUDE_DIR})
list(APPEND CLIF_LIBRARY_DIRS ${HDF5_LIBRARY_DIR})

find_package(Vigra REQUIRED)
#FIXME ask Ullrich why theres not VIGRA_LIBRARIES?
list(APPEND CLIF_LINK_LIBRARIES vigraimpex)
list(APPEND CLIF_INCLUDE_DIRS ${Vigra_INCLUDE_DIRS})
#FIXME ask Ullrich why theres not VIGRA_LIBRARY_DIRS?
#list(APPEND CLIF_LIBRARY_DIRS !!!FIXME!!!)

find_package(Boost REQUIRED COMPONENTS filesystem system)
list(APPEND CLIF_LINK_LIBRARIES ${Boost_LIBRARIES})
list(APPEND CLIF_INCLUDE_DIRS ${Boost_INCLUDE_DIRS})
list(APPEND CLIF_LIBRARY_DIRS ${Boost_LIBRARY_DIRS})


find_package(Qt5Widgets QUIET)

if (Qt5Widgets_FOUND)
  set(CLIF_BUILD_QT TRUE)
  add_definitions(-DCLIF_BUILD_QT)
  
  list(APPEND CLIF_LINK_LIBRARIES ${Qt5Widgets_LIBRARIES})
  list(APPEND CLIF_INCLUDE_DIRS ${Qt5Widgets_INCLUDE_DIRS})
  list(APPEND CLIF_LIBRARY_DIRS ${Qt5Widgets_LIBRARY_DIRS})
else()
  set(CLIF_BUILD_QT_REASON "[Qt5Widgets not found]")
endif()

#WARNING no find_package after this line! (would use our included find... macros!)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
include(ClifCMakePackageConfigHelpers)


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
  list(APPEND CLIF_BUILD_COMPILE_DIRS external/fnmatch)
endif()
list(APPEND CLIF_BUILD_COMPILE_DIRS external/cliini src/lib src/bin src/gui src/clifview)

configure_file(config.h.in config.h)
list(APPEND CLIF_HEADERS config.h)
list(APPEND CLIF_FULLPATH_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/config.h) 
list(APPEND CLIF_BUILD_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR})

foreach(dir ${CLIF_BUILD_COMPILE_DIRS})
  include_directories(${CLIF_BUILD_INCLUDE_DIRS} ${CLIF_INCLUDE_DIRS} ${CLIF_PRIVATE_INCLUDE_DIRS})
  link_directories(${CLIF_BUILD_LINK_DIRS} ${CLIF_LIBRARY_DIRS} ${CLIF_PRIVATE_LIBRARY_DIRS})
  set(CLIF_LIBRARIES ${CLIF_LINK_LIBRARIES})
  add_subdirectory(${dir})
  list(REMOVE_DUPLICATES CLIF_BUILD_INCLUDE_DIRS)
  list(REMOVE_DUPLICATES CLIF_INCLUDE_DIRS)
  list(REMOVE_DUPLICATES CLIF_BUILD_LINK_DIRS)
  list(REMOVE_DUPLICATES CLIF_LIBRARY_DIRS)
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


message("")

#copy headers into clif subdir in build

foreach(header ${CLIF_FULLPATH_HEADERS})
#message(${header})
  get_filename_component(_FILE ${header} NAME)
  configure_file(${header} "include/clif/${_FILE}" COPYONLY)
  #message(${_FILE})
endforeach()

# for local built tree use 
set(CLIF_INSTALL_INCLUDE_DIR "include" "include/clif")
foreach(dir ${CLIF_BUILD_LINK_DIRS})
  if (IS_ABSOLUTE "${dir}")
    set(CLIF_INSTALL_LIBRARY_DIR ${CLIF_INSTALL_LIBRARY_DIR} "${dir}")
  else()
    set(CLIF_INSTALL_LIBRARY_DIR ${CLIF_INSTALL_LIBRARY_DIR} "${CMAKE_CURRENT_BINARY_DIR}/${dir}")
  endif()
endforeach()
set(CLIF_INSTALL_LIBRARIES clif cliini clif-qt)
set(CLIF_INSTALL_CMAKE_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(INSTALL_PREFIX_BACKUP ${CMAKE_INSTALL_PREFIX})
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR})
configure_package_config_file(cmake/clifConfig.cmake.in
                              "${CLIF_INSTALL_CMAKE_DIR}/clifConfig.cmake"
                              INSTALL_DESTINATION "${CLIF_INSTALL_CMAKE_DIR}"
                              PATH_VARS CLIF_INSTALL_INCLUDE_DIR CLIF_INSTALL_LIBRARY_DIR CLIF_INSTALL_LIBRARIES CLIF_INSTALL_CMAKE_DIR)
set(CMAKE_INSTALL_PREFIX ${INSTALL_PREFIX_BACKUP})

# for the installed package
set(CLIF_INSTALL_INCLUDE_DIR "include" "include/clif")
set(CLIF_INSTALL_LIBRARY_DIR "lib")
set(CLIF_INSTALL_LIBRARIES clif cliini clif-qt)
set(CLIF_INSTALL_CMAKE_DIR "lib/cmake/clif")
configure_package_config_file(cmake/clifConfig.cmake.in
                              "config/clifConfig.cmake"
                              INSTALL_DESTINATION ${CLIF_INSTALL_CMAKE_DIR}
                              PATH_VARS CLIF_INSTALL_INCLUDE_DIR CLIF_INSTALL_LIBRARY_DIR CLIF_INSTALL_LIBRARIES CLIF_INSTALL_CMAKE_DIR)

# ... for both
configure_file(cmake/clifConfigVersion.cmake.in "${PROJECT_BINARY_DIR}/clifConfigVersion.cmake" @ONLY)

# Install the clifConfig.cmake and clifConfigVersion.cmake
install(FILES
  "${PROJECT_BINARY_DIR}/config/clifConfig.cmake"
  "${PROJECT_BINARY_DIR}/clifConfigVersion.cmake"
  DESTINATION ${CLIF_INSTALL_CMAKE_DIR} COMPONENT dev)

#export targets for installation
install(EXPORT clif DESTINATION ${CLIF_INSTALL_CMAKE_DIR} COMPONENT dev)
install(FILES ${CLIF_FULLPATH_HEADERS} DESTINATION include/clif )

#local export into binary dir (may be used with cmake .. -DCMAKE_PREFIX_PATH=<...> or with cmake-gui)
if (${CMAKE_VERSION} VERSION_LESS 3.0.0)
  #TODO use a generated list of targets!
  export(TARGETS clif cliini FILE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.cmake)
else()
  message(${CMAKE_VERSION})
  export(EXPORT clif FILE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.cmake)
endif()

#export into cmakes local package cache - should be directly visible to cmake of local user
export(PACKAGE clif)



