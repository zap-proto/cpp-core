## Process this file with autoconf to produce configure.

AC_INIT([Zap],[2.0-dev],[zap@googlegroups.com],[zap-c++])

AC_CONFIG_SRCDIR([src/zap/layout.c++])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# autoconf's default CXXFLAGS are usually "-g -O2".  A far more reasonable
# default is -O2 -NDEBUG.
AS_IF([test "x${ac_cv_env_CFLAGS_set}" = "x"],
      [CFLAGS="-O2 -DNDEBUG"])
AS_IF([test "x${ac_cv_env_CXXFLAGS_set}" = "x"],
      [CXXFLAGS="-O2 -DNDEBUG"])

AM_INIT_AUTOMAKE([tar-ustar])

AC_ARG_WITH([external-zap],
  [AS_HELP_STRING([--with-external-zap],
    [use the system zap binary (or the one specified with $ZAP) instead of compiling a new
     one (useful for cross-compiling)])],
  [external_zap=yes],[external_zap=no])

AC_ARG_WITH([zlib],
  [AS_HELP_STRING([--with-zlib],
    [build libkj-gzip by linking against zlib @<:@default=check@:>@])],
  [],[with_zlib=check])

AC_ARG_WITH([openssl],
  [AS_HELP_STRING([--with-openssl],
    [build libkj-tls by linking against openssl @<:@default=check@:>@])],
  [],[with_openssl=check])

AC_ARG_WITH([fibers],
  [AS_HELP_STRING([--with-fibers],
    [build libkj-async with fibers @<:@default=check@:>@])],
  [],[with_fibers=check])

AC_ARG_ENABLE([reflection], [
  AS_HELP_STRING([--disable-reflection], [
    compile Zap in "lite mode", in which all reflection APIs (schema.h, dynamic.h, etc.)
    are not included. Produces a smaller library at the cost of features. All programs built
    against the library MUST be compiled with -DZAP_LITE=1. Note that because the compiler
    itself uses reflection in its implementation, you must also use --with-external-zap when
    using this option.])
  ], [
    case "${enableval}" in
      yes)
        lite_mode=no
        ;;
      no)
        lite_mode=yes
        AS_IF([test "$external_zap" != "yes"], [
          AC_MSG_ERROR([you must specify --with-external-zap when using --disable-reflection])
        ])
        ;;
      *)
        AC_MSG_ERROR([bad value ${enableval} for --enable-reflection])
        ;;
    esac
  ], [lite_mode=no])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_LANG([C++])
AX_CXX_COMPILE_STDCXX_23

AS_CASE("${host_os}", *mingw*, [
    # We don't use pthreads on MinGW.
    PTHREAD_CFLAGS="-mthreads"
    PTHREAD_LIBS=""
    PTHREAD_CC=""
    ASYNC_LIBS="-lws2_32"
    AC_SUBST(PTHREAD_LIBS)
    AC_SUBST(PTHREAD_CFLAGS)
    AC_SUBST(PTHREAD_CC)
    AC_SUBST(ASYNC_LIBS)
  ], *, [
    ACX_PTHREAD
    ASYNC_LIBS=""
    AC_SUBST(ASYNC_LIBS)
  ])

LT_INIT

