# workaround for wrong default installations 
# see https://github.com/glfw/glfw/issues/483
# TODO we should iterate cmaek search paths and search for glfw/...

if (EXISTS /usr/lib/cmake/glfw/glfw3Config.cmake)
  include(/usr/lib/cmake/glfw/glfw3Config.cmake)
  set(glfw3_FOUND TRUE)
endif()
