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
    list(APPEND ${PNU}_FEATURES ${RET})
  else()
    #message("${${pkg_up}_FOUND}"
    #message("did not find ${PACKAGE}, adding cmake/${PKG_LOW} to CMAKE_MODULE_PATH")
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/find/${PKG_LOW})
    find_package(${PACKAGE} QUIET)
    pkg_found(${PACKAGE} FOUND)
    if (FOUND)
      message("${PACKAGE} - found using included Find${PACKAGE}.cmake")
      set(${RET} TRUE)
      list(APPEND ${PNU}_FEATURES ${RET})
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

macro(dep_lists_prepare_env)
  
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
  
  foreach(F ${${_FDP_PNU}_FEATURES})
    message("has feature ${F}")
    set(${F} true)
    string(REPLACE "-" "_" F ${F})
    add_definitions(-D${F})
  endforeach()
endmacro(dep_lists_prepare_env)

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
  
#   message("PNU: ${_FDP_PREFIX}")
#   message("NAME: ${_FDP_NAME}")
#   message("INC: ${_FDP_INC}")
#   message("LINK: ${_FDP_LINK}")
#   message("LIB: ${_FDP_LIB}")
#   message("OPTIONAL: ${dep_lists_append_OPTIONAL}")
#   message("PRIVATE: ${dep_lists_append_PRIVATE}")
  
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

# install ${${PNU}_HEADERS} into CMAKE_CURRENT_BINARY_DIR/${PNL}/
# FIXME only works for relative path headers atm!
function(dep_lists_export_local)

  set(dep_lists_append_UNPARSED_ARGUMENTS "")
  cmake_parse_arguments(dep_lists_append "" "HEADER_PREFIX" "" ${ARGN})
  dep_lists_opt_get(dep_lists_append_UNPARSED_ARGUMENTS 0 _FDP_A0)

  #project name prefix (for list names)
  if (_FDP_A0)
    set(_FDP_PNU ${_FDP_A0})
  else()
    string(TOUPPER ${PROJECT_NAME} _FDP_PNU)
  endif()
  
  string(TOLOWER ${_FDP_PNU} PNL)
  
  #output prefix (normally upper case project name)
  if (dep_lists_append_HEADER_PREFIX)
    set(_FDP_HEADER_PREFIX ${dep_lists_append_HEADER_PREFIX})
  else()
    string(TOLOWER ${_FDP_PNU} _FDP_HEADER_PREFIX)
  endif()
  
  add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/include/${_FDP_HEADER_PREFIX}/ COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/include/${_FDP_HEADER_PREFIX})
  set(_FPD_HEADER_DEPLIST ${CMAKE_CURRENT_BINARY_DIR}/include/${_FDP_HEADER_PREFIX})
                     
  foreach(_H ${${_FDP_PNU}_HEADERS})
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/include/${_FDP_HEADER_PREFIX}/${_H}
                       COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${_H} ${CMAKE_CURRENT_BINARY_DIR}/include/${_FDP_HEADER_PREFIX}/
                       DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_H})
    list(APPEND _FPD_HEADER_DEPLIST ${CMAKE_CURRENT_BINARY_DIR}/include/${_FDP_HEADER_PREFIX}/${_H})
  endforeach()
  
  add_custom_target(${PNL}-header-export ALL DEPENDS ${_FPD_HEADER_DEPLIST})

  include(CMakePackageConfigListHelpers)

  #####################################################
  ## ...Config.cmake generation
  #####################################################
  set(CMAKECONFIG_PKG ${${_FDP_PNU}_PKG})
  set(CMAKECONFIG_PKG_INC ${${_FDP_PNU}_PKG_INC})
  set(CMAKECONFIG_PKG_LINK ${${_FDP_PNU}_PKG_LINK})
  set(CMAKECONFIG_PKG_LIB ${${_FDP_PNU}_PKG_LIB})

  set(CMAKECONFIG_INC "include") #in build dir - headers were already copied above
  set(CMAKECONFIG_LIB ${${_FDP_PNU}_EXPORT_LIBS}) # our libs to link on import
  set(CMAKECONFIG_FEATURES ${${_FDP_PNU}_FEATURES})


  #####################################################
  ## local config.cmake
  #####################################################
  set(CMAKECONFIG_CMAKE_DIR ${CMAKE_CURRENT_BINARY_DIR})
  set(CMAKECONFIG_LINK ${CMAKE_CURRENT_BINARY_DIR})

  set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR})
  configure_package_config_file(cmake/projectConfig.cmake.in
                                "${CMAKECONFIG_CMAKE_DIR}/${PROJECT_NAME}Config.cmake"
                                INSTALL_DESTINATION "${CMAKECONFIG_CMAKE_DIR}"
                                PATH_VARS CMAKECONFIG_PKG CMAKECONFIG_PKG_INC CMAKECONFIG_PKG_LINK CMAKECONFIG_PKG_LIB CMAKECONFIG_INC CMAKECONFIG_LINK CMAKECONFIG_LIB)
  export(PACKAGE ${PROJECT_NAME})
endfunction()

# args: TITLE BOOLVAR [LENGTH]
function(msg_yesno TITLE BOOLVAR)

  cmake_parse_arguments(msg_yesno "NO_COLOR" "COLOR" "" ${ARGN})
  
  #output prefix (normally upper case project name)
  if (msg_yesno_NO_COLOR)
    set(_FDP_PREFIX ${dep_lists_append_PREFIX})
  else()
    string(TOUPPER ${PROJECT_NAME} _FDP_PREFIX)
  endif()
  
  dep_lists_opt_get(msg_yesno_UNPARSED_ARGUMENTS 0 LENGTH)
  
  
  string(ASCII 27 Esc)
  set(ColourReset "${Esc}[m")
  set(ColourBold  "${Esc}[1m")
  set(Red         "${Esc}[31m")
  set(Green       "${Esc}[32m")
  set(Yellow      "${Esc}[33m")
  set(Blue        "${Esc}[34m")
  set(Magenta     "${Esc}[35m")
  set(Cyan        "${Esc}[36m")
  set(White       "${Esc}[37m")
  set(BoldRed     "${Esc}[1;31m")
  set(BoldGreen   "${Esc}[1;32m")
  set(BoldYellow  "${Esc}[1;33m")
  set(BoldBlue    "${Esc}[1;34m")
  set(BoldMagenta "${Esc}[1;35m")
  set(BoldCyan    "${Esc}[1;36m")
  set(BoldWhite   "${Esc}[1;37m")
  
  if (NOT LENGTH)
    set(LENGTH 30)
  endif()
  
  set(FILLED "${TITLE}                                                               ")
  string(SUBSTRING ${FILLED} 0 ${LENGTH} CUT)
  
  if (msg_yesno_NO_COLOR)
    if (${BOOLVAR})
      message("${CUT}- yes")
    else()
      message("${CUT}- no")
    endif()
  elseif (msg_yesno_COLOR)
    if (${BOOLVAR})
      message("${${msg_yesno_COLOR}}${CUT}- yes${ColourReset}")
    else()
      message("${${msg_yesno_COLOR}}${CUT}- no${ColourReset}")
    endif()
  else()
    if (${BOOLVAR})
      message("${Green}${CUT}- yes${ColourReset}")
    else()
      message("${BoldRed}${CUT}- no${ColourReset}")
    endif()
  endif()
  
endfunction(msg_yesno)
