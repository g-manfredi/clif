cmake_minimum_required (VERSION 2.6)
project(cliini)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -std=c99")

add_library(cliini SHARED cliini.c)

add_executable(cliini_test test.c)
add_executable(cliini_initest testini.c)
add_executable(cliini_types testini_types.c)

target_link_libraries(cliini_test cliini)
target_link_libraries(cliini_initest cliini)
target_link_libraries(cliini_types cliini)

install(TARGETS cliini
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/static)