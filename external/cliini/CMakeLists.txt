cmake_minimum_required (VERSION 2.6)
project(cliini)

if (CLIF_COMPILER_MSVC)
  #use c++ compiler
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /TP")
else ()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -std=c99 -fPIC")
endif()

add_library(cliini ${CLIF_BUILD_TYPE} cliini.c)

if (NOT CLIF_COMPILER_MSVC)
  add_executable(cliini_test test.c)
  add_executable(cliini_initest testini.c)
  add_executable(cliini_types testini_types.c)
endif()

target_link_libraries(cliini ${CLIF_LIBRARIES})

set(CLIF_LINK_LIBRARIES ${CLIF_LINK_LIBRARIES} cliini PARENT_SCOPE)
set(CLIF_BUILD_INCLUDE_DIRS ${CLIF_BUILD_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
set(CLIF_BUILD_LINK_DIRS ${CLIF_BUILD_LINK_DIRS} ${CMAKE_CURRENT_BINARY_DIR} PARENT_SCOPE)

set(CLIF_HEADERS ${CLIF_HEADERS} cliini.h PARENT_SCOPE)
set(CLIF_FULLPATH_HEADERS ${CLIF_FULLPATH_HEADERS} ${CMAKE_CURRENT_SOURCE_DIR}/cliini.h PARENT_SCOPE)

if (NOT CLIF_COMPILER_MSVC)
  target_link_libraries(cliini_test cliini)
  target_link_libraries(cliini_initest cliini)
  target_link_libraries(cliini_types cliini)
endif()

install(TARGETS cliini
        EXPORT clif
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
