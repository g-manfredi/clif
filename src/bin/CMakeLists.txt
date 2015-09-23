cmake_minimum_required (VERSION 2.6)
project(clif-bin)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3 -fno-omit-frame-pointer -std=c++11 -Wall -fopenmp -flto")

add_executable(loadbench loadbench.cpp)
add_executable(clif-bin clif.cpp)
add_executable(clifinfo clifinfo.cpp)
add_executable(linktest linktest.cpp)
add_executable(clifsweep clifsweep.cpp)
add_executable(clifsweep_tensor clifsweep_tensor.cpp)
add_executable(loadbench_vigra loadbench_vigra.cpp)
add_executable(loadbench_vigra2 loadbench_vigra2.cpp)
add_executable(clif_image_attribute clif_image_attribute.cpp)

SET_TARGET_PROPERTIES(clif-bin
  PROPERTIES OUTPUT_NAME clif)

target_link_libraries(loadbench clif cliini)
target_link_libraries(clif-bin clif cliini)
target_link_libraries(clifinfo clif cliini)
target_link_libraries(linktest clif cliini)
target_link_libraries(clifsweep clif cliini)
target_link_libraries(clifsweep_tensor clif cliini)
target_link_libraries(clif_image_attribute clif cliini)
target_link_libraries(loadbench_vigra clif_vigra cliini)
#FIXME vigraimpex?
target_link_libraries(loadbench_vigra2 clif_vigra cliini vigraimpex)
