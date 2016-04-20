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
