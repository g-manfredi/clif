cmake_minimum_required(VERSION 2.8.11)
project(clifview)

#set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -g -std=c++11 -Wall")

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)


if (CLIF_BUILD_QT)
  remove_definitions(-DCLIF_EXPORT_LIBRARY)

  #no idea why we need this here, should be inherited from clif-qt?
  find_package(Qt5Core)

  file(GLOB srcs *.cpp)
  file(GLOB hdrs *.h)

  add_executable(clifview ${srcs})
  target_link_libraries(clifview clif-qt ${Qt5Core_LIBRARIES})

  install(TARGETS clifview DESTINATION bin )
endif()
