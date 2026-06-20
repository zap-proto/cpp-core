
# Tests ========================================================================

if(BUILD_TESTING)
  include(CTest)

  if(EXTERNAL_ZAP)
    # Set up ZAP_GENERATE_CPP for compiling test schemas
    find_package(Zap CONFIG QUIET)
    if(NOT Zap_FOUND)
      # No working installation of Zap found, so fall back to searching the environment.
      #
      # We search for the external zap compiler binaries via $ZAP, $ZAPC_CXX, and
      # find_program(). find_program() will use various paths in its search, among them
      # ${CMAKE_PREFIX_PATH}/bin and $PATH.

      if(NOT ZAP_EXECUTABLE)
        if(DEFINED ENV{ZAP})
          set(ZAP_EXECUTABLE "$ENV{ZAP}")
        else()
          find_program(ZAP_EXECUTABLE zap)
        endif()
      endif()

      if(NOT ZAPC_CXX_EXECUTABLE)
        if(DEFINED ENV{ZAPC_CXX})
          set(ZAPC_CXX_EXECUTABLE "$ENV{ZAPC_CXX}")
        else()
          # Also search in the same directory that `zap` was found in
          get_filename_component(zap_dir "${ZAP_EXECUTABLE}" DIRECTORY)
          find_program(ZAPC_CXX_EXECUTABLE zapc-c++ HINTS "${zap_dir}")
        endif()
      endif()
    endif()

    set(ZAP_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    #TODO(someday) It would be nice to use targets instead of variables in ZAP_GENERATE_CPP macro
  endif()

  # Sadly, we can't use the 'test' target, as that's coopted by ctest
  add_custom_target(check "${CMAKE_CTEST_COMMAND}" -V)
endif()  # BUILD_TESTING

# kj ===========================================================================

add_subdirectory(kj)

# zap ========================================================================

add_subdirectory(zap)
