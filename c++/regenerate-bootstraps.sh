#! /usr/bin/env bash
# Run this script every time compiler generated code changes to update checked-in generated code.

set -euo pipefail

export PATH=$PWD/bin:$PWD:$PATH

bazel build src/zap/zap_tool src/zap/zapc-c++

bazel-bin/src/zap/zap_tool compile -Isrc --no-standard-import --src-prefix=src \
    -obazel-bin/src/zap/zapc-c++:src \
    src/zap/c++.zap src/zap/schema.zap src/zap/stream.zap \
    src/zap/compiler/lexer.zap src/zap/compiler/grammar.zap \
    src/zap/rpc.zap src/zap/rpc-twoparty.zap src/zap/persistent.zap \
    src/zap/compat/json.zap
