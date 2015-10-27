cmake_minimum_required (VERSION 2.6)
project(clif-qt)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

#set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -g -std=c++11 -Wall")

find_package(Qt5Widgets QUIET)

set(CLIF_BUILD_QT TRUE)
set(CLIF_BUILD_QT_REASON "")

if (Qt5Widgets_FOUND)
else()
  set(CLIF_BUILD_QT FALSE)
  set(CLIF_BUILD_QT_REASON "[Qt5Widgets not found]")
endif()

set(CLIF_BUILD_QT ${CLIF_BUILD_QT} PARENT_SCOPE)
set(CLIF_BUILD_QT_REASON ${CLIF_BUILD_QT_REASON} PARENT_SCOPE)

if(CLIF_BUILD_QT)
  find_package(Qt5Core)

  set(CMAKE_AUTOMOC ON)
  set(CMAKE_INCLUDE_CURRENT_DIR ON)

  include_directories(${Qt5Widgets_INCLUDE_DIRS})

  add_library(clif-qt ${CLIF_BUILD_TYPE} clifscaledimageview.cpp clifepiview.cpp clif_qt.cpp)

  #TODO add clif
  target_link_libraries(clif-qt ${CLIF_LIBRARIES} ${Qt5Core_LIBRARIES} ${Qt5Widgets_LIBRARIES})
  
  set(CLIF_LINK_LIBRARIES ${CLIF_LINK_LIBRARIES} clif-qt PARENT_SCOPE)
  set(CLIF_BUILD_INCLUDE_DIRS ${CLIF_BUILD_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
  set(CLIF_BUILD_LINK_DIRS ${CLIF_BUILD_LINK_DIRS} ${CMAKE_CURRENT_BINARY_DIR} PARENT_SCOPE)

  list(APPEND CLIF_QT_HEADERS clifscaledimageview.hpp clifepiview.hpp clif_qt.hpp)

  foreach(header ${CLIF_QT_HEADERS})
    if (IS_ABSOLUTE ${header})
      list(APPEND CLIF_FULLPATH_HEADERS "${header}")
    else()
      list(APPEND CLIF_FULLPATH_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/${header}")
    endif()
  endforeach()

  set(CLIF_HEADERS "${CLIF_HEADERS}" PARENT_SCOPE)
  set(CLIF_FULLPATH_HEADERS "${CLIF_FULLPATH_HEADERS}" PARENT_SCOPE)
endif()
