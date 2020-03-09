# These functions add as a replacement for the LMI internal ones

set(_THIS_DIR "${CMAKE_CURRENT_LIST_DIR}")

function(asyncly_add_google_test target)
  add_executable(${target} ${ARGN})
  set_property(TARGET ${target} APPEND PROPERTY LINK_LIBRARIES "GTest::gmock")
  add_test(NAME ${target} COMMAND ${target})
  set_property(TEST ${target} APPEND PROPERTY LABELS Benchmark)
endfunction()

function(asyncly_add_google_benchmark target)
  add_executable(${target} ${ARGN})
  set_property(TARGET ${target} APPEND PROPERTY LINK_LIBRARIES "benchmark::benchmark")
  add_test(NAME ${target} COMMAND ${target})
endfunction()

function(asyncly_add_self_contained_header_test target_name include_root)
  asyncly_add_self_contained_header_check(
    TARGET ${target_name}
    ROOT ${include_root}
    GLOB_RECURSE "*")
endfunction()

function(asyncly_add_self_contained_header_check)
  set(_options)
  set(_oneValueArgs TARGET ROOT)
  set(_multiValueArgs GLOB_RECURSE)

  cmake_parse_arguments(_ARG "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})

  if(NOT _ARG_TARGET OR NOT _ARG_ROOT OR NOT _ARG_GLOB_RECURSE)
    message(FATAL_ERROR "Missing arguments ${ARGN}")
  endif()

  get_filename_component(_absolute_root "${_ARG_ROOT}" ABSOLUTE)

  # Glob all requested files
  set(_all_headers)
  foreach(_glob IN LISTS _ARG_GLOB_RECURSE)
    file(GLOB_RECURSE _glob_headers LIST_DIRECTORIES FALSE RELATIVE "${_absolute_root}" "${_absolute_root}/${_glob}")
    list(APPEND _all_headers ${_glob_headers})
  endforeach()

  # add check for all headers

  unset(_sources)

  foreach(HEADER IN LISTS _all_headers)
    string(MAKE_C_IDENTIFIER "${HEADER}" _short_name)
    set(_source "${CMAKE_CURRENT_BINARY_DIR}/${_short_name}.cpp")
    set(UNIQUE_PART "${_short_name}")
    configure_file("${_THIS_DIR}/test_headercompile.cpp.in" "${_source}" @ONLY)
    list(APPEND _sources "${_source}")
  endforeach()

  add_library(${_ARG_TARGET} STATIC ${_sources})
  target_include_directories(${_ARG_TARGET} PRIVATE "${_absolute_root}")
endfunction()

function(asyncly_install_export)
  set(options )
  set(oneValueArgs DESTINATION EXPORT NAMESPACE)
  set(multiValueArgs )

  cmake_parse_arguments(_ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT _ARGS_DESTINATION)
    message(FATAL_ERROR "Required argument DESTINATION missing in asyncly_install_export() call")
  endif()

  if(NOT _ARGS_EXPORT)
    message(FATAL_ERROR "Required argument EXPORT missing in asyncly_install_export() call")
  endif()

  if(NOT _ARGS_NAMESPACE)
    message(FATAL_ERROR "Required argument NAMESPACE missing in asyncly_install_export() call")
  endif()

  install(
    EXPORT ${_ARGS_EXPORT}
    DESTINATION ${_ARGS_DESTINATION}
    NAMESPACE ${_ARGS_NAMESPACE}
  )

  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/CMake/${PROJECT_NAME}-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
    INSTALL_DESTINATION ${_ARGS_DESTINATION}
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR
  )

  install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
    DESTINATION ${_ARGS_DESTINATION}
  )
endfunction()
