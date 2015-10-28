cmake_minimum_required(VERSION 2.8.11)

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -g -std=c++11 -Wall -fPIC")

project(clifview)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5Widgets REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem system)

include_directories(${Qt5Widgets_INCLUDE_DIRS})
include_directories(include)
add_definitions(${Qt5Widgets_DEFINITIONS})

#Find all files of the procject in the related directory / subdirectories (necessary for linking in the executable)
file(GLOB srcs *.cpp *.cpp)
file(GLOB hdrs include/*.h)

#QT5_ADD_RESOURCES(ResourceSources ${Resources})

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHc /EHsc")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

add_executable (${PROJECT_NAME} ${srcs} ${hdrs} ${Boost_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} clif-qt ${Qt5Widgets_LIBRARIES} ${Boost_LIBRARIES})

install(TARGETS clifview DESTINATION bin )


#cmake_minimum_required(VERSION 2.8.11)
#project(clifview)

#set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -g -std=c++11 -Wall")

# Instruct CMake to run moc automatically when needed.
#set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTOUIC ON)
#set(CMAKE_INCLUDE_CURRENT_DIR ON)


#if (CLIF_BUILD_QT)
#  remove_definitions(-DCLIF_EXPORT_LIBRARY)

  #no idea why we need this here, should be inherited from clif-qt?
#  find_package(Qt5Core)

#  file(GLOB srcs *.cpp)
#  file(GLOB hdrs *.h)

#  add_executable(clifview ${srcs})
#  target_link_libraries(clifview clif-qt ${Qt5Core_LIBRARIES})

#  install(TARGETS clifview DESTINATION bin )
#endif()
