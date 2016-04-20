cmake_minimum_required(VERSION 2.8.12)

project(clifview)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

dep_lists_init()

dep_lists_append(clif-qt)

dep_lists_pkg_search()
dep_lists_prepare_env()

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

add_executable (clifview ${srcs} ${hdrs})
target_link_libraries(clifview PUBLIC ${${PNU}_LIB})
target_link_libraries(clifview PRIVATE ${${PNU}_PRIVATE_LIB})


