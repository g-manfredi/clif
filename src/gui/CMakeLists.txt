cmake_minimum_required (VERSION 2.6)
project(clif-qt)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -g -std=c++11 -Wall")

find_package(Qt5Widgets REQUIRED)

find_package(Qwt REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)


include_directories(${Qt5Widgets_INCLUDE_DIRS})
include_directories(${Qwt_INCLUDE_DIRS})

add_definitions(-DWTF_HACK_QT_EXPORT)
add_library(clif-qt SHARED clifscaledimageview.cpp clifepiview.cpp clifqt.cpp)

#TODO add clif
target_link_libraries(clif-qt ${Qt5Widgets_LIBRARIES} ${Qwt_LIBRARIES})
