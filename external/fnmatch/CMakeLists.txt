cmake_minimum_required (VERSION 2.6)
project(fnmatch)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -std=c99")

add_library(fnmatch SHARED fnmatch.c)

install(TARGETS fnmatch
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/static)
install(FILES fnmatch.h DESTINATION include/clif)