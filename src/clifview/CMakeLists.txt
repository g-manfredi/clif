cmake_minimum_required(VERSION 2.8.11)
project(clifview)

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -g -std=c++11 -Wall")

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

#find_package(clif REQUIRED)
find_package(Qt5Widgets REQUIRED)

include_directories(${CLIF_INCLUDE_DIRS})
include_directories(${Qt5Widgets_INCLUDE_DIRS})

#Find all files of the procject in the related directory / subdirectories (necessary for linking in the executable)
file(GLOB srcs *.cpp)
file(GLOB hdrs *.h)


link_directories(${CLIF_LIBRARY_DIRS})
link_directories(${Qt5Widgets_LIBRARY_DIRS})

################################################################################
######################## WINDOWS ###############################################
################################################################################
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  message("INFO: WINDOWS BUILD")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHc /EHsc")
endif()

################################################################################
######################## Linux #################################################
################################################################################
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  message( "INFO: LINUX BUILD" )
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

add_executable(clifview ${srcs} ${hdrs})
target_link_libraries(clifview ${CLIF_LIBRARIES} ${Qt5Widgets_LIBRARIES})

install(TARGETS clifview DESTINATION bin)
