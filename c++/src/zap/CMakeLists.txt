
# zap ========================================================================

set(zap_sources_lite
  c++.zap.c++
  blob.c++
  arena.c++
  layout.c++
  list.c++
  any.c++
  message.c++
  schema.zap.c++
  stream.zap.c++
  serialize.c++
  serialize-packed.c++
)
set(zap_sources_heavy
  schema.c++
  schema-loader.c++
  dynamic.c++
  stringify.c++
)
if(NOT ZAP_LITE)
  set(zap_sources ${zap_sources_lite} ${zap_sources_heavy})
else()
  set(zap_sources ${zap_sources_lite})
endif()

set(zap_headers
  c++.zap.h
  common.h
  blob.h
  endian.h
  layout.h
  orphan.h
  list.h
  any.h
  message.h
  capability.h
  membrane.h
  dynamic.h
  schema.h
  schema.zap.h
  stream.zap.h
  schema-lite.h
  schema-loader.h
  schema-parser.h
  pretty-print.h
  serialize.h
  serialize-async.h
  serialize-packed.h
  serialize-text.h
  pointer-helpers.h
  generated-header-support.h
  raw-schema.h
)
set(zap_compat_headers
  compat/std-iterator.h
)
set(zap_schemas
  c++.zap
  schema.zap
  stream.zap
)
add_library(zap ${zap_sources})
add_library(Zap::zap ALIAS zap)
target_link_libraries(zap PUBLIC kj-async kj)
#make sure external consumers don't need to manually set the include dirs
get_filename_component(PARENT_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
target_include_directories(zap INTERFACE
  $<BUILD_INTERFACE:${PARENT_DIR}>
  $<INSTALL_INTERFACE:include>
)
# Ensure the library has a version set to match autotools build
set_target_properties(zap PROPERTIES VERSION ${VERSION})
install(TARGETS zap ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES ${zap_headers} ${zap_schemas} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/zap")
install(FILES ${zap_compat_headers} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/zap/compat")

set(zap-rpc_sources
  serialize-async.c++
  capability.c++
  membrane.c++
  dynamic-capability.c++
  rpc.c++
  rpc.zap.c++
  rpc-twoparty.c++
  rpc-twoparty.zap.c++
  persistent.zap.c++
)
set(zap-rpc_headers
  rpc-prelude.h
  rpc.h
  rpc-twoparty.h
  rpc.zap.h
  rpc-twoparty.zap.h
  persistent.zap.h
)
set(zap-rpc_schemas
  rpc.zap
  rpc-twoparty.zap
  persistent.zap
)
if(NOT ZAP_LITE)
  add_library(zap-rpc ${zap-rpc_sources})
  add_library(Zap::zap-rpc ALIAS zap-rpc)
  target_link_libraries(zap-rpc PUBLIC zap kj-async kj)
  # Ensure the library has a version set to match autotools build
  set_target_properties(zap-rpc PROPERTIES VERSION ${VERSION})
  install(TARGETS zap-rpc ${INSTALL_TARGETS_DEFAULT_ARGS})
  install(FILES ${zap-rpc_headers} ${zap-rpc_schemas} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/zap")
endif()

# zap-json ========================================================================

set(zap-json_sources
  compat/json.c++
  compat/json.zap.c++
)
set(zap-json_headers
  compat/json.h
  compat/json.zap.h
)
set(zap-json_schemas
  compat/json.zap
)
if(NOT ZAP_LITE)
  add_library(zap-json ${zap-json_sources})
  add_library(Zap::zap-json ALIAS zap-json)
  target_link_libraries(zap-json PUBLIC zap kj-async kj)
  # Ensure the library has a version set to match autotools build
  set_target_properties(zap-json PROPERTIES VERSION ${VERSION})
  install(TARGETS zap-json ${INSTALL_TARGETS_DEFAULT_ARGS})
  install(FILES ${zap-json_headers} ${zap-json_schemas} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/zap/compat")
endif()

# zap-websocket ========================================================================

set(zap-websocket_sources
  compat/websocket-rpc.c++
)
set(zap-websocket_headers
  compat/websocket-rpc.h
)
if(NOT ZAP_LITE)
  add_library(zap-websocket ${zap-websocket_sources})
  add_library(Zap::zap-websocket ALIAS zap-websocket)
  target_link_libraries(zap-websocket PUBLIC zap zap-rpc kj-http kj-async kj)
  # Ensure the library has a version set to match autotools build
  set_target_properties(zap-websocket PROPERTIES VERSION ${VERSION})
  install(TARGETS zap-websocket ${INSTALL_TARGETS_DEFAULT_ARGS})
  install(FILES ${zap-websocket_headers} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/zap/compat")
endif()

# Tools/Compilers ==============================================================

set(zapc_sources
  compiler/type-id.c++
  compiler/error-reporter.c++
  compiler/lexer.zap.c++
  compiler/lexer.c++
  compiler/grammar.zap.c++
  compiler/desugar.c++
  compiler/parser.c++
  compiler/generics.c++
  compiler/node-translator.c++
  compiler/compiler.c++
  schema-parser.c++
  serialize-text.c++
)
if(NOT ZAP_LITE)
  add_library(zapc ${zapc_sources})
  target_link_libraries(zapc PUBLIC zap kj)
  # Ensure the library has a version set to match autotools build
  set_target_properties(zapc PROPERTIES VERSION ${VERSION})
  install(TARGETS zapc ${INSTALL_TARGETS_DEFAULT_ARGS})
  install(FILES ${zapc_headers} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/zap")
endif()

if(NOT ZAP_LITE)
  add_executable(zap_tool
    compiler/module-loader.c++
    compiler/zap.c++
  )
  target_link_libraries(zap_tool zapc zap-json zap kj)
  set_target_properties(zap_tool PROPERTIES OUTPUT_NAME zap)
  set_target_properties(zap_tool PROPERTIES ZAP_INCLUDE_DIRECTORY
    $<JOIN:$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>,$<INSTALL_INTERFACE:${CMAKE_INSTALL_BINDIR}/..>>
  )
  target_compile_definitions(zap_tool PRIVATE
    "ZAP_INCLUDE_DIR=\"${CMAKE_INSTALL_FULL_INCLUDEDIR}\""
    "VERSION=\"${VERSION}\""
  )

  add_executable(zapc_cpp
    compiler/zapc-c++.c++
  )
  target_link_libraries(zapc_cpp zap kj)
  set_target_properties(zapc_cpp PROPERTIES OUTPUT_NAME zapc-c++)
  #Zap tool needs zapc_cpp location. But cmake deprecated LOCATION property.
  #So we use custom property to pass location
  set_target_properties(zapc_cpp PROPERTIES ZAPC_CXX_EXECUTABLE
    $<TARGET_FILE:zapc_cpp>
  )

  add_executable(zapc_zap
    compiler/zapc-zap.c++
  )
  target_link_libraries(zapc_zap zap kj)
  set_target_properties(zapc_zap PROPERTIES OUTPUT_NAME zapc-zap)

  install(TARGETS zap_tool zapc_cpp zapc_zap ${INSTALL_TARGETS_DEFAULT_ARGS})

  if(WIN32)
    # On Windows platforms symlinks are not guaranteed to support. Also different version of CMake handle create_symlink in a different way.
    # The most portable way in this case just copy the file.
    install(CODE "execute_process(COMMAND \"${CMAKE_COMMAND}\" -E copy \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_BINDIR}/zap${CMAKE_EXECUTABLE_SUFFIX}\" \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_BINDIR}/zapc${CMAKE_EXECUTABLE_SUFFIX}\")")
  else()
    # Symlink zapc -> zap
    install(CODE "execute_process(COMMAND \"${CMAKE_COMMAND}\" -E create_symlink zap${CMAKE_EXECUTABLE_SUFFIX} \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_BINDIR}/zapc${CMAKE_EXECUTABLE_SUFFIX}\")")
  endif()
endif()  # NOT ZAP_LITE

# Tests ========================================================================

if(BUILD_TESTING)
  set(test_zap_files
    test.zap
    test-import.zap
    test-import2.zap
    compat/json-test.zap
  )

  set(ZAPC_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/test_zap")
  include_directories("${ZAPC_OUTPUT_DIR}")
  file(MAKE_DIRECTORY "${ZAPC_OUTPUT_DIR}")
  # Tell zap_generate_cpp to set --src-prefix to our parent directory. This allows us to pass our
  # .zap files relative to this directory, but have their canonical name end up as
  # zap/test.zap, zap/test-import.zap, etc.
  get_filename_component(ZAPC_SRC_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)

  zap_generate_cpp(test_zap_cpp_files test_zap_h_files ${test_zap_files})

  # TODO(cleanup): zap-tests and zap-heavy-tests both depend on the test.zap output files. In
  #   a parallel Makefile-based build (maybe others?), they can race and cause the custom zap
  #   command in zap_generate_cpp() to run twice. To get around this I'm using a custom target to
  #   force CMake to generate race-free Makefiles. Remove this garbage when we move to a
  #   target-based zap_generate() command, as that will make CMake do the right thing by default.
  add_custom_target(test_zap DEPENDS ${test_zap_cpp_files} ${test_zap_h_files})

  if(ZAP_LITE)
    set(test_libraries zap kj-test kj)
  else()
    set(test_libraries zap-json zap-rpc zap-websocket zap zapc kj-http kj-async kj-test kj)
  endif()

  add_executable(zap-tests
    common-test.c++
    blob-test.c++
    endian-test.c++
    endian-fallback-test.c++
    layout-test.c++
    any-test.c++
    message-test.c++
    encoding-test.c++
    orphan-test.c++
    serialize-test.c++
    serialize-packed-test.c++
    canonicalize-test.c++
    fuzz-test.c++
    test-util.c++
    ${test_zap_cpp_files}
    ${test_zap_h_files}
  )
  target_link_libraries(zap-tests ${test_libraries})
  add_dependencies(zap-tests test_zap)
  add_dependencies(check zap-tests)
  add_test(NAME zap-tests-run COMMAND zap-tests)

  if(NOT ZAP_LITE)
    add_executable(zap-heavy-tests
      endian-reverse-test.c++
      capability-test.c++
      membrane-test.c++
      schema-test.c++
      schema-loader-test.c++
      schema-parser-test.c++
      dynamic-test.c++
      stringify-test.c++
      serialize-async-test.c++
      serialize-text-test.c++
      rpc-test.c++
      rpc-twoparty-test.c++
      compiler/lexer-test.c++
      compiler/desugar-test.c++
      compiler/type-id-test.c++
      test-util.c++
      compat/json-test.c++
      compat/websocket-rpc-test.c++
      ${test_zap_cpp_files}
      ${test_zap_h_files}
    )
    target_link_libraries(zap-heavy-tests ${test_libraries})
    if(NOT MSVC)
      set_target_properties(zap-heavy-tests
        PROPERTIES COMPILE_FLAGS "-Wno-deprecated-declarations"
      )
    endif()

    add_dependencies(zap-heavy-tests test_zap)
    add_dependencies(check zap-heavy-tests)
    add_test(NAME zap-heavy-tests-run COMMAND zap-heavy-tests)

    add_executable(zap-evolution-tests compiler/evolution-test.c++)
    target_link_libraries(zap-evolution-tests zapc zap kj)
    add_dependencies(check zap-evolution-tests)
    add_test(NAME zap-evolution-tests-run COMMAND zap-evolution-tests)
  endif()  # NOT ZAP_LITE
endif()  # BUILD_TESTING

if(DEFINED ENV{LIB_FUZZING_ENGINE})
  add_executable(zap_llvm_fuzzer_testcase llvm-fuzzer-testcase.c++ test-util.c++ test-util.h ${test_zap_cpp_files} ${test_zap_h_files})
  target_link_libraries(zap_llvm_fuzzer_testcase zap-rpc zap kj kj-async zap-json $ENV{LIB_FUZZING_ENGINE})
endif()
