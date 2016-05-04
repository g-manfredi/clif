cmake_minimum_required(VERSION 2.8)
project(mm-mesh)

#####################################################
## INIT
#####################################################
include(../flexdeplists/FlexDepLists.cmake)
dep_lists_init()

#####################################################
## ENV
#####################################################
option(${PNU}_BUILD_LIBIGL "build with (internal) libigl" ON)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
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
dep_lists_append(MetaMat)
dep_lists_append(Eigen3 EIGEN3_INCLUDE_DIR)

dep_lists_append(glfw3 OPTIONAL GLFW3_INCLUDE_DIR GLFW3_LIBRARY_DIR GLFW3_LIBRARY)
dep_lists_append(GLEW OPTIONAL)

# search pkgs
dep_lists_pkg_search()

# checker wether to build with libigl (internal, private)
if (${PNU}_BUILD_LIBIGL)
  list(APPEND ${PNU}_INC "libigl/include")
  list(APPEND ${PNU}_FEATURES ${PNU}_WITH_LIBIGL)

  if (${PNU}_WITH_GLFW3 AND ${PNU}_WITH_GLEW)
    set(${PNU}_WITH_VIEWER TRUE)
	if (WIN32)
		set(${PNU}_EXTRA_LIBS glu32 opengl32)
	endif()
    list(APPEND ${PNU}_FEATURES ${PNU}_WITH_VIEWER)
  endif()
endif()

set(${PNU}_HEADERS mesh.hpp)
dep_lists_prepare_env()

#####################################################
## COMPILE
#####################################################
add_library(mm-mesh mesh.cpp)
target_link_libraries(mm-mesh PUBLIC ${${PNU}_LIB})
target_link_libraries(mm-mesh PRIVATE ${${PNU}_PRIVATE_LIB})
list(APPEND ${PNU}_EXPORT_LIBS mm-mesh)

#####################################################
## INSTALL
#####################################################
dep_lists_export_local()

#####################################################
## INFO
#####################################################
message("build info for ${PROJECT_NAME}:")
message("  optional components:")
msg_yesno("    opengl viewer" ${PNU}_WITH_VIEWER)
msg_yesno("       glfw3" ${PNU}_WITH_GLFW3)
msg_yesno("       glew" ${PNU}_WITH_GLEW)
message("")
