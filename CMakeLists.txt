cmake_minimum_required (VERSION 2.8)
project(clif)

set(CLIF_MINOR_VERSION 1)
set(CLIF_PATCH_VERSION 0)
set(CLIF_VERSION ${FOOBAR_MAJOR_VERSION}.${FOOBAR_MINOR_VERSION}.${FOOBAR_PATCH_VERSION})

cmake_policy(VERSION 2.8)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
  # using Clang
  message("use clang")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
  # using GCC
  message("use gcc")
  set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -g -O3 -fno-omit-frame-pointer -fPIC -std=c++11 -Wall -march=native")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
  # using Visual Studio C++
  message("use msvc")
  set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} /noOy /W3")
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  add_definitions(-DCLIF_COMPILER_MSVC)
  set(CLIF_COMPILER_MSVC true)
  set(CLIF_EXPORT "__declspec(dllexport)")
endif()

find_package(OpenCV)
if (NOT OpenCV_FOUND)
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
  message(WARNING "WARNING: using internal cmake find macros!")
endif()

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
link_directories(${OpenCV_LIB_DIR})

#FIXME check wether actual built is static?
if (CLIF_COMPILER_MSVC)
  set(HDF5_USE_STATIC_LIBRARIES true)
endif()
find_package(HDF5 REQUIRED C CXX)
find_package(HDF5 REQUIRED)
include_directories(${HDF5_INCLUDE_DIR})
link_directories(${HDF5_LIBRARY_DIR})


find_package(Vigra REQUIRED)
include_directories(${Vigra_INCLUDE_DIRS})
#link_directories(${Vigra?})

find_package(Boost REQUIRED COMPONENTS filesystem system)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

#config header (contains CLIF_EXPORT for now)
configure_file(src/lib/config.h.in src/lib/config.h)
set(CLIF_HEADERS ${CLIF_HEADERS} ${CMAKE_CURRENT_BINARY_DIR}/src/lib/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/src/lib/)

find_path(FNMATCH_INCLUDE_DIR NAMES fnmatch.h)
if (NOT FNMATCH_INCLUDE_DIR)
  set(FNMATCH_USE_INTERNAL TRUE CACHE BOOL "use internal fnmatch code")
  add_subdirectory(external/fnmatch)
  include_directories(external/fnmatch)
  link_directories(external/fnmatch)
  set(CLIF_FNMATCH_LIBRARIES ${CLIF_FNMATCH_LIBRARIES} fnmatch)
  set(CLIF_LINK_LIBRARIES ${CLIF_LINK_LIBRARIES} ${CLIF_FNMATCH_LIBRARIES})
endif()

add_subdirectory(external/cliini)
include_directories(external/cliini)

#for generated header
include_directories(${CMAKE_BINARY_DIR}/src/lib)
add_subdirectory(src/lib)
include_directories(src/lib)

add_subdirectory(src/bin)
add_subdirectory(src/gui)

message("")
message("build info:")
message("  optional components:")

if(CLIF_BUILD_QT)
  message("    qt-widgets   -   yes")
else()
  message("    qt-widgets   -   no - ${CLIF_BUILD_QT_REASON}")
endif()

if(FNMATCH_USE_INTERNAL)
  message("    internal fnmatch   -   yes (no fnmatch.h header found)")
else()
  message("    internal fnmatch   -   no")
endif()


message("")

# using Visual Studio C++
# The interesting stuff goes here
# ===============================
# Add all targets to the build-tree export set
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
	export(TARGETS clif clif_vigra cliini FILE "${PROJECT_BINARY_DIR}clifTargets.cmake")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
	export(TARGETS clif clif_vigra cliini fnmatch FILE "${PROJECT_BINARY_DIR}clifTargets.cmake")
endif()

# Export the package for use from the build-tree
# (this registers the build-tree with a global CMake-registry)
export(PACKAGE ${PROJECT_NAME})
# Create the clifConfig.cmake and clifConfigVersion files
file(RELATIVE_PATH REL_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/cmake/clif" "${CMAKE_INSTALL_PREFIX}/include")
# ... for the build tree
set(CONF_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}")
configure_file(cmake/clifConfig.cmake.in "${PROJECT_BINARY_DIR}clifConfig.cmake" @ONLY)
# ... for the install tree
set(CONF_INCLUDE_DIRS "\${CMAKE_INSTALL_PREFIX}/cmake/clif/${REL_INCLUDE_DIR}")
configure_file(cmake/clifConfig.cmake.in "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/clifConfig.cmake" @ONLY)
# ... for both
configure_file(cmake/clifConfigVersion.cmake.in "${PROJECT_BINARY_DIR}clifConfigVersion.cmake" @ONLY)
# Install the clifConfig.cmake and clifConfigVersion.cmake
install(FILES
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/clifConfig.cmake"
  "${PROJECT_BINARY_DIR}/clifConfigVersion.cmake"
  DESTINATION "lib/cmake/clif" COMPONENT dev)



 















