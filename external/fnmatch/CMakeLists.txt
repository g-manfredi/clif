cmake_minimum_required (VERSION 2.6)
project(fnmatch)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -std=c99")

#build as c++ with msvc
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /TP")
#add_definitions(/D__STDC__)


add_library(fnmatch fnmatch.c)

install(TARGETS fnmatch
		EXPORT ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(FILES fnmatch.h DESTINATION include)

install(EXPORT ${PROJECT_NAME} DESTINATION "lib/cmake/clif" COMPONENT dev)
