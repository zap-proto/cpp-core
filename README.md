# ZAP — C++ schema front-end

> **Docs:** [Schema language](https://zap-proto.dev/docs/schema) · [Code generation](https://zap-proto.dev/docs/codegen) — part of the [ZAP Protocol](https://github.com/zap-proto)

This repository is the canonical **schema front-end** for **ZAP** (Zero-copy
Application Protocol): the C++ implementation of the `.zap` schema parser and the
`zap` compiler binary that every language plugin (Rust, OCaml, Python, Haskell,
…) executes to read a schema. It also provides the underlying C++ runtime and KJ
support library.

It is derived from `libcapnp` and keeps that codebase's wire model and code-quality
bar, while ZAP adds the whitespace-significant schema grammar on top.

## What it does

The `zap` binary parses a schema and emits a binary *code-generator request* on
stdout. Language plugins consume that request and emit code for their target
language — they never parse `.zap` text themselves. That is the contract that
keeps schema semantics identical across every binding: there is exactly one
parser, and it lives here.

ZAP schemas are whitespace-significant. Blocks are delimited by indentation (the
offside rule), there are no braces, and field ordinals are assigned automatically
in declaration order. A `point.zap` schema:

```zap
struct Point
  x Float32
  y Float32

interface PointTracker
  addPoint (p Point) -> (totalPoints UInt64)
```

The front-end's `compiler/desugar.{h,c++}` lowers this indentation form to the
core grammar before the rest of the compiler runs. The legacy brace form
(`x @0 :Float32;`) still parses for backward compatibility — desugaring is a
no-op on it — so both surfaces produce byte-identical schemas.

## Compiling schemas

```sh
# Emit a code-generator request to stdout (what a plugin reads on stdin).
zap compile -o- point.zap

# Drive a language plugin found on PATH (e.g. the Rust backend).
zap compile -orust point.zap
```

Plugins are resolved as `zap-<lang>` executables on `PATH`. The Rust, OCaml,
Python, and Haskell runtimes all shell out to this same `zap` binary; provisioning
it is how their CI builds run schema codegen.

## Building

```sh
# Autotools
cd c++
autoreconf -i && ./configure && make -j check

# CMake
cmake -S c++ -B build && cmake --build build

# Bazel
bazel build //c++/...
```

`make check` builds the compiler (the `zap_tool` target) and runs the schema,
compiler, and runtime test suites.

## Consumed by

| Plugin / runtime | Repository |
| --- | --- |
| Rust | [`zap-proto/rust`](https://github.com/zap-proto/rust) |
| OCaml | [`zap-proto/ocaml`](https://github.com/zap-proto/ocaml) |
| Python | [`zap-proto/py`](https://github.com/zap-proto/py) |
| Haskell | [`zap-proto/haskell`](https://github.com/zap-proto/haskell) |

The high-level C++ *binding* (client/server convenience API) is a separate
project, [`zap-proto/cpp`](https://github.com/zap-proto/cpp); this repository is
the schema toolchain and core runtime those bindings build on.

## License

MIT — see [LICENSE](LICENSE).
