# ZAP_GENERATE_CPP ===========================================================
#
# Example usage:
#   find_package(Zap)
#   zap_generate_cpp(ZAP_SRCS ZAP_HDRS schema.zap)
#   add_executable(foo main.cpp ${ZAP_SRCS})
#   target_link_libraries(foo PRIVATE Zap::zap-rpc)
#   target_include_directories(foo PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
#
#  If you are not using the RPC features you can use 'Zap::zap' in the
#  target_link_libraries call
#
# Configuration variables (optional):
#   ZAPC_OUTPUT_DIR
#       Directory to place compiled schema sources (default: CMAKE_CURRENT_BINARY_DIR).
#   ZAPC_IMPORT_DIRS
#       List of additional include directories for the schema compiler.
#       (ZAPC_SRC_PREFIX and ZAP_INCLUDE_DIRECTORY are always included.)
#   ZAPC_SRC_PREFIX
#       Schema file source prefix (default: CMAKE_CURRENT_SOURCE_DIR).
#   ZAPC_FLAGS
#       Additional flags to pass to the schema compiler.
#
# TODO: convert to cmake_parse_arguments

function(ZAP_GENERATE_CPP SOURCES HEADERS)
  if(NOT ARGN)
    message(SEND_ERROR "ZAP_GENERATE_CPP() called without any source files.")
  endif()
  set(tool_depends ${EMPTY_STRING})
  #Use cmake targets available
  if(TARGET zap_tool)
    if(NOT ZAP_EXECUTABLE)
      set(ZAP_EXECUTABLE $<TARGET_FILE:zap_tool>)
    endif()
    if(NOT ZAPC_CXX_EXECUTABLE)
      get_target_property(ZAPC_CXX_EXECUTABLE zapc_cpp ZAPC_CXX_EXECUTABLE)
    endif()
    if(NOT ZAP_INCLUDE_DIRECTORY)
      get_target_property(ZAP_INCLUDE_DIRECTORY zap_tool ZAP_INCLUDE_DIRECTORY)
    endif()
    list(APPEND tool_depends zap_tool zapc_cpp)
  endif()
  if(NOT ZAP_EXECUTABLE)
    message(SEND_ERROR "Could not locate zap executable (ZAP_EXECUTABLE).")
  endif()
  if(NOT ZAPC_CXX_EXECUTABLE)
    message(SEND_ERROR "Could not locate zapc-c++ executable (ZAPC_CXX_EXECUTABLE).")
  endif()
  if(NOT ZAP_INCLUDE_DIRECTORY)
    message(SEND_ERROR "Could not locate zap header files (ZAP_INCLUDE_DIRECTORY).")
  endif()

  if(DEFINED ZAPC_OUTPUT_DIR)
    # Prepend a ':' to get the format for the '-o' flag right
    set(output_dir ":${ZAPC_OUTPUT_DIR}")
  else()
    set(output_dir ":.")
  endif()

  if(NOT DEFINED ZAPC_SRC_PREFIX)
    set(ZAPC_SRC_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}")
  endif()
  get_filename_component(ZAPC_SRC_PREFIX "${ZAPC_SRC_PREFIX}" ABSOLUTE)

  # Default compiler includes. Note that in zap's own test usage of zap_generate_cpp(), these
  # two variables will end up evaluating to the same directory. However, it's difficult to
  # deduplicate them because if ZAP_INCLUDE_DIRECTORY came from the zap_tool target property,
  # then it must be a generator expression in order to handle usages in both the build tree and the
  # install tree. This vastly overcomplicates duplication detection, so the duplication doesn't seem
  # worth fixing.
  set(include_path -I "${ZAPC_SRC_PREFIX}" -I "${ZAP_INCLUDE_DIRECTORY}")

  if(DEFINED ZAPC_IMPORT_DIRS)
    # Append each directory as a series of '-I' flags in ${include_path}
    foreach(directory ${ZAPC_IMPORT_DIRS})
      get_filename_component(absolute_path "${directory}" ABSOLUTE)
      list(APPEND include_path -I "${absolute_path}")
    endforeach()
  endif()

  set(${SOURCES})
  set(${HEADERS})
  foreach(schema_file ${ARGN})
    get_filename_component(file_path "${schema_file}" ABSOLUTE)
    get_filename_component(file_dir "${file_path}" PATH)
    if(NOT EXISTS "${file_path}")
      message(FATAL_ERROR "Zap schema file '${file_path}' does not exist!")
    endif()

    # Figure out where the output files will go
    if (NOT DEFINED ZAPC_OUTPUT_DIR)
      set(ZAPC_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/")
    endif()
    # Output files are placed in ZAPC_OUTPUT_DIR, at a location as if they were
    # relative to ZAPC_SRC_PREFIX.
    string(LENGTH "${ZAPC_SRC_PREFIX}" prefix_len)
    string(SUBSTRING "${file_path}" 0 ${prefix_len} output_prefix)
    if(NOT "${ZAPC_SRC_PREFIX}" STREQUAL "${output_prefix}")
      message(SEND_ERROR "Could not determine output path for '${schema_file}' ('${file_path}') with source prefix '${ZAPC_SRC_PREFIX}' into '${ZAPC_OUTPUT_DIR}'.")
    endif()

    string(SUBSTRING "${file_path}" ${prefix_len} -1 output_path)
    set(output_base "${ZAPC_OUTPUT_DIR}${output_path}")

    add_custom_command(
      OUTPUT "${output_base}.c++" "${output_base}.h"
      COMMAND "${ZAP_EXECUTABLE}"
      ARGS compile
          -o ${ZAPC_CXX_EXECUTABLE}${output_dir}
          --src-prefix ${ZAPC_SRC_PREFIX}
          ${include_path}
          ${ZAPC_FLAGS}
          ${file_path}
      DEPENDS "${schema_file}" ${tool_depends}
      COMMENT "Compiling Zap schema ${schema_file}"
      VERBATIM
    )

    list(APPEND ${SOURCES} "${output_base}.c++")
    list(APPEND ${HEADERS} "${output_base}.h")
  endforeach()

  set_source_files_properties(${${SOURCES}} ${${HEADERS}} PROPERTIES GENERATED TRUE)
  set(${SOURCES} ${${SOURCES}} PARENT_SCOPE)
  set(${HEADERS} ${${HEADERS}} PARENT_SCOPE)
endfunction()
