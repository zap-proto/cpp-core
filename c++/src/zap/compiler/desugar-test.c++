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

// ===== red-team edge-case regressions =====================================

// [audit: C2] A trailing '#' comment on a whitespace MEMBER must not swallow the ';'
// terminator. Previously `a Int8  # doc` produced `a @0 :Int8  # doc;` (the ';' landed
// AFTER the comment, leaving the field unterminated — capnp requires ';'). The ';' must
// be inserted BEFORE the comment, and the original gap before '#' is preserved.
KJ_TEST("desugar: trailing comment on field keeps ';' before comment") {
  KJ_EXPECT(desugarStr("struct Foo\n  a Int8  # doc\n") ==
            "struct Foo {\n  a @0 :Int8;  # doc\n}\n");
  // enum value and interface method with trailing comments likewise stay terminated.
  KJ_EXPECT(desugarStr("enum Color\n  red # primary\n  green\n") ==
            "enum Color {\n  red @0; # primary\n  green @1;\n}\n");
  KJ_EXPECT(desugarStr("interface Calc\n  add (a :Int32) -> (s :Int32) # sum\n") ==
            "interface Calc {\n  add @0 (a :Int32) -> (s :Int32); # sum\n}\n");
  // an explicit ';' already present is still not doubled when a comment follows.
  KJ_EXPECT(desugarStr("struct Foo\n  a Int8;  # doc\n") ==
            "struct Foo {\n  a @0 :Int8;  # doc\n}\n");
}

// [audit: C1] A trailing '#' comment on a whitespace HEADER must not inject '{' INSIDE
// the comment. Previously `struct S # hi` produced `struct S # hi {` (brace commented
// out → unbalanced braces). The '{' must be emitted BEFORE the comment: `struct S { # hi`.
KJ_TEST("desugar: trailing comment on header keeps '{' before comment") {
  KJ_EXPECT(desugarStr("struct S # hi\n  a Int8\n") ==
            "struct S { # hi\n  a @0 :Int8;\n}\n");
  KJ_EXPECT(desugarStr("enum E # colors\n  a\n") ==
            "enum E { # colors\n  a @0;\n}\n");
  // named union/group headers with comments open a real block (see also C4 below).
  // `p` is the first member here, so the shared-ordinal union starts at @0.
  KJ_EXPECT(desugarStr("struct Foo\n  p :union # tag\n    x Text\n") ==
            "struct Foo {\n  p :union { # tag\n    x @0 :Text;\n  }\n}\n");
}

// [audit: C3] A tab-indented body must nest under its header. Previously leadingSpaces()
// counted only ' ', so a tab body had indent 0 == the header's indent and the dedent loop
// closed the struct empty, orphaning every field at file scope. Tabs now count as indent
// (consistent with the Go and TypeScript desugarers); literal tabs are preserved.
KJ_TEST("desugar: tab-indented body nests instead of orphaning fields") {
  KJ_EXPECT(desugarStr("struct Foo\n\ta Int8\n\tb Text\n") ==
            "struct Foo {\n\ta @0 :Int8;\n\tb @1 :Text;\n}\n");
  // nested tab body (deeper tab = deeper block). Close braces are emitted with space
  // indentation (one space per indent level) regardless of the tab-indented body, exactly
  // as the Go/TS desugarers do; the parser accepts the close brace at any column.
  KJ_EXPECT(desugarStr("struct Outer\n\ta Int8\n\tstruct Inner\n\t\tb Text\n") ==
            "struct Outer {\n\ta @0 :Int8;\n\tstruct Inner {\n\t\tb @0 :Text;\n }\n}\n");
}

// [audit: C4] A named `name :union` / `name :group` header carrying a trailing comment
// must still be recognized as a union/group. Previously classifyHeader compared the
// suffix against "union"/"group" WITHOUT stripping the comment, returned NONE, and the
// union collapsed into sibling fields with wrong ordinals. The comment is now stripped
// before the suffix comparison; members share the enclosing struct's ordinal sequence.
KJ_TEST("desugar: named union/group with trailing comment is recognized") {
  KJ_EXPECT(desugarStr("struct Foo\n  a Int32\n  payload :union # tag\n    x Text\n    y Bool\n") ==
            "struct Foo {\n  a @0 :Int32;\n  payload :union { # tag\n"
            "    x @1 :Text;\n    y @2 :Bool;\n  }\n}\n");
  KJ_EXPECT(desugarStr("struct Foo\n  a Int32\n  payload :group # tag\n    x Text\n    y Bool\n") ==
            "struct Foo {\n  a @0 :Int32;\n  payload :group { # tag\n"
            "    x @1 :Text;\n    y @2 :Bool;\n  }\n}\n");
}

