cmake_minimum_required (VERSION 2.6)
project(clif)


set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -g -O3 -fno-omit-frame-pointer -std=c++11 -Wall -march=native")

find_package(Vigra)
find_package(OpenCV)


if ((NOT OpenCV_FOUND) OR (NOT Vigra_FOUND))
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
  message(WARNING "WARNING: using internal cmake find macros!")
endif()

find_package(OpenCV REQUIRED)

find_package(Vigra REQUIRED)
set(CLIF_LINK_LIBRARIES ${CLIF_LINK_LIBRARIES} vigraimpex)
#TODO ask Ullrich: VigraConfig.cmake misses Vigra_LIBRARIES! 

find_package(Boost REQUIRED COMPONENTS filesystem system)

add_subdirectory(external/cliini)
include_directories(external/cliini)

add_subdirectory(src/lib)
include_directories(src/lib)

add_subdirectory(src/bin)
add_subdirectory(src/gui)

find_path(FNMATCH_INCLUDE_DIR NAMES fnmatch.h)
if (NOT FNMATCH_INCLUDE_DIR)
  set(FNMATCH_USE_INTERNAL TRUE)
  add_subdirectory(external/fnmatch)
  include_directories(external/fnmatch)
  set(CLIF_LINK_LIBRARIES ${CLIF_LINK_LIBRARIES} fnmatch)
endif()



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