macro(dep_lists_clean_list LIST)
  if (DEFINED ${LIST})
    list(REMOVE_DUPLICATES ${LIST})
  endif()
endmacro()

macro(dep_lists_check_find PACKAGE RET)
  find_package(${PACKAGE} QUIET)
  
  string(TOLOWER ${PACKAGE} PKG_LOW)
  
  #check if pkg was found (various conventions...)
  pkg_found(${PACKAGE} FOUND)
  if (FOUND)
    message("${PACKAGE} - found")
    set(${RET} TRUE)
    list(APPEND ${PNU}_DEFINES ${RET})
  else()
    #message("${${pkg_up}_FOUND}"
    #message("did not find ${PACKAGE}, adding cmake/${PKG_LOW} to CMAKE_MODULE_PATH")
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/find/${PKG_LOW})
    find_package(${PACKAGE} QUIET)
    pkg_found(${PACKAGE} FOUND)
    if (FOUND)
      message("${PACKAGE} - found using included Find${PACKAGE}.cmake")
      set(${RET} TRUE)
      list(APPEND ${PNU}_DEFINES ${RET})
    else()
      message("${PACKAGE} - missing")
      set(${RET} FALSE)
    endif()
  endif()
endmacro()

macro(dep_lists_pkg_search)
  
  if (ARGV0)
    set(_FDP_PNU ${ARGV0})
  else()
    string(TOUPPER ${PROJECT_NAME} _FDP_PNU)
  endif()

  message("")
  message("searching dependencies for ${_FDP_PNU}:")

  foreach(PACKAGE ${${_FDP_PNU}_PKG_OPT})
    string(TOUPPER ${PACKAGE} PKG_UP)
    dep_lists_check_find(${PACKAGE} ${_FDP_PNU}_WITH_${PKG_UP})
    if (NOT ${${_FDP_PNU}_WITH_${PKG_UP}})
      list(APPEND ${_FDP_PNU}_MISSING_OPTIONAL ${PACKAGE})
    else()
      list(APPEND ${_FDP_PNU}_EXPORT_DEPS ${PACKAGE})
    endif()
  endforeach()

  foreach(PACKAGE ${${_FDP_PNU}_PRIVATE_PKG_OPT})
    string(TOUPPER ${PACKAGE} PKG_UP)
    dep_lists_check_find(${PACKAGE} ${_FDP_PNU}_WITH_${PKG_UP})
    if (NOT ${${_FDP_PNU}_WITH_${PKG_UP}})
      list(APPEND ${_FDP_PNU}_MISSING_OPTIONAL ${PACKAGE})
    endif()
  endforeach()

  foreach(PACKAGE ${${_FDP_PNU}_PKG})
    string(TOUPPER ${PACKAGE} PKG_UP)
    dep_lists_check_find(${PACKAGE} ${_FDP_PNU}_WITH_${PKG_UP})
    if (NOT ${${_FDP_PNU}_WITH_${PKG_UP}})
      list(APPEND ${_FDP_PNU}_MISSING_REQUIRED ${PACKAGE})
    else()
      list(APPEND ${_FDP_PNU}_EXPORT_DEPS ${PACKAGE})
    endif()
  endforeach()

  foreach(PACKAGE ${${_FDP_PNU}_PRIVATE_PKG})
    string(TOUPPER ${PACKAGE} PKG_UP)
    dep_lists_check_find(${PACKAGE} ${_FDP_PNU}_WITH_${PKG_UP})
    if (NOT ${${_FDP_PNU}_WITH_${PKG_UP}})
      list(APPEND ${_FDP_PNU}_MISSING_REQUIRED ${PACKAGE})
    endif()
  endforeach()

  message("")
    
  if (${_FDP_PNU}_MISSING_OPTIONAL)
    message("missing OPTIONAL packages:")
    foreach(PACKAGE ${${_FDP_PNU}_MISSING_OPTIONAL})
      message(${PACKAGE})
    endforeach()
    message("")
  endif()
  
  if (${_FDP_PNU}_MISSING_REQUIRED)
    message("missing REQUIRED packages:")
    foreach(PACKAGE ${${_FDP_PNU}_MISSING_REQUIRED})
      message(${PACKAGE})
    endforeach()
    message("")
    if (NOT DEP_LISTS_SOFT_FAIL)
      message(FATAL_ERROR "required package(s) not found, exiting.")
    else()
      return()
    endif()
  endif()
endmacro(dep_lists_pkg_search)

macro(dep_lists_opt_get _FDP_LIST _FDP_IDX _FDP_OUT)
  list(LENGTH ${_FDP_LIST} _FDP_LIST_L)
  if (${_FDP_IDX} LESS _FDP_LIST_L)
    list(GET ${_FDP_LIST} _FDP_IDX ${_FDP_OUT})
  else()
    set(${_FDP_OUT} "")
  endif()
endmacro(dep_lists_opt_get)

macro(dep_lists_inc_link)
  
  if (ARGV0)
    set(_FDP_PNU ${ARGV0})
  else()
    string(TOUPPER ${PROJECT_NAME} _FDP_PNU)
  endif()

  #####################################################
  ## SET INCLUDES, LIBS, ... (public)
  #####################################################
  foreach(INCLUDE ${${_FDP_PNU}_PKG_INC})
    if (NOT ("${${INCLUDE}}" MATCHES ".*-NOTFOUND"))
      list(APPEND ${_FDP_PNU}_INC ${${INCLUDE}})
    else()
      # FIXME remove including in deps!
    endif()
  endforeach()
  dep_lists_clean_list(${_FDP_PNU}_INC)

  foreach(LIBDIR ${${_FDP_PNU}_PKG_LINK})
    if (NOT ("${${LIBDIR}}" MATCHES ".*-NOTFOUND"))
      list(APPEND ${_FDP_PNU}_LINK ${${LIBDIR}})
    else()
      # FIXME remove including in deps!
    endif()
  endforeach()
  dep_lists_clean_list(${_FDP_PNU}_LINK)

  foreach(LIB ${${_FDP_PNU}_PKG_LIB})
    if (NOT ("${${LIB}}" MATCHES ".*-NOTFOUND"))
      list(APPEND ${_FDP_PNU}_LIB ${${LIB}})
    else()
      # FIXME remove including in deps!
    endif()
  endforeach()
  dep_lists_clean_list(${_FDP_PNU}_LIB)

  #####################################################
  ## SET INCLUDES, LIBS, ... (private)
  #####################################################

  foreach(INCLUDE ${${_FDP_PNU}_PRIVATE_PKG_INC})
    if (NOT ("${${INCLUDE}}" MATCHES ".*-NOTFOUND"))
      list(APPEND ${_FDP_PNU}_PRIVATE_INC ${${INCLUDE}})
    else()
      # FIXME remove including in deps!
    endif()
  endforeach()
  dep_lists_clean_list(${_FDP_PNU}_PRIVATE_INC)

  foreach(LIBDIR ${${_FDP_PNU}_PRIVATE_PKG_LINK})
    if (NOT ("${${LIBDIR}}" MATCHES ".*-NOTFOUND"))
      list(APPEND ${_FDP_PNU}_PRIVATE_LINK ${${LIBDIR}})
    else()
      # FIXME remove including in deps!
    endif()
  endforeach()
  dep_lists_clean_list(${_FDP_PNU}_PRIVATE_LINK)

  foreach(LIB ${${_FDP_PNU}_PRIVATE_PKG_LIB})
    if (NOT ("${${LIB}}" MATCHES ".*-NOTFOUND"))
      list(APPEND ${_FDP_PNU}_PRIVATE_LIB ${${LIB}})
    else()
      # FIXME remove including in deps!
    endif()
  endforeach()
  dep_lists_clean_list(${_FDP_PNU}_PRIVATE_LIB)


  #####################################################
  ## actually inc/link DIRS (from above)
  #####################################################
  include_directories(${${_FDP_PNU}_INC} ${${_FDP_PNU}_PRIVATE_INC})
  link_directories(${${_FDP_PNU}_LINK} ${${_FDP_PNU}_PRIVATE_LINK})
endmacro(dep_lists_inc_link)

macro(dep_lists_append _FDP_NAME)
  message("parse dep_lists_append for ${_FDP_NAME}")
  set(dep_lists_append_UNPARSED_ARGUMENTS "")
  cmake_parse_arguments(dep_lists_append "OPTIONAL;PRIVATE" "PREFIX" "" ${ARGN})
  
  string(TOUPPER ${_FDP_NAME} _FDP_NAME_UPPER)
  
  #output prefix (normally upper case project name)
  if (dep_lists_append_PREFIX)
    set(_FDP_PREFIX ${dep_lists_append_PREFIX})
  else()
    string(TOUPPER ${PROJECT_NAME} _FDP_PREFIX)
  endif()
  
  dep_lists_opt_get(dep_lists_append_UNPARSED_ARGUMENTS 0 _FDP_A0)
  dep_lists_opt_get(dep_lists_append_UNPARSED_ARGUMENTS 1 _FDP_A1)
  dep_lists_opt_get(dep_lists_append_UNPARSED_ARGUMENTS 2 _FDP_A2)
  
  if (_FDP_A0)
    set(_FDP_INC ${_FDP_A0})
  else()
    set(_FDP_INC ${_FDP_NAME_UPPER}_INCLUDE_DIRS)
  endif()
  
  if (_FDP_A1)
    set(_FDP_LINK ${_FDP_A1})
  else()
    set(_FDP_LINK ${_FDP_NAME_UPPER}_LIBRARY_DIRS)
  endif()
  
  if (_FDP_A2)
    set(_FDP_LIB ${_FDP_A2})
  else()
    set(_FDP_LIB ${_FDP_NAME_UPPER}_LIBRARIES)
  endif()
  
  message("PNU: ${_FDP_PREFIX}")
  message("NAME: ${_FDP_NAME}")
  message("INC: ${_FDP_INC}")
  message("LINK: ${_FDP_LINK}")
  message("LIB: ${_FDP_LIB}")
  message("OPTIONAL: ${dep_lists_append_OPTIONAL}")
  message("PRIVATE: ${dep_lists_append_PRIVATE}")
  
  if (dep_lists_append_PRIVATE)
    set(_FDP_PREFIX ${_FDP_PREFIX}_PRIVATE)
  endif()
  set(_FDP_PREFIX ${_FDP_PREFIX}_PKG)
  
  if (dep_lists_append_OPTIONAL)
    list(APPEND ${_FDP_PREFIX}_OPT ${_FDP_NAME})
  else()
    list(APPEND ${_FDP_PREFIX} ${_FDP_NAME})
  endif()
  
  list(APPEND ${_FDP_PREFIX}_INC  "${_FDP_INC}")
  list(APPEND ${_FDP_PREFIX}_LINK "${_FDP_LINK}")
  list(APPEND ${_FDP_PREFIX}_LIB  "${_FDP_LIB}")
endmacro(dep_lists_append)

macro(dep_lists_cleanup)
  file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake")
endmacro()
