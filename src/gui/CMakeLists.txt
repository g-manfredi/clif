cmake_minimum_required (VERSION 2.6)
project(clif-qt)

dep_lists_init()



dep_lists_append(clif)
dep_lists_append(Qt5Widgets Qt5Widgets_INCLUDE_DIRS Qt5Widgets_LIBRARY_DIRS Qt5Widgets_LIBRARIES)
dep_lists_append(Qt5Core Qt5Core_INCLUDE_DIRS Qt5Core_LIBRARY_DIRS Qt5Core_LIBRARIES)
dep_lists_append(Qt5Network Qt5Network_INCLUDE_DIRS Qt5Network_LIBRARY_DIRS Qt5Network_LIBRARIES)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(${PNU}_HEADERS clif_qt.hpp clifepiview.hpp clifscaledimageview.hpp clifviewcaller.hpp clifstoreview.hpp)
set(${PNU}_EXPORT_LIBS clif-qt)

dep_lists_pkg_search()
dep_lists_prepare_env()

add_library(clif-qt clifscaledimageview.cpp clifepiview.cpp clif_qt.cpp clifstoreview.cpp clifviewcaller.cpp)
target_link_libraries(clif-qt PUBLIC ${${PNU}_PRIVATE_LIB})
target_link_libraries(clif-qt PRIVATE ${${PNU}_LIB})

dep_lists_export_local()

# cmake_minimum_required (VERSION 2.6)
# project(clif-qt)
# 
# cmake_policy(VERSION 2.8.11)
# 
# set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
# 
# #set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -g -std=c++11 -Wall")
# 
# if(CLIF_BUILD_QT)
#   find_package(Qt5Core REQUIRED)
#   find_package(Qt5Network REQUIRED)
#   
#   add_definitions(${Qt5Widgets_DEFINITIONS})
#   add_definitions(${Qt5Network_DEFINITIONS})
# 
#   set(CMAKE_AUTOMOC ON)
#   set(CMAKE_INCLUDE_CURRENT_DIR ON)
# 
#   include_directories(${Qt5Widgets_INCLUDE_DIRS})
#   include_directories(${Qt5Network_INCLUDE_DIRS})
# 
#   add_library(${PROJECT_NAME} ${CLIF_BUILD_TYPE} clifscaledimageview.cpp clifepiview.cpp clif_qt.cpp clifstoreview.cpp clifviewcaller.cpp)
# 
#   #TODO add clif
#   #target_link_libraries(${PROJECT_NAME} ${CLIF_LINK_LIBRARIES} ${Qt5Core_LIBRARIES} ${Qt5Widgets_LIBRARIES} ${Qt5Network_LIBRARIES})
#   
#   target_link_libraries(${PROJECT_NAME} PUBLIC ${CLIF_LINK_LIBRARIES} ${${PNU}_LIB} ${Qt5Core_LIBRARIES} ${Qt5Widgets_LIBRARIES} ${Qt5Network_LIBRARIES})
#   target_link_libraries(${PROJECT_NAME} PRIVATE ${CLIF_PRIVATE_LINK_LIBRARIES} ${${PNU}_PRIVATE_LIB})
#   
#   set(CLIF_BUILD_INCLUDE_DIRS ${CLIF_BUILD_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
#   
#   if(NOT CLIF_COMPILER_MSVC)
#     set(CLIF_LINK_LIBRARIES ${CLIF_LINK_LIBRARIES} clif-qt PARENT_SCOPE)
#   else()
#     set(CLIF_LINK_LIBRARIES ${CLIF_LINK_LIBRARIES} optimized;clif-qt debug;clif-qtd PARENT_SCOPE)
#   endif()
# 
#   list(APPEND CLIF_QT_HEADERS clifscaledimageview.hpp clifepiview.hpp clif_qt.hpp clifviewcaller.hpp)
# 
#   foreach(header ${CLIF_QT_HEADERS})
#     if (IS_ABSOLUTE ${header})
#       list(APPEND CLIF_FULLPATH_HEADERS "${header}")
#     else()
#       list(APPEND CLIF_FULLPATH_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/${header}")
#     endif()
#   endforeach()
# 
#   set(CLIF_HEADERS "${CLIF_HEADERS}" PARENT_SCOPE)
#   set(CLIF_FULLPATH_HEADERS "${CLIF_FULLPATH_HEADERS}" PARENT_SCOPE)
#   
#   install(TARGETS ${PROJECT_NAME} DESTINATION lib)
# endif()
