cmake_minimum_required (VERSION 2.6)
project(clif)

add_executable(clif-bin clif.cpp)

SET_TARGET_PROPERTIES(clif-bin
  PROPERTIES OUTPUT_NAME clif)

target_link_libraries(clif-bin clif cliini ${OpenCV_LIBRARIES})
