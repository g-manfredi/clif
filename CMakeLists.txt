cmake_minimum_required (VERSION 2.6)
project(clif)

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -g -O3 -fno-omit-frame-pointer -std=c++11 -Wall")

find_package(OpenCV REQUIRED)

add_subdirectory(external/cliini)
include_directories(external/cliini)

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


message("")