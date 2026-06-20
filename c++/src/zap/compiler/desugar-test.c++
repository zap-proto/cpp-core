// Copyright (c) 2024 ZAP contributors
// Licensed under the MIT License (see desugar.c++ for full text).
//
// Unit tests for the whitespace->brace schema desugar preprocessor.

#include "desugar.h"
#include <kj/test.h>
#include <kj/string.h>

namespace zap {
namespace compiler {
namespace {

kj::String desugarStr(kj::StringPtr src) {
  auto out = desugar(src.asArray());
  return kj::heapString(out.begin(), out.size());
}

// Back-compat: brace source must be returned byte-identical (idempotent).
void assertIdempotent(kj::StringPtr brace) {
  KJ_EXPECT(desugarStr(brace) == brace, brace, desugarStr(brace));
}

KJ_TEST("desugar: brace struct unchanged") {
  assertIdempotent("struct Foo {\n  bar @0 :Int32;\n}\n");
}

KJ_TEST("desugar: brace file fixture unchanged") {
  assertIdempotent(
      "@0xf36d7b330303c66e;\n\nusing Test = import \"test.zap\";\n\n"
      "struct TestImport {\n  field @0 :Test.TestAllTypes;\n}\n");
}

KJ_TEST("desugar: nested brace struct unchanged") {
  assertIdempotent(
      "struct Foo {\n  struct Bar {\n    value @0 :Int32;\n"
      "    struct Baz {\n      value @0 :Int32;\n    }\n  }\n}\n");
}

KJ_TEST("desugar: brace interface unchanged") {
  assertIdempotent(
      "interface EmployeeManagement {\n"
      "  addEmployee @0 (employee :Employee) -> (id :Int32);\n"
      "  struct Employee {\n    name @0 :Text;\n  }\n}\n");
}

KJ_TEST("desugar: whitespace struct -> brace") {
  KJ_EXPECT(desugarStr("struct Foo\n  bar Int32\n  baz Text\n") ==
            "struct Foo {\n  bar @0 :Int32;\n  baz @1 :Text;\n}\n");
}

KJ_TEST("desugar: typed field gets ordinal only") {
  KJ_EXPECT(desugarStr("struct Foo\n  bar :Int32\n  baz :Text\n") ==
            "struct Foo {\n  bar @0 :Int32;\n  baz @1 :Text;\n}\n");
}

KJ_TEST("desugar: explicit ordinal preserved and continues") {
  KJ_EXPECT(desugarStr("struct Foo\n  bar @5 :Int32\n  baz Text\n") ==
            "struct Foo {\n  bar @5 :Int32;\n  baz @6 :Text;\n}\n");
}

KJ_TEST("desugar: nested whitespace struct") {
  KJ_EXPECT(desugarStr("struct Outer\n  a Int32\n  struct Inner\n    b Text\n  c Bool\n") ==
            "struct Outer {\n  a @0 :Int32;\n  struct Inner {\n    b @0 :Text;\n  }\n  c @1 :Bool;\n}\n");
}

KJ_TEST("desugar: enum") {
  KJ_EXPECT(desugarStr("enum Color\n  red\n  green\n  blue\n") ==
            "enum Color {\n  red @0;\n  green @1;\n  blue @2;\n}\n");
}

KJ_TEST("desugar: anonymous union continues struct ordinals") {
  KJ_EXPECT(desugarStr("struct Foo\n  a Int32\n  union\n    b Text\n    c Bool\n  d Int8\n") ==
            "struct Foo {\n  a @0 :Int32;\n  union {\n    b @1 :Text;\n    c @2 :Bool;\n  }\n  d @3 :Int8;\n}\n");
}

KJ_TEST("desugar: named union shares struct ordinals") {
  KJ_EXPECT(desugarStr("struct Foo\n  a Int32\n  payload :union\n    x Text\n    y Bool\n") ==
            "struct Foo {\n  a @0 :Int32;\n  payload :union {\n    x @1 :Text;\n    y @2 :Bool;\n  }\n}\n");
}

KJ_TEST("desugar: interface methods auto-assign ordinals") {
  KJ_EXPECT(desugarStr("interface Calc\n  add (a :Int32, b :Int32) -> (sum :Int32)\n  neg (x :Int32) -> (y :Int32)\n") ==
            "interface Calc {\n  add @0 (a :Int32, b :Int32) -> (sum :Int32);\n  neg @1 (x :Int32) -> (y :Int32);\n}\n");
}

KJ_TEST("desugar: blank lines and comments are transparent") {
  KJ_EXPECT(desugarStr("struct Foo\n  a Int32\n\n  # a comment\n  b Text\n") ==
            "struct Foo {\n  a @0 :Int32;\n\n  # a comment\n  b @1 :Text;\n}\n");
}

KJ_TEST("desugar: mixed brace and whitespace at top level") {
  KJ_EXPECT(desugarStr("struct A {\n  x @0 :Int32;\n}\n\nstruct B\n  y Text\n") ==
            "struct A {\n  x @0 :Int32;\n}\n\nstruct B {\n  y @0 :Text;\n}\n");
}

KJ_TEST("desugar: top-level decls untouched") {
  KJ_EXPECT(desugarStr("@0xabcd1234abcd1234;\nusing Foo = import \"x.zap\";\nconst k :UInt32 = 5;\nstruct S\n  a Int32\n") ==
            "@0xabcd1234abcd1234;\nusing Foo = import \"x.zap\";\nconst k :UInt32 = 5;\nstruct S {\n  a @0 :Int32;\n}\n");
}

KJ_TEST("desugar: CRLF line endings preserved") {
  KJ_EXPECT(desugarStr("struct Foo\r\n  a Int32\r\n") ==
            "struct Foo {\r\n  a @0 :Int32;\r\n}\n");
}

KJ_TEST("desugar: explicit semicolon not doubled") {
  KJ_EXPECT(desugarStr("struct Foo\n  a Int32;\n  b Text;\n") ==
            "struct Foo {\n  a @0 :Int32;\n  b @1 :Text;\n}\n");
}

KJ_TEST("desugar: blank line between top decls lands after close") {
  KJ_EXPECT(desugarStr("struct A\n  x Int32\n\nstruct B\n  y Text\n") ==
            "struct A {\n  x @0 :Int32;\n}\n\nstruct B {\n  y @0 :Text;\n}\n");
}

KJ_TEST("desugar: brace block then whitespace enum") {
  KJ_EXPECT(desugarStr("struct Outer {\n  inner @0 :Int32;\n}\nenum E\n  a\n  b\n") ==
            "struct Outer {\n  inner @0 :Int32;\n}\nenum E {\n  a @0;\n  b @1;\n}\n");
}

KJ_TEST("desugar: inline single-line brace struct untouched") {
  // `struct Foo { ... }` / `struct Bar {}` on one line are brace syntax (regression:
  // these forms appear in test.zap and rpc-twoparty.zap) and must pass through verbatim.
  assertIdempotent("struct BoxedText { text @0 :Text; }\n");
  assertIdempotent("struct ThirdPartyToAwait {}\n");
  assertIdempotent("struct Empty {}\nstruct Filled { x @0 :Int32; }\n");
}

KJ_TEST("desugar: inline brace then whitespace mix") {
  KJ_EXPECT(desugarStr("struct Inline { a @0 :Int32; }\nstruct WS\n  b Text\n") ==
            "struct Inline { a @0 :Int32; }\nstruct WS {\n  b @0 :Text;\n}\n");
}

KJ_TEST("desugar: isWhitespaceSchema detects ws vs brace") {
  KJ_EXPECT(isWhitespaceSchema(kj::StringPtr("struct Foo\n  a Int32\n").asArray()));
  KJ_EXPECT(!isWhitespaceSchema(kj::StringPtr("struct Foo {\n  a @0 :Int32;\n}\n").asArray()));
  KJ_EXPECT(!isWhitespaceSchema(kj::StringPtr("@0xabcd;\nusing X = import \"y.zap\";\n").asArray()));
  // inline single-line brace structs must NOT be misdetected as whitespace syntax.
  KJ_EXPECT(!isWhitespaceSchema(kj::StringPtr("struct BoxedText { text @0 :Text; }\n").asArray()));
  KJ_EXPECT(!isWhitespaceSchema(kj::StringPtr("struct ThirdPartyToAwait {}\n").asArray()));
}

}  // namespace
}  // namespace compiler
}  // namespace zap
