cmake_minimum_required (VERSION 2.6)
project(cliini)

if (CLIF_COMPILER_MSVC)
  #use c++ compiler
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /TP")
else ()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -std=c99 -fPIC")
endif()

add_library(cliini cliini.c)

if (NOT CLIF_COMPILER_MSVC)
  add_executable(cliini_test test.c)
  add_executable(cliini_initest testini.c)
  add_executable(cliini_types testini_types.c)
endif()

target_link_libraries(cliini ${CLIF_FNMATCH_LIBRARIES})

if (NOT CLIF_COMPILER_MSVC)
  target_link_libraries(cliini_test cliini)
  target_link_libraries(cliini_initest cliini)
  target_link_libraries(cliini_types cliini)
endif()

install(TARGETS cliini
		EXPORT ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/static)
		
install(EXPORT ${PROJECT_NAME} DESTINATION "lib/cmake/clif" COMPONENT dev)
