cmake_minimum_required (VERSION 2.6)
project(clif-bin)

cmake_policy(VERSION 2.8.11)

dep_lists_init()
dep_lists_append(clif)
dep_lists_pkg_search()
dep_lists_prepare_env()

add_executable(clif-bin clif.cpp)
add_executable(clifinfo clifinfo.cpp)
add_executable(clif_image_attribute clif_image_attribute.cpp)
add_executable(resolve_test resolve_test.cpp)
add_executable(h5reshape h5reshape.cpp)
add_executable(h5skip h5skip.cpp)
add_executable(h5resize h5resize.cpp)
add_executable(h5remove h5remove.cpp)
add_executable(clifvis clifvis.cpp)
add_executable(clifprocstore clifprocstore.cpp)

if (CLIF_COMPILER_MSVC)
  #link_directories("C:/Users/Test/Desktop/UniHD_OpenLF/glew-1.13.0/lib/Release/Win32")
  target_link_libraries(clif-bin ${CLIF-BIN_LIBRARIES})
else()
  set_target_properties(clif-bin PROPERTIES OUTPUT_NAME clif)
  target_link_libraries(clif-bin ${CLIF-BIN_LIBRARIES})
endif()


target_link_libraries(clifinfo ${CLIF-BIN_LIBRARIES})
target_link_libraries(clif_image_attribute ${CLIF-BIN_LIBRARIES})
target_link_libraries(resolve_test ${CLIF-BIN_LIBRARIES})


target_link_libraries(h5reshape ${CLIF-BIN_LIBRARIES})
target_link_libraries(h5resize ${CLIF-BIN_LIBRARIES})
target_link_libraries(h5skip ${CLIF-BIN_LIBRARIES})
target_link_libraries(h5remove ${CLIF-BIN_LIBRARIES})
target_link_libraries(clifprocstore ${CLIF-BIN_LIBRARIES})

target_link_libraries(clifvis ${CLIF-BIN_LIBRARIES})