AS_IF([test "$external_zap" != "no"], [
	AS_IF([test "x$ZAP" = "x"], [ZAP="zap"], [with_zap=yes])
	AS_IF([test "x$ZAPC_CXX" = "x"], [
	  # ZAPC_CXX was not specified.  Choose a reasonable default.
	  AS_CASE([$ZAP], [*/*], [
	    # $ZAP contains a slash, so it's not on $PATH.  Assume zapc-c++ is not either, but is
	    # in the same directory.
	    ZAPC_CXX=`dirname $ZAP`/zapc-c++
	  ], [
	    # $ZAP is on $PATH, so tell it to find the plugin on $PATH as well.
	    ZAPC_CXX="c++"
	  ])
	])
	AC_SUBST([ZAP])
	AC_SUBST([ZAPC_CXX])
])
AM_CONDITIONAL([USE_EXTERNAL_ZAP], [test "$external_zap" != "no"])

AM_CONDITIONAL([LITE_MODE], [test "$lite_mode" = "yes"])

AS_IF([test "$lite_mode" = "yes"], [
  CXXFLAGS="-DZAP_LITE $CXXFLAGS"
  ZAP_LITE_FLAG=-DZAP_LITE
])
AC_SUBST([ZAP_LITE_FLAG])

AC_SEARCH_LIBS(sched_yield, rt)

# Check if we need to link against libatomic for atomic operations.
# On some architectures (e.g., armel, mipsel, powerpc), GCC requires linking
# with libatomic to support certain atomic operations.
AC_MSG_CHECKING([if libatomic is needed])
AC_LINK_IFELSE([
  AC_LANG_PROGRAM(
    [[#include <atomic>
      #include <cstdint>]],
    [[std::atomic<std::int64_t> a{0};
      a.fetch_add(1, std::memory_order_relaxed);
      return 0;]])
], [
  AC_MSG_RESULT([no])
], [
  # Linking failed, try with -latomic
  SAVED_LIBS="$LIBS"
  LIBS="$LIBS -latomic"
  AC_LINK_IFELSE([
    AC_LANG_PROGRAM(
      [[#include <atomic>
        #include <cstdint>]],
      [[std::atomic<std::int64_t> a{0};
        a.fetch_add(1, std::memory_order_relaxed);
        return 0;]])
  ], [
    AC_MSG_RESULT([yes])
  ], [
    AC_MSG_RESULT([no, but linking still fails])
    LIBS="$SAVED_LIBS"
    AC_MSG_ERROR([Cannot link program using atomic operations. Please ensure your compiler and system support C++ atomics.])
  ])
])

# Users will need to use the same -stdlib as us so we'd better let pkg-config know about it.
STDLIB_FLAG=`echo "$CXX $CXXFLAGS" | grep -o ' [[-]]stdlib=[[^ ]]*'`
AC_SUBST([STDLIB_FLAG])

LIBS="$PTHREAD_LIBS $LIBS"
CXXFLAGS="$CXXFLAGS $PTHREAD_CFLAGS"

AC_DEFUN([ZAP_PKG_CONFIG_FILES], [ \
  pkgconfig/zap.pc \
  pkgconfig/zapc.pc \
  pkgconfig/zap-rpc.pc \
  pkgconfig/zap-json.pc \
  pkgconfig/zap-websocket.pc \
  pkgconfig/kj.pc \
  pkgconfig/kj-async.pc \
  pkgconfig/kj-http.pc \
  pkgconfig/kj-gzip.pc \
  pkgconfig/kj-tls.pc \
  pkgconfig/kj-test.pc \
])
AC_DEFUN([ZAP_CMAKE_CONFIG_FILES], [ \
  cmake/ZapConfig.cmake \
  cmake/ZapConfigVersion.cmake \
])

[ZAP_PKG_CONFIG_FILES]="ZAP_PKG_CONFIG_FILES"
[ZAP_CMAKE_CONFIG_FILES]="ZAP_CMAKE_CONFIG_FILES"
AC_SUBST([ZAP_PKG_CONFIG_FILES])
AC_SUBST([ZAP_CMAKE_CONFIG_FILES])

# Don't include security release in soname -- we want to replace old binaries
# in this case.
SO_VERSION=$(echo $VERSION | sed -e 's/^\([0-9]*[.][0-9]*[.][0-9]*\)\([.][0-9]*\)*\(-.*\)*$/\1\3/g')
AC_SUBST([SO_VERSION])

# ZapConfig.cmake.in needs these PACKAGE_* output variables.
PACKAGE_INIT="set([ZAP_PKG_CONFIG_FILES] ZAP_PKG_CONFIG_FILES)"
PACKAGE_CMAKE_INSTALL_FULL_INCLUDEDIR="\${CMAKE_CURRENT_LIST_DIR}/../../../include"
AC_SUBST([PACKAGE_INIT])
AC_SUBST([PACKAGE_CMAKE_INSTALL_FULL_INCLUDEDIR])

# ZapConfigVersion.cmake.in needs PACKAGE_VERSION (already defined by AC_INIT) and
# CMAKE_SIZEOF_VOID_P output variables.
AC_CHECK_SIZEOF([void *])
AC_SUBST(CMAKE_SIZEOF_VOID_P, $ac_cv_sizeof_void_p)

# Detect presence of zlib, if it was not specified explicitly.
AS_IF([test "$with_zlib" = check], [
  AC_CHECK_LIB(z, deflate, [:], [
    with_zlib=no
  ])
  AC_CHECK_HEADER([zlib.h], [:], [
    with_zlib=no
  ])
  AS_IF([test "$with_zlib" = no], [
    AC_MSG_WARN("could not find zlib -- won't build libkj-gzip")
  ], [
    with_zlib=yes
  ])
])
AS_IF([test "$with_zlib" != no], [
  CXXFLAGS="$CXXFLAGS -DKJ_HAS_ZLIB"
])
AM_CONDITIONAL([BUILD_KJ_GZIP], [test "$with_zlib" != no])

# Detect presence of OpenSSL, if it was not specified explicitly.
AS_IF([test "$with_openssl" = check], [
  AC_CHECK_LIB(crypto, CRYPTO_new_ex_data, [:], [
    with_openssl=no
  ])
  AC_CHECK_LIB(ssl, OPENSSL_init_ssl, [:], [
    with_openssl=no
  ], [-lcrypto])
  AC_CHECK_HEADER([openssl/ssl.h], [:], [
    with_openssl=no
  ])
  AS_IF([test "$with_openssl" = no], [
    AC_MSG_WARN("could not find OpenSSL -- won't build libkj-tls")
  ], [
    with_openssl=yes
  ])
])
AS_IF([test "$with_openssl" != no], [
  CXXFLAGS="$CXXFLAGS -DKJ_HAS_OPENSSL"
])
AM_CONDITIONAL([BUILD_KJ_TLS], [test "$with_openssl" != no])

# Fibers don't work if exceptions are disabled, so default off in that case.
AS_IF([test "$with_fibers" != no], [
  AC_MSG_CHECKING([if exceptions are enabled])
  AC_COMPILE_IFELSE([void foo() { throw 1; }], [
    AC_MSG_RESULT([yes])
  ], [
    AS_IF([test "$with_fibers" = check], [
      AC_MSG_RESULT([no -- therefore, disabling fibers])
      with_fibers=no
    ], [
      AC_MSG_RESULT([no])
      AC_MSG_ERROR([Fibers require exceptions, but your compiler flags disable exceptions. Please either enable exceptions or disable fibers (--without-fibers).])
    ])
  ])
])

# Check for library support necessary for fibers.
AS_IF([test "$with_fibers" != no], [
  case "${host_os}" in
    cygwin* | mingw* )
      # Fibers always work on Windows, where there's an explicit API for them.
      with_fibers=yes
      ;;
    * )
      # Fibers need the symbols getcontext, setcontext, swapcontext and makecontext.
      # We assume that makecontext implies the rest.
      libc_supports_fibers=yes
      AC_SEARCH_LIBS([makecontext], [], [], [
        libc_supports_fibers=no
      ])

      AS_IF([test "$libc_supports_fibers" = yes], [
          with_fibers=yes
        ], [
        # If getcontext does not exist in libc, try with libucontext
        ucontext_supports_fibers=yes
        AC_CHECK_LIB(ucontext, [makecontext], [], [
          ucontext_supports_fibers=no
        ])
        AS_IF([test "$ucontext_supports_fibers" = yes], [
          ASYNC_LIBS="$ASYNC_LIBS -lucontext"
          with_fibers=yes
        ], [
          AS_IF([test "$with_fibers" = yes], [
            AC_MSG_ERROR([Missing symbols required for fibers (makecontext, setcontext, ...). Disable fibers (--without-fibers) or install libucontext])
          ], [
            AC_MSG_WARN([could not find required symbols (makecontext, setcontext, ...) -- won't build with fibers])
            with_fibers=no
          ])
        ])
      ])
      ;;
  esac
])
AS_IF([test "$with_fibers" = yes], [
  CXXFLAGS="$CXXFLAGS -DKJ_USE_FIBERS"
], [
  CXXFLAGS="$CXXFLAGS -DKJ_USE_FIBERS=0"
])

# ZapConfig.cmake.in needs these variables,
# we force them to NO because we don't need the CMake dependency for them,
# the dependencies are provided by the .pc files.
AC_SUBST(WITH_OPENSSL, NO)
AC_SUBST(_WITH_LIBUCONTEXT, NO)

AM_CONDITIONAL([HAS_FUZZING_ENGINE], [test "x$LIB_FUZZING_ENGINE" != "x"])

AC_CONFIG_FILES([Makefile] ZAP_PKG_CONFIG_FILES ZAP_CMAKE_CONFIG_FILES)
AC_OUTPUT
