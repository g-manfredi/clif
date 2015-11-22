cmake_minimum_required (VERSION 2.6)
project(clif-qt)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

#set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -g -std=c++11 -Wall")

if(CLIF_BUILD_QT)
  find_package(Qt5Core)
  find_package(Qt5Network)

  set(CMAKE_AUTOMOC ON)
  set(CMAKE_INCLUDE_CURRENT_DIR ON)

  include_directories(${Qt5Widgets_INCLUDE_DIRS})
  include_directories(${Qt5Network_INCLUDE_DIRS})

  add_library(${PROJECT_NAME} ${CLIF_BUILD_TYPE} clifscaledimageview.cpp clifepiview.cpp clif_qt.cpp clifstoreview.cpp clifviewcaller.cpp)

  #TODO add clif
  target_link_libraries(${PROJECT_NAME} ${CLIF_LIBRARIES} ${Qt5Core_LIBRARIES} ${Qt5Widgets_LIBRARIES} ${Qt5Network_LIBRARIES})
  
  set(CLIF_LINK_LIBRARIES ${CLIF_LINK_LIBRARIES} ${PROJECT_NAME} PARENT_SCOPE)
  set(CLIF_BUILD_INCLUDE_DIRS ${CLIF_BUILD_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
  set(CLIF_BUILD_LINK_DIRS ${CLIF_BUILD_LINK_DIRS} ${CMAKE_CURRENT_BINARY_DIR} PARENT_SCOPE)

  list(APPEND CLIF_QT_HEADERS clifscaledimageview.hpp clifepiview.hpp clif_qt.hpp clifviewcaller.hpp)

  foreach(header ${CLIF_QT_HEADERS})
    if (IS_ABSOLUTE ${header})
      list(APPEND CLIF_FULLPATH_HEADERS "${header}")
    else()
      list(APPEND CLIF_FULLPATH_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/${header}")
    endif()
  endforeach()

  set(CLIF_HEADERS "${CLIF_HEADERS}" PARENT_SCOPE)
  set(CLIF_FULLPATH_HEADERS "${CLIF_FULLPATH_HEADERS}" PARENT_SCOPE)
  
  install(TARGETS ${PROJECT_NAME} DESTINATION lib)
endif()
