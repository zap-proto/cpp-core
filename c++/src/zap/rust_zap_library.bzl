"""Bazel rule to compile .zap files into rust."""

load("@bazel_skylib//rules:write_file.bzl", "write_file")
load("@zap-cpp//src/zap:zap_gen.bzl", "zap_gen", _zap_provider = "zap_provider")
load("@rules_rust//rust:defs.bzl", "rust_library")

zap_provider = _zap_provider

def rust_zap_library(
        name,
        srcs,
        crate_name,
        data = [],
        deps = [],
        src_prefix = "",
        tags = [],
        visibility = None,
        target_compatible_with = None,
        zap_crate = "@crates_vendor//:zap",
        zapc_plugin = "@crates_vendor//:zapc__zapc-rust",
        **kwargs):
    """Define rust zap library.

    Creates a rust_library crate with a given `crate_name`.
    """
    outs = [src.removesuffix(".zap").replace("-", "_") + "_zap.rs" for src in srcs]
    lib_rs = "lib_" + crate_name + ".rs"

    zap_gen(
        name = name + "_gen",
        srcs = srcs,
        deps = [s + "_gen" for s in deps],
        data = data,
        outs = outs,
        zapc_plugin = zapc_plugin,
        src_prefix = src_prefix,
        visibility = visibility,
        tags = tags,
        target_compatible_with = target_compatible_with,
    )

    # zapc-rust doesn't generate a standalon crate, but a file that is supposed
    # to be a part of another crate with all the dependencies in the place.
    # Generate crate root for the library and import/export all the necessary symbols.

    write_file(
        name = name + "lib_rs",
        out = lib_rs,
        content = [_lib_rs_content(crate_name, deps, outs)],
    )

    rust_library(
        name = name,
        srcs = outs + [lib_rs],
        crate_name = crate_name,
        crate_root = lib_rs,
        deps = deps + [zap_crate],
        tags = tags + ["no-clippy"],
        visibility = visibility,
        target_compatible_with = target_compatible_with,
        **kwargs
    )

def _lib_rs_content(crate_name, deps, outs):
    use_deps = [
        "#[allow(unused_imports)] use ::{}::*;".format(dep.split(":")[1].removesuffix("_rust").replace("-", "_"))
        for dep in deps
    ]

    include_outs = [
        "    include!(\"{}\");".format(out)
        for out in outs
    ]

    return """pub mod {crate_name} {{
{outs}
}}
// re-export names to be accessible directly
#[allow(unused_imports)] 
pub use {crate_name}::*;
// use dependencies
{deps}
#[allow(unused_imports)] use ::zap::*;
""".format(crate_name = crate_name, outs = "\n".join(include_outs), deps = "\n".join(use_deps))
