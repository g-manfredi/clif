cmake_minimum_required (VERSION 2.8)
project(fnmatch)

include(../flexdeplists/FlexDepLists.cmake)
dep_lists_init()
dep_lists_pkg_search()

set(FNMATCH_HEADERS fnmatch.h)

dep_lists_prepare_env()

add_library(fnmatch fnmatch.c)

set(FNMATCH_EXPORT_LIBS fnmatch)

dep_lists_export_local(DIRECT_HEADER_INCLUDE)
