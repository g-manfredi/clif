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

macro(dep_lists_pkg_search PNU)
  message("")
  message("searching dependencies for ${PNU}:")

  foreach(PACKAGE ${${PNU}_PKG_OPT})
    string(TOUPPER ${PACKAGE} PKG_UP)
    dep_lists_check_find(${PACKAGE} ${PNU}_WITH_${PKG_UP})
    if (NOT ${${PNU}_WITH_${PKG_UP}})
      list(APPEND ${PNU}_MISSING_OPTIONAL ${PACKAGE})
    else()
      list(APPEND ${PNU}_EXPORT_DEPS ${PACKAGE})
    endif()
  endforeach()

  foreach(PACKAGE ${${PNU}_PRIVATE_PKG_OPT})
    string(TOUPPER ${PACKAGE} PKG_UP)
    dep_lists_check_find(${PACKAGE} ${PNU}_WITH_${PKG_UP})
    if (NOT ${${PNU}_WITH_${PKG_UP}})
      list(APPEND ${PNU}_MISSING_OPTIONAL ${PACKAGE})
    endif()
  endforeach()

  foreach(PACKAGE ${${PNU}_PKG})
    string(TOUPPER ${PACKAGE} PKG_UP)
    dep_lists_check_find(${PACKAGE} ${PNU}_WITH_${PKG_UP})
    if (NOT ${${PNU}_WITH_${PKG_UP}})
      list(APPEND ${PNU}_MISSING_REQUIRED ${PACKAGE})
    else()
      list(APPEND ${PNU}_EXPORT_DEPS ${PACKAGE})
    endif()
  endforeach()

  foreach(PACKAGE ${${PNU}_PRIVATE_PKG})
    string(TOUPPER ${PACKAGE} PKG_UP)
    dep_lists_check_find(${PACKAGE} ${PNU}_WITH_${PKG_UP})
    if (NOT ${${PNU}_WITH_${PKG_UP}})
      list(APPEND ${PNU}_MISSING_REQUIRED ${PACKAGE})
    endif()
  endforeach()

  message("")
    
  if (${PNU}_MISSING_OPTIONAL)
    message("missing OPTIONAL packages:")
    foreach(PACKAGE ${${PNU}_MISSING_OPTIONAL})
      message(${PACKAGE})
    endforeach()
    message("")
  endif()
  
  if (${PNU}_MISSING_REQUIRED)
    message("missing REQUIRED packages:")
    foreach(PACKAGE ${${PNU}_MISSING_REQUIRED})
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

macro(dep_lists_inc_link PNU)
  #####################################################
  ## SET INCLUDES, LIBS, ... (public)
  #####################################################
  foreach(INCLUDE ${${PNU}_PKG_INC})
    if (NOT ("${${INCLUDE}}" MATCHES ".*-NOTFOUND"))
      list(APPEND ${PNU}_INC ${${INCLUDE}})
    else()
      # FIXME remove including in deps!
    endif()
  endforeach()
  dep_lists_clean_list(${PNU}_INC)

  foreach(LIBDIR ${${PNU}_PKG_LINK})
    if (NOT ("${${LIBDIR}}" MATCHES ".*-NOTFOUND"))
      list(APPEND ${PNU}_LINK ${${LIBDIR}})
    else()
      # FIXME remove including in deps!
    endif()
  endforeach()
  dep_lists_clean_list(${PNU}_LINK)

  foreach(LIB ${${PNU}_PKG_LIB})
    if (NOT ("${${LIB}}" MATCHES ".*-NOTFOUND"))
      list(APPEND ${PNU}_LIB ${${LIB}})
    else()
      # FIXME remove including in deps!
    endif()
  endforeach()
  dep_lists_clean_list(${PNU}_LIB)

  #####################################################
  ## SET INCLUDES, LIBS, ... (private)
  #####################################################

  foreach(INCLUDE ${${PNU}_PRIVATE_PKG_INC})
    if (NOT ("${${INCLUDE}}" MATCHES ".*-NOTFOUND"))
      list(APPEND ${PNU}_PRIVATE_INC ${${INCLUDE}})
    else()
      # FIXME remove including in deps!
    endif()
  endforeach()
  dep_lists_clean_list(${PNU}_PRIVATE_INC)

  foreach(LIBDIR ${${PNU}_PRIVATE_PKG_LINK})
    if (NOT ("${${LIBDIR}}" MATCHES ".*-NOTFOUND"))
      list(APPEND ${PNU}_PRIVATE_LINK ${${LIBDIR}})
    else()
      # FIXME remove including in deps!
    endif()
  endforeach()
  dep_lists_clean_list(${PNU}_PRIVATE_LINK)

  foreach(LIB ${${PNU}_PRIVATE_PKG_LIB})
    if (NOT ("${${LIB}}" MATCHES ".*-NOTFOUND"))
      list(APPEND ${PNU}_PRIVATE_LIB ${${LIB}})
    else()
      # FIXME remove including in deps!
    endif()
  endforeach()
  dep_lists_clean_list(${PNU}_PRIVATE_LIB)


  #####################################################
  ## actually inc/link DIRS (from above)
  #####################################################
  include_directories(${${PNU}_INC} ${${PNU}_PRIVATE_INC})
  link_directories(${${PNU}_LINK} ${${PNU}_PRIVATE_LINK})
endmacro(dep_lists_inc_link)
