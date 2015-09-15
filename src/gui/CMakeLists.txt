cmake_minimum_required (VERSION 2.6)
project(clif-qt)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

#set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -g -std=c++11 -Wall")

find_package(Qt5Widgets)
find_package(Qwt)

set(CLIF_BUILD_QT TRUE)
set(CLIF_BUILD_QT_REASON "")

if (!Qt5Widgets_FOUND)
  set(CLIF_BUILD_QT FALSE)
  set(CLIF_BUILD_QT_REASON "[Qt5Widgets not found]")
endif()

if (!QWT_FOUND)
  set(CLIF_BUILD_QT FALSE)
  set(CLIF_BUILD_QT_REASON "${CLIF_BUILD_QT_REASON}[Qwt not found]")
endif()

set(CLIF_BUILD_QT ${CLIF_BUILD_QT} PARENT_SCOPE)
set(CLIF_BUILD_QT_REASON ${CLIF_BUILD_QT_REASON} PARENT_SCOPE)

if(CLIF_BUILD_QT)
  set(CMAKE_AUTOMOC ON)
  set(CMAKE_INCLUDE_CURRENT_DIR ON)

  include_directories(${Qt5Widgets_INCLUDE_DIRS})
  include_directories(${QWT_INCLUDE_DIR})

  add_definitions(-DWTF_HACK_QT_EXPORT)
  add_library(clif-qt SHARED clifscaledimageview.cpp clifepiview.cpp clif_qt.cpp)

  #TODO add clif
  target_link_libraries(clif-qt ${Qt5Widgets_LIBRARIES} ${QWT_LIBRARY})
endif()