// [audit: H1] Explicit ordinals are 16-bit (parser rejects >= 65536). The desugar must
// parse without uint32 wrap and reject out-of-range ordinals rather than silently
// emitting/wrapping them. Previously `@4294967295` set the next member to `@0` (wrapped),
// and `@70000` was accepted though invalid.
KJ_TEST("desugar: out-of-range ordinal is rejected, in-range preserved") {
  KJ_EXPECT_THROW_RECOVERABLE_MESSAGE("ordinal out of range",
      desugar(kj::StringPtr("struct Foo\n  a @4294967295 :Int8\n  b Text\n").asArray()));
  KJ_EXPECT_THROW_RECOVERABLE_MESSAGE("ordinal out of range",
      desugar(kj::StringPtr("struct Foo\n  a @70000 :Int8\n  b Text\n").asArray()));
  // astronomically long literal must not wrap a uint32 back into range.
  KJ_EXPECT_THROW_RECOVERABLE_MESSAGE("ordinal out of range",
      desugar(kj::StringPtr("struct Foo\n  a @99999999999999999999 :Int8\n").asArray()));
  // the maximum valid ordinal (65535) is accepted; the successor auto-ordinal would be
  // out of range, so a following auto field is rejected — but an explicit one is fine.
  KJ_EXPECT(desugarStr("struct Foo\n  a @65535 :Int8\n") ==
            "struct Foo {\n  a @65535 :Int8;\n}\n");
  KJ_EXPECT_THROW_RECOVERABLE_MESSAGE("ordinal out of range",
      desugar(kj::StringPtr("struct Foo\n  a @65535 :Int8\n  b Text\n").asArray()));
}

// [audit: H4] A field NAMED after a structural keyword must not be misdetected as a block
// header. `union`/`group` take no name, so `union Int8`/`group Int8` are unambiguously
// fields. `struct`/`interface`/`enum` take a name, so the field form is disambiguated by a
// ':type' or trailing '@N'. A genuine `keyword Identifier` (nothing after) remains a header.
KJ_TEST("desugar: field named after a keyword is not a header") {
  // union/group as field names.
  KJ_EXPECT(desugarStr("struct Foo\n  union Int8\n  b Text\n") ==
            "struct Foo {\n  union @0 :Int8;\n  b @1 :Text;\n}\n");
  KJ_EXPECT(desugarStr("struct Foo\n  group Int8\n  b Text\n") ==
            "struct Foo {\n  group @0 :Int8;\n  b @1 :Text;\n}\n");
  // struct/interface/enum as field names (disambiguated by ':type' or '@N').
  KJ_EXPECT(desugarStr("struct Foo\n  struct :Int8\n  b Text\n") ==
            "struct Foo {\n  struct @0 :Int8;\n  b @1 :Text;\n}\n");
  KJ_EXPECT(desugarStr("struct Foo\n  enum UInt8 @0\n  b Text\n") ==
            "struct Foo {\n  enum UInt8 @0;\n  b @1 :Text;\n}\n");
  // genuine headers (keyword + one identifier + EOL) still open a block.
  KJ_EXPECT(desugarStr("struct Int8\n  a Text\n") ==
            "struct Int8 {\n  a @0 :Text;\n}\n");
  KJ_EXPECT(!isWhitespaceSchema(kj::StringPtr("struct Foo {\n  union @0 :Int8;\n}\n").asArray()));
}

// Back-compat guard: brace constructs that appear in the real corpus — trailing comments
// on brace lines, a field NAMED after a keyword (`struct :group`), and tab-indented brace
// bodies — must still round-trip byte-identical after the edge-case fixes above.
KJ_TEST("desugar: brace corpus constructs remain byte-identical") {
  assertIdempotent("struct Foo {  # a documented struct\n  a @0 :Int8;  # a field\n}\n");
  assertIdempotent("struct Node {\n  struct :group {\n    x @0 :Int32;\n  }\n}\n");
  assertIdempotent("struct Foo {\n\ta @0 :Int8;\n\tb @1 :Text;\n}\n");
  assertIdempotent("interface Persistent @0 (Ref) {\n  save @0 () -> (ref :Ref);\n}\n");
}

}  // namespace
}  // namespace compiler
}  // namespace zap
