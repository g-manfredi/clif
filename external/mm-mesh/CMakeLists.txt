cmake_minimum_required(VERSION 2.6)
project(mm-mesh)
string(TOUPPER ${PROJECT_NAME} PNU)

option(${PNU}_BUILD_LIBIGL "build with (internal) libigl" ON)

#####################################################
## COMPILER / FLAGS
#####################################################
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
  set(${PNU}_BUILD_TYPE "SHARED")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
  set(${PNU}_BUILD_TYPE "SHARED")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3 -fno-omit-frame-pointer -fPIC -std=c++11 -Wall -march=native -fopenmp")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
  set(${PNU}_BUILD_TYPE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /noOy /W2 /EHsc")
endif()

#remove old local projectconfig
dep_lists_init()

#deps
dep_lists_append(MetaMat)
dep_lists_append(Eigen3 EIGEN3_INCLUDE_DIR)
dep_lists_append(glfw3 OPTIONAL FOUND_INDICATOR GLFW3_LIBRARY GLFW3_INCLUDE_DIR GLFW3_LIBRARY_DIR GLFW3_LIBRARY)
dep_lists_append(GLEW OPTIONAL)

dep_lists_pkg_search()

if (${PNU}_BUILD_LIBIGL)
  list(APPEND ${PNU}_INC "libigl/include")
  list(APPEND ${PNU}_FEATURES ${PNU}_WITH_LIBIGL)

  if (${PNU}_WITH_GLFW3 AND ${PNU}_WITH_GLEW)
    list(APPEND ${PNU}_FEATURES ${PNU}_WITH_VIEWER)
  endif()
endif()

set(${PNU}_HEADERS mesh.hpp)
dep_lists_prepare_env()

#####################################################
## COMPILE AND LINK
#####################################################

#ucalib
add_library(mm-mesh SHARED mesh.cpp)
target_link_libraries(mm-mesh PUBLIC ${${PNU}_LIB})
target_link_libraries(mm-mesh PRIVATE ${${PNU}_PRIVATE_LIB})

list(APPEND ${PNU}_EXPORT_LIBS mm-mesh)

#####################################################
## PUBLIC HEADERS
#####################################################

#copy headers and generate XXXConfig.cmake
dep_lists_export_local()

#####################################################
## SHOW CONFIG
#####################################################
message("build info for ${PROJECT_NAME}:")
message("  optional components:")

msg_yesno("    opengl viewer" ${PNU}_WITH_VIEWER)
msg_yesno("       glfw3" ${PNU}_WITH_GLFW3)
msg_yesno("       glew" ${PNU}_WITH_GLEW)

message("")
