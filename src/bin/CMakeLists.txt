cmake_minimum_required (VERSION 2.6)
project(clif-bin)

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -g -O3 -fno-omit-frame-pointer -std=c++11 -Wall -fopenmp")

add_executable(clif-bin clif.cpp)
add_executable(clifinfo clifinfo.cpp)
#add_executable(clifsweep clifsweep.cpp)

SET_TARGET_PROPERTIES(clif-bin
  PROPERTIES OUTPUT_NAME clif)

target_link_libraries(clif-bin clif cliini ${OpenCV_LIBRARIES})
target_link_libraries(clifinfo clif cliini ${OpenCV_LIBRARIES})
#target_link_libraries(clifsweep clif cliini ${OpenCV_LIBRARIES})
