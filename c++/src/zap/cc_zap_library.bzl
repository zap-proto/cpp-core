"""Bazel rule to compile .zap files into c++."""

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load(":zap_gen.bzl", "zap_gen", _zap_provider = "zap_provider")

# re-export for backward compatibility
zap_provider = _zap_provider

def cc_zap_library(
        name,
        srcs = [],
        data = [],
        deps = [],
        src_prefix = "",
        tags = ["off-by-default"],
        visibility = None,
        target_compatible_with = None,
        **kwargs):
    """Bazel rule to create a C++ zap library from zap source files

    Args:
        name: library name
        srcs: list of files to compile
        data: additional files to provide to the compiler - data files and includes that need not to
            be compiled
        deps: other cc_zap_library rules to depend on
        src_prefix: src_prefix for zap compiler to the source root
        visibility: rule visibility
        target_compatible_with: target compatibility
        **kwargs: rest of the arguments to cc_library rule
    """

    hdrs = [s + ".h" for s in srcs]
    srcs_cpp = [s + ".c++" for s in srcs]

    zap_gen(
        name = name + "_gen",
        srcs = srcs,
        deps = [s + "_gen" for s in deps],
        data = data,
        outs = hdrs + srcs_cpp,
        src_prefix = src_prefix,
        visibility = visibility,
        zapc_plugin = "@zap-cpp//src/zap:zapc-c++",
        target_compatible_with = target_compatible_with,
    )
    cc_library(
        name = name,
        srcs = srcs_cpp,
        hdrs = hdrs,
        deps = deps + ["@zap-cpp//src/zap:zap_runtime"],
        # Allows us to avoid building the library archive when using start_end_lib
        tags = tags,
        visibility = visibility,
        target_compatible_with = target_compatible_with,
        **kwargs
    )
