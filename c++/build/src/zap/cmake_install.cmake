# Install script for directory: /home/z/zap-cpp-core/c++/src/zap

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/llvm-objdump-21")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/z/zap-cpp-core/c++/build/src/zap/libzap.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/zap" TYPE FILE FILES
    "/home/z/zap-cpp-core/c++/src/zap/c++.zap.h"
    "/home/z/zap-cpp-core/c++/src/zap/common.h"
    "/home/z/zap-cpp-core/c++/src/zap/blob.h"
    "/home/z/zap-cpp-core/c++/src/zap/endian.h"
    "/home/z/zap-cpp-core/c++/src/zap/layout.h"
    "/home/z/zap-cpp-core/c++/src/zap/orphan.h"
    "/home/z/zap-cpp-core/c++/src/zap/list.h"
    "/home/z/zap-cpp-core/c++/src/zap/any.h"
    "/home/z/zap-cpp-core/c++/src/zap/message.h"
    "/home/z/zap-cpp-core/c++/src/zap/capability.h"
    "/home/z/zap-cpp-core/c++/src/zap/membrane.h"
    "/home/z/zap-cpp-core/c++/src/zap/dynamic.h"
    "/home/z/zap-cpp-core/c++/src/zap/schema.h"
    "/home/z/zap-cpp-core/c++/src/zap/schema.zap.h"
    "/home/z/zap-cpp-core/c++/src/zap/stream.zap.h"
    "/home/z/zap-cpp-core/c++/src/zap/schema-lite.h"
    "/home/z/zap-cpp-core/c++/src/zap/schema-loader.h"
    "/home/z/zap-cpp-core/c++/src/zap/schema-parser.h"
    "/home/z/zap-cpp-core/c++/src/zap/pretty-print.h"
    "/home/z/zap-cpp-core/c++/src/zap/serialize.h"
    "/home/z/zap-cpp-core/c++/src/zap/serialize-async.h"
    "/home/z/zap-cpp-core/c++/src/zap/serialize-packed.h"
    "/home/z/zap-cpp-core/c++/src/zap/serialize-text.h"
    "/home/z/zap-cpp-core/c++/src/zap/pointer-helpers.h"
    "/home/z/zap-cpp-core/c++/src/zap/generated-header-support.h"
    "/home/z/zap-cpp-core/c++/src/zap/raw-schema.h"
    "/home/z/zap-cpp-core/c++/src/zap/c++.zap"
    "/home/z/zap-cpp-core/c++/src/zap/schema.zap"
    "/home/z/zap-cpp-core/c++/src/zap/stream.zap"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/zap/compat" TYPE FILE FILES "/home/z/zap-cpp-core/c++/src/zap/compat/std-iterator.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/z/zap-cpp-core/c++/build/src/zap/libzap-rpc.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/zap" TYPE FILE FILES
    "/home/z/zap-cpp-core/c++/src/zap/rpc-prelude.h"
    "/home/z/zap-cpp-core/c++/src/zap/rpc.h"
    "/home/z/zap-cpp-core/c++/src/zap/rpc-twoparty.h"
    "/home/z/zap-cpp-core/c++/src/zap/rpc.zap.h"
    "/home/z/zap-cpp-core/c++/src/zap/rpc-twoparty.zap.h"
    "/home/z/zap-cpp-core/c++/src/zap/persistent.zap.h"
    "/home/z/zap-cpp-core/c++/src/zap/rpc.zap"
    "/home/z/zap-cpp-core/c++/src/zap/rpc-twoparty.zap"
    "/home/z/zap-cpp-core/c++/src/zap/persistent.zap"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/z/zap-cpp-core/c++/build/src/zap/libzap-json.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/zap/compat" TYPE FILE FILES
    "/home/z/zap-cpp-core/c++/src/zap/compat/json.h"
    "/home/z/zap-cpp-core/c++/src/zap/compat/json.zap.h"
    "/home/z/zap-cpp-core/c++/src/zap/compat/json.zap"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/z/zap-cpp-core/c++/build/src/zap/libzap-websocket.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/zap/compat" TYPE FILE FILES "/home/z/zap-cpp-core/c++/src/zap/compat/websocket-rpc.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/z/zap-cpp-core/c++/build/src/zap/libzapc.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/zap" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/zap")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/zap"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/home/z/zap-cpp-core/c++/build/src/zap/zap")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/zap" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/zap")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/llvm-strip-21" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/zap")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/zapc-c++" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/zapc-c++")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/zapc-c++"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/home/z/zap-cpp-core/c++/build/src/zap/zapc-c++")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/zapc-c++" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/zapc-c++")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/llvm-strip-21" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/zapc-c++")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/zapc-zap" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/zapc-zap")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/zapc-zap"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/home/z/zap-cpp-core/c++/build/src/zap/zapc-zap")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/zapc-zap" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/zapc-zap")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/llvm-strip-21" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/zapc-zap")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  execute_process(COMMAND "/usr/bin/cmake" -E create_symlink zap "$ENV{DESTDIR}/usr/local/bin/zapc")
endif()

