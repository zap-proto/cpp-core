// Copyright (c) 2024 ZAP contributors
// Licensed under the MIT License:
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once

#include <kj/common.h>
#include <kj/array.h>
#include <zap/common.h>

ZAP_BEGIN_HEADER

namespace zap {
namespace compiler {

bool isWhitespaceSchema(kj::ArrayPtr<const char> input);
// Cheap heuristic: returns true if `input` appears to use the whitespace-significant
// schema syntax (i.e. there exists a block header line — struct/interface/enum/union/
// group, or `name :union`/`name :group` — that is NOT terminated by '{'). Pure brace
// files return false and bypass desugaring entirely (zero-cost back-compat).

kj::Array<char> desugar(kj::ArrayPtr<const char> input);
// Source preprocessor: rewrite ZAP whitespace-significant schema syntax into the
// equivalent brace/Cap'n-Proto syntax that the existing lexer/parser already accepts.
// Runs BEFORE the lexer so the proven brace front-end is untouched.
//
// Rules:
//  * Line-based. Indentation is measured in leading spaces. Blank lines and full-line
//    '#' comments are transparent (do not open/close blocks, do not affect indent).
//  * A block header is a line that in brace grammar would be followed by '{' but is not
//    already terminated by '{': `struct <Id>`, `interface <Id>` (optionally extends(...)),
//    `enum <Id>`, `union`, `group`, or `<name> :union` / `<name> :group`. A header that
//    already ends in '{' is brace syntax and is left UNTOUCHED along with its body
//    (files may mix styles per top-level decl).
//  * For a header at indent I, its body is the following lines with indent > I. A '{' is
//    appended to the header line and a matching '}' is emitted (at indent I) when indent
//    returns to <= I. Nesting recurses.
//  * Whitespace-mode fields `name Type` become `name @<n> :Type`, where <n> is the next
//    positional ordinal of the enclosing struct (a per-struct counter advanced in source
//    order across fields AND union/group members, per Cap'n Proto). Explicit `@N` and/or
//    `:Type` are preserved. Enum value `name` becomes `name @<n>`. Interface methods
//    keep/auto-assign `@N (params) -> (results)`.
//  * Output is valid brace source the existing parser already accepts. Brace input is
//    returned byte-identical (idempotent).

}  // namespace compiler
}  // namespace zap

ZAP_END_HEADER
