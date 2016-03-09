cmake_minimum_required(VERSION 2.8.12)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -Wall -fPIC")

project(clifview)

if (CLIF_BUILD_QT)
   find_package(Qt5Widgets REQUIRED)
   find_package(Qt5Network REQUIRED)
   
   set(CMAKE_AUTOMOC ON)
   set(CMAKE_INCLUDE_CURRENT_DIR ON)
   
   include_directories(${Qt5Widgets_INCLUDE_DIRS})
   include_directories(${Qt5Network_INCLUDE_DIRS})
   include_directories(include)
   add_definitions(${Qt5Widgets_DEFINITIONS})
   add_definitions(${Qt5Network_DEFINITIONS})
   
  #Find all files of the procject in the related directory / subdirectories (necessary for linking in the executable)
   file(GLOB srcs *.cpp *.cpp)
   file(GLOB hdrs include/*.h)

   qt5_wrap_ui(hdrs clifview.ui)
   

   if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	   SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHc /EHsc")
   endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
   if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	   SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
   endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

   add_executable (${PROJECT_NAME} ${srcs} ${hdrs})
   target_link_libraries(${PROJECT_NAME} PRIVATE ${CLIF_LINK_LIBRARIES} ${Qt5Widgets_LIBRARIES} ${Qt5Network_LIBRARIES})
   
   target_link_libraries(${PROJECT_NAME} PUBLIC ${CLIF_LINK_LIBRARIES} ${${PNU}_LIB})
   target_link_libraries(${PROJECT_NAME} PRIVATE ${CLIF_PRIVATE_LINK_LIBRARIES} ${${PNU}_PRIVATE_LIB})

   install(TARGETS ${PROJECT_NAME} DESTINATION bin )
endif()
