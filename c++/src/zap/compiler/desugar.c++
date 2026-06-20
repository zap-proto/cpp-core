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

#include "desugar.h"
#include <kj/vector.h>

namespace zap {
namespace compiler {

namespace {

// ---- low-level char helpers -------------------------------------------------

inline bool isIdentStart(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}
inline bool isIdentChar(char c) {
  return isIdentStart(c) || (c >= '0' && c <= '9');
}
inline bool isDigit(char c) { return c >= '0' && c <= '9'; }

// Number of leading indentation CHARACTERS (each ' ' or '\t' counts as one). This is the
// raw column span of the indent prefix; it is used only to SLICE the leading whitespace for
// re-emission and to choose the close-brace column, so emitted output preserves the source's
// exact indent bytes. Nesting decisions use indentWidth() instead. [audit: C3]
size_t leadingIndent(kj::ArrayPtr<const char> line) {
  size_t n = 0;
  while (n < line.size() && (line[n] == ' ' || line[n] == '\t')) ++n;
  return n;
}

// Tab stop used when expanding tabs to columns for nesting comparisons. A power of two so a
// tab always advances to the next multiple of kTabWidth.
constexpr size_t kTabWidth = 8;

// Visual indentation WIDTH in columns, expanding each '\t' to the next kTabWidth stop and
// each ' ' to one column. Nesting (which block a line belongs to) is decided on this width,
// not the raw character count, so a space-indented header and a tab-indented body — in either
// order — compare correctly instead of orphaning the body when one tab (1 char) tested as
// shallower than several spaces. This is the one canonical measure of "how deep" a line is.
// [audit: mixed space/tab indentation]
size_t indentWidth(kj::ArrayPtr<const char> line) {
  size_t col = 0;
  for (size_t i = 0; i < line.size(); ++i) {
    if (line[i] == ' ') ++col;
    else if (line[i] == '\t') col += kTabWidth - (col % kTabWidth);
    else break;
  }
  return col;
}

// Trim trailing whitespace (space, tab, \r) — returns a sub-slice.
kj::ArrayPtr<const char> rtrim(kj::ArrayPtr<const char> s) {
  size_t end = s.size();
  while (end > 0 && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r')) --end;
  return s.first(end);
}

// Trim leading whitespace (space, tab).
kj::ArrayPtr<const char> ltrim(kj::ArrayPtr<const char> s) {
  size_t start = 0;
  while (start < s.size() && (s[start] == ' ' || s[start] == '\t')) ++start;
  return s.slice(start, s.size());
}

kj::ArrayPtr<const char> trim(kj::ArrayPtr<const char> s) { return rtrim(ltrim(s)); }

bool startsWith(kj::ArrayPtr<const char> s, const char* prefix) {
  size_t i = 0;
  while (prefix[i] != '\0') {
    if (i >= s.size() || s[i] != prefix[i]) return false;
    ++i;
  }
  return true;
}

bool equals(kj::ArrayPtr<const char> s, const char* lit) {
  size_t i = 0;
  while (lit[i] != '\0') {
    if (i >= s.size() || s[i] != lit[i]) return false;
    ++i;
  }
  return i == s.size();
}

// First whitespace-delimited token of a trimmed line.
kj::ArrayPtr<const char> firstWord(kj::ArrayPtr<const char> t) {
  size_t i = 0;
  while (i < t.size() && t[i] != ' ' && t[i] != '\t') ++i;
  return t.first(i);
}

// Index of a trailing '#' comment marker (outside double-quoted strings), or t.size()
// if the line has no comment. Single source of truth for where code ends / comment begins.
size_t commentStart(kj::ArrayPtr<const char> t) {
  bool inStr = false;
  for (size_t i = 0; i < t.size(); ++i) {
    char c = t[i];
    if (c == '"') inStr = !inStr;
    else if (c == '#' && !inStr) return i;
  }
  return t.size();
}

// Drop a trailing '#' comment (outside double-quoted strings) — the code part. The member
// and header emitters split a line at commentStart() directly (to preserve the original
// whitespace gap before the comment when re-emitting it after the synthesized ';' / '{'). [audit: C1, C2]
kj::ArrayPtr<const char> stripTrailingComment(kj::ArrayPtr<const char> t) {
  return t.first(commentStart(t));
}

// Net brace depth change on a code line (ignoring strings/comments).
int braceDelta(kj::ArrayPtr<const char> t) {
  auto code = stripTrailingComment(t);
  bool inStr = false;
  int d = 0;
  for (size_t i = 0; i < code.size(); ++i) {
    char c = code[i];
    if (c == '"') inStr = !inStr;
    else if (c == '{' && !inStr) ++d;
    else if (c == '}' && !inStr) --d;
  }
  return d;
}

// True if the line contains any '{' (outside strings/comments). Such a line is brace
// syntax — whether it OPENS a block (`struct Foo {`) or is a complete inline declaration
// (`struct Foo { x @0 :T; }`) — and is passed through untouched. `braceDelta` then tracks
// whether a verbatim region remains open (net > 0) or the line was self-contained (net 0).
bool containsBrace(kj::ArrayPtr<const char> t) {
  auto code = stripTrailingComment(t);
  bool inStr = false;
  for (size_t i = 0; i < code.size(); ++i) {
    char c = code[i];
    if (c == '"') inStr = !inStr;
    else if (c == '{' && !inStr) return true;
  }
  return false;
}

bool isIdent(kj::ArrayPtr<const char> s) {
  if (s.size() == 0 || !isIdentStart(s[0])) return false;
  for (size_t i = 1; i < s.size(); ++i) {
    if (!isIdentChar(s[i])) return false;
  }
  return true;
}

// Leading identifier (field/value/method name); returns its length (0 if none).
size_t identLen(kj::ArrayPtr<const char> t) {
  if (t.size() == 0 || !isIdentStart(t[0])) return 0;
  size_t i = 1;
  while (i < t.size() && isIdentChar(t[i])) ++i;
  return i;
}

// ZAP ordinals are 16-bit; the parser is the single authority that REJECTS an out-of-range
// ordinal — `parsers.ordinal` reports "Ordinals cannot be greater than 65535." at the exact
// source span for any value >= 65536 (parser.c++). The desugar therefore does NOT re-validate
// (no duplicated policy, no throw across the un-tried call sites): it emits the field — including
// an out-of-range explicit `@N` — verbatim, and the parser produces the clean file:line:col
// diagnostic. The desugar's ONE remaining ordinal duty is to advance the per-scope auto-ordinal
// counter WITHOUT wrapping a uint32, so a huge literal can never silently roll the next member's
// ordinal back into the valid range. [audit: H1 / graceful failure]
constexpr uint32_t kMaxOrdinal = 65535;

// The span of a member line where an ORDINAL may legally appear: from just after the leading
// name to the first ':' (type) or '(' (method params / type brand) — whichever comes first.
// An '@N' anywhere else (e.g. a generic/brand argument inside a TYPE, `b :Bar(@7)`, or a
// method's parameter list) is NOT this member's ordinal and must be ignored. This is the
// single authority for "where the ordinal can be". [audit: ordinal inside type]
kj::ArrayPtr<const char> ordinalSpan(kj::ArrayPtr<const char> t) {
  size_t nameLen = identLen(t);
  if (nameLen == 0) return t.first(0);  // no name → no ordinal position
  size_t end = nameLen;
  bool inStr = false;
  while (end < t.size()) {
    char c = t[end];
    if (c == '"') { inStr = !inStr; ++end; continue; }
    if (!inStr && (c == ':' || c == '(')) break;
    ++end;
  }
  return t.slice(nameLen, end);
}

// Detect an explicit `@<digits>` ordinal in ORDINAL POSITION (after the name, before the
// type/params), outside strings. Returns true if present and reports, via `outClamped`, the
// value CLAMPED to kMaxOrdinal + 1 (= 65536). Clamping (not exact capture) is deliberate: the
// only consumer is the counter advance, which must not wrap, and any value >= 65536 is rejected
// downstream by the parser regardless of its exact magnitude — so an astronomically long literal
// (@99999999999999999999) maps to the same 65536 sentinel without overflowing a uint32. The
// field text itself is emitted verbatim by the caller, so the parser still sees (and rejects) the
// original digits. [audit: H1, ordinal-inside-type]
bool explicitOrdinal(kj::ArrayPtr<const char> t, uint32_t& outClamped) {
  auto span = ordinalSpan(t);
  bool inStr = false;
  for (size_t i = 0; i < span.size(); ++i) {
    char c = span[i];
    if (c == '"') { inStr = !inStr; continue; }
    if (c == '@' && !inStr) {
      size_t j = i + 1;
      uint32_t v = 0;
      while (j < span.size() && isDigit(span[j])) {
        if (v <= kMaxOrdinal) {
          v = v * 10 + (uint32_t)(span[j] - '0');
          if (v > kMaxOrdinal) v = kMaxOrdinal + 1;  // clamp; saturates, never wraps
        }
        ++j;
      }
      if (j > i + 1) {
        outClamped = v;
        return true;
      }
    }
  }
  return false;
}

// Advance the per-scope ordinal counter past an explicit ordinal, saturating so a field
// declared `@65535` (or any out-of-range/huge literal, already clamped to 65536) cannot wrap
// `next` and silently reassign a low ordinal to the following member. The successor may reach
// kMaxOrdinal + 1; a subsequent auto field then emits an out-of-range ordinal that the parser
// rejects. [audit: H1]
void advanceNext(uint32_t& next, uint32_t explicitClamped) {
  next = (explicitClamped >= kMaxOrdinal + 1) ? kMaxOrdinal + 1 : explicitClamped + 1;
}

// ---- output buffer helpers --------------------------------------------------

void put(kj::Vector<char>& out, kj::ArrayPtr<const char> s) {
  out.addAll(s);
}
void put(kj::Vector<char>& out, const char* s) {
  for (size_t i = 0; s[i] != '\0'; ++i) out.add(s[i]);
}
void putUInt(kj::Vector<char>& out, uint32_t v) {
  char buf[16];
  int n = 0;
  if (v == 0) { buf[n++] = '0'; }
  else { while (v > 0) { buf[n++] = (char)('0' + (v % 10)); v /= 10; } }
  while (n > 0) out.add(buf[--n]);
}

// ---- header classification --------------------------------------------------

enum class HeaderKind {
  STRUCT, INTERFACE, ENUM, UNION, GROUP, NAMED_UNION, NAMED_GROUP, NONE
};

// Text following the first whitespace-delimited word, with surrounding space trimmed.
kj::ArrayPtr<const char> restAfterFirstWord(kj::ArrayPtr<const char> t) {
  return trim(t.slice(firstWord(t).size(), t.size()));
}

// A nested NON-FIELD declaration that may appear inside a struct/interface body: `using`,
// `const`, or `annotation`. These carry their own statement terminator (';') and take NO
// positional ordinal, so they must be passed through verbatim — never rewritten into a
// `name @n :Type` field. Detected by the leading keyword followed by a word boundary, so a
// field genuinely named `using`/`const`/`annotation` (followed by a type, not its decl
// syntax) is not misread. [audit: nested using/const/annotation]
bool isNestedDecl(kj::ArrayPtr<const char> t) {
  auto w = firstWord(t);
  // `const`/`using`/`annotation` as a field NAME would be `const :Type` / `using Type` —
  // the keyword's own declaration syntax is `using <Id> = ...`, `const <Id> :T = ...`,
  // `annotation <Id> (...) :T`, i.e. keyword + an identifier name. A field named after the
  // keyword is disambiguated exactly as in classifyHeader/emitField: a ':' or trailing '@N'
  // right after the name. Here we require keyword + identifier with neither, which is only
  // ever the declaration form.
  if (!(equals(w, "using") || equals(w, "const") || equals(w, "annotation"))) return false;
  auto rest = restAfterFirstWord(t);
  auto name = firstWord(rest);
  if (!isIdent(name)) return false;
  auto afterName = trim(rest.slice(name.size(), rest.size()));
  // Field forms that happen to start with the keyword: `using Type` (name `using`, type
  // `Type`) has afterName empty AND would be a header otherwise — but `using <Id>` decl is
  // distinguished because a real `using` decl always continues with '=' (alias) and `const`
  // with ':' or '=', and `annotation` with '('. A bare `keyword Ident` with nothing after is
  // the ambiguous case; treat it as a FIELD named after the keyword (matches emitField/H4),
  // not a declaration, so `using Foo` (field `using` of type `Foo`) stays a field.
  if (afterName.size() == 0) return false;
  char c = afterName[0];
  if (equals(w, "using"))      return c == '=';
  if (equals(w, "const"))      return c == ':' || c == '=';
  /* annotation */             return c == '(' || c == ':';
}

// True if `s` is a type NAME optionally followed by a generic parameter list: `<Id>` or
// `<Id>(...)` with balanced parens, and nothing after the ')'. This is the name shape of a
// generic struct/interface declaration header (`struct Map(Key, Value)`, `interface Cap(T)`).
// A trailing field marker (`struct :group`, `enum UInt8 @0`) is NOT this shape, so it is not
// mistaken for a header. [audit: generic struct/interface header]
bool isNameWithOptionalParams(kj::ArrayPtr<const char> s) {
  size_t nameLen = identLen(s);
  if (nameLen == 0) return false;
  auto after = trim(s.slice(nameLen, s.size()));
  if (after.size() == 0) return true;          // bare `<Id>`
  if (after[0] != '(') return false;           // anything else after the name → not a header
  // Balanced-paren scan to the matching ')'. Parameter lists do not nest in ZAP, but scan
  // depth-aware to be safe; require the ')' to be the final non-space token.
  int depth = 0;
  for (size_t i = 0; i < after.size(); ++i) {
    char c = after[i];
    if (c == '(') ++depth;
    else if (c == ')') {
      if (--depth == 0) {
        return trim(after.slice(i + 1, after.size())).size() == 0;  // nothing after ')'
      }
    }
  }
  return false;  // unbalanced
}

HeaderKind classifyHeader(kj::ArrayPtr<const char> t) {
  // A braceless header is keyword + the keyword's own name shape + end-of-line — NOT a
  // `name type` field line whose name merely happens to be a keyword (e.g. `union Int8`,
  // a field named `union`, or `struct :group`, a field named `struct`). Strip any trailing
  // comment first so a commented header (`payload :union # tag`) is still recognized. [audit: C4, H4]
  t = trim(stripTrailingComment(t));
  auto w = firstWord(t);
  auto rest = restAfterFirstWord(t);

  // struct: keyword + an identifier, optionally with a generic parameter list `(K, V)`.
  if (equals(w, "struct") && isNameWithOptionalParams(rest)) return HeaderKind::STRUCT;
  // enum: keyword + exactly one identifier (enums are never generic), nothing after.
  if (equals(w, "enum") && isIdent(rest)) return HeaderKind::ENUM;

  // interface: keyword + one identifier, optionally followed by a generic parameter list `(T)`
  // and/or an `extends(...)` clause. The name ends at the first non-identifier char (so
  // `Cap(T)` splits into name `Cap` + `(T)`), unlike firstWord which only splits on spaces.
  if (equals(w, "interface")) {
    size_t nameLen = identLen(rest);
    if (nameLen > 0) {
      auto afterName = trim(rest.slice(nameLen, rest.size()));
      if (afterName.size() == 0 || afterName[0] == '(' || startsWith(afterName, "extends")) {
        return HeaderKind::INTERFACE;
      }
    }
  }

  // union/group: bare keyword (anonymous), nothing after. `union Int8` is a field, not a header.
  if (equals(w, "union") && rest.size() == 0) return HeaderKind::UNION;
  if (equals(w, "group") && rest.size() == 0) return HeaderKind::GROUP;

  // `name :union` / `name :group` (named, share the enclosing struct's ordinals).
  for (size_t i = 0; i < t.size(); ++i) {
    if (t[i] == ':') {
      auto before = trim(t.first(i));
      auto after = trim(t.slice(i + 1, t.size()));
      if (isIdent(before)) {
        if (equals(after, "union")) return HeaderKind::NAMED_UNION;
        if (equals(after, "group")) return HeaderKind::NAMED_GROUP;
      }
      break;  // only consider the first ':'
    }
  }
  return HeaderKind::NONE;
}

// ---- member transforms ------------------------------------------------------

// `name Type` -> `name @n :Type`; explicit @N preserved (counter jumps to N+1).
void emitField(kj::Vector<char>& out, kj::ArrayPtr<const char> t, uint32_t& next) {
  uint32_t explicitClamped;
  if (explicitOrdinal(t, explicitClamped)) {
    advanceNext(next, explicitClamped);
    put(out, t);
    return;
  }
  size_t nameLen = identLen(t);
  if (nameLen == 0) { put(out, t); return; }
  auto name = t.first(nameLen);
  auto rest = trim(t.slice(nameLen, t.size()));
  uint32_t n = next++;
  put(out, name);
  put(out, " @");
  putUInt(out, n);
  if (rest.size() == 0) {
    // bare 'name' in struct context.
  } else if (rest[0] == ':') {
    // typed but no ordinal: 'name :Type' -> 'name @n :Type'
    out.add(' ');
    put(out, rest);
  } else {
    // 'name Type [default...]' -> 'name @n :Type [default...]'
    put(out, " :");
    put(out, rest);
  }
}

// enum value 'name' -> 'name @n'; explicit @N preserved.
void emitEnumValue(kj::Vector<char>& out, kj::ArrayPtr<const char> t, uint32_t& next) {
  uint32_t explicitClamped;
  if (explicitOrdinal(t, explicitClamped)) {
    advanceNext(next, explicitClamped);
    put(out, t);
    return;
  }
  size_t nameLen = identLen(t);
  if (nameLen == 0) { put(out, t); return; }
  auto name = t.first(nameLen);
  auto rest = trim(t.slice(nameLen, t.size()));
  uint32_t n = next++;
  put(out, name);
  put(out, " @");
  putUInt(out, n);
  if (rest.size() > 0) { out.add(' '); put(out, rest); }
}

// interface method 'foo (params) -> (results)' -> 'foo @n (params) -> (results)'.
void emitMethod(kj::Vector<char>& out, kj::ArrayPtr<const char> t, uint32_t& next) {
  uint32_t explicitClamped;
  if (explicitOrdinal(t, explicitClamped)) {
    advanceNext(next, explicitClamped);
    put(out, t);
    return;
  }
  size_t nameLen = identLen(t);
  if (nameLen == 0) { put(out, t); return; }
  auto name = t.first(nameLen);
  auto rest = trim(t.slice(nameLen, t.size()));
  uint32_t n = next++;
  put(out, name);
  put(out, " @");
  putUInt(out, n);
  if (rest.size() > 0) { out.add(' '); put(out, rest); }
}

// ---- frame stack ------------------------------------------------------------

enum class FrameKind { STRUCT_LIKE, ENUM, INTERFACE, FILE };

struct Frame {
  size_t indent;  // raw leading-char count of the header — used only to slice/emit the close
                  // brace at the header's exact column (preserves source indent bytes).
  size_t width;   // expanded indent WIDTH (tabs → columns) of the header — the value dedent
                  // compares against, so tab/space mixes nest correctly. [audit: mixed indent]
  FrameKind kind;
  size_t ord;  // index into ords[], or SIZE_MAX
};

void emitClose(kj::Vector<char>& out, size_t indent) {
  for (size_t i = 0; i < indent; ++i) out.add(' ');
  out.add('}');
  out.add('\n');
}

}  // namespace

bool isWhitespaceSchema(kj::ArrayPtr<const char> input) {
  // Scan lines; a whitespace schema has at least one header line not ending in '{'.
  // Brace-block bodies are skipped so a brace `struct Foo {` followed by an unbraced
  // member never triggers a false positive.
  int verbatimDepth = 0;
  size_t pos = 0;
  while (pos < input.size()) {
    size_t nl = pos;
    while (nl < input.size() && input[nl] != '\n') ++nl;
    auto line = input.slice(pos, nl);
    pos = (nl < input.size()) ? nl + 1 : input.size();

    auto t = rtrim(ltrim(line));
    if (t.size() == 0 || t[0] == '#') continue;
    if (verbatimDepth > 0) { verbatimDepth += braceDelta(t); continue; }
    if (containsBrace(t)) { verbatimDepth += braceDelta(t); continue; }
    if (classifyHeader(t) != HeaderKind::NONE) return true;
  }
  return false;
}

kj::Array<char> desugar(kj::ArrayPtr<const char> input) {
  kj::Vector<char> out(input.size() + input.size() / 8 + 16);
  kj::Vector<uint32_t> ords;
  kj::Vector<Frame> frames;
  frames.add(Frame { 0, 0, FrameKind::FILE, SIZE_MAX });
  int verbatimDepth = 0;

  // Transparent lines (blank / full-line comment) buffered since the last non-transparent
  // line. Flushed AFTER any dedent close-braces so a blank line between two top-level decls
  // lands after the inner block's '}', matching idiomatic brace layout.
  kj::Vector<kj::ArrayPtr<const char>> pending;
  auto flushPending = [&]() {
    for (auto& p: pending) put(out, p);
    pending.clear();
  };

  size_t pos = 0;
  while (pos < input.size()) {
    // Slice one physical line INCLUDING its trailing '\n' (if any).
    size_t nl = pos;
    while (nl < input.size() && input[nl] != '\n') ++nl;
    bool hasNl = (nl < input.size());
    auto raw = input.slice(pos, hasNl ? nl + 1 : nl);
    pos = hasNl ? nl + 1 : input.size();

    // Content = raw minus trailing \n and optional preceding \r.
    size_t contentEnd = raw.size();
    kj::ArrayPtr<const char> eol = raw.slice(raw.size(), raw.size());  // empty
    if (hasNl) {
      contentEnd -= 1;  // drop '\n'
      if (contentEnd > 0 && raw[contentEnd - 1] == '\r') {
        contentEnd -= 1;  // drop '\r'
        eol = raw.slice(raw.size() - 2, raw.size());  // "\r\n"
      } else {
        eol = raw.slice(raw.size() - 1, raw.size());  // "\n"
      }
    }
    auto content = raw.first(contentEnd);
    size_t indent = leadingIndent(content);        // raw chars (for slicing / close column)
    size_t indentCols = indentWidth(content);      // expanded columns (for nesting) [audit]
    auto trimmed = rtrim(content.slice(indent, content.size()));

    // Transparent lines: blank or full-line comment — no structure. Buffer it.
    if (trimmed.size() == 0 || trimmed[0] == '#') {
      pending.add(raw);
      continue;
    }

    // Inside a verbatim brace region: pass through, track brace depth only.
    if (verbatimDepth > 0) {
      flushPending();
      put(out, raw);
      verbatimDepth += braceDelta(trimmed);
      continue;
    }

    // Dedent: close whitespace frames whose indent width >= current indent width. The
    // comparison is on expanded columns so a tab-indented body and a space-indented header
    // (in either order) nest/close correctly instead of orphaning fields. [audit: mixed indent]
    while (frames.size() > 1) {
      Frame& top = frames[frames.size() - 1];
      if (top.kind == FrameKind::FILE) break;
      if (indentCols <= top.width) {
        size_t fi = top.indent;
        frames.removeLast();
        emitClose(out, fi);
      } else {
        break;
      }
    }

    // Flush buffered transparent lines (after closes, before this line's content).
    flushPending();

    auto leadingWs = content.first(indent);

    // Brace syntax (line contains '{'): emit verbatim. If it opens a block (net depth > 0)
    // enter a verbatim region; if it's a self-contained inline decl (net 0) nothing else
    // is needed. Either way the proven brace front-end handles it untouched.
    if (containsBrace(trimmed)) {
      put(out, raw);
      verbatimDepth += braceDelta(trimmed);
      continue;
    }

    FrameKind parentKind = frames[frames.size() - 1].kind;
    size_t parentOrd = frames[frames.size() - 1].ord;

    HeaderKind hk = classifyHeader(trimmed);
    if (hk != HeaderKind::NONE) {
      // Open the block with '{' placed BEFORE any trailing comment, so the brace is not
      // swallowed into the comment (which would leave braces unbalanced). A commented
      // header `struct S # hi` becomes `struct S { # hi`, matching the TS/Go desugarers.
      // The original whitespace gap before the comment is preserved. [audit: C1]
      size_t ci = commentStart(trimmed);
      auto code = rtrim(trimmed.first(ci));
      auto gap = trimmed.slice(code.size(), ci);  // whitespace between code and '#'
      auto comment = trimmed.slice(ci, trimmed.size());
      put(out, leadingWs);
      put(out, code);
      put(out, " {");
      if (comment.size() > 0) { put(out, gap); put(out, comment); }
      if (eol.size() > 0) put(out, eol); else out.add('\n');

      FrameKind fk;
      switch (hk) {
        case HeaderKind::INTERFACE: fk = FrameKind::INTERFACE; break;
        case HeaderKind::ENUM:      fk = FrameKind::ENUM; break;
        default:                    fk = FrameKind::STRUCT_LIKE; break;
      }

      // Ordinal ownership (ZAP): union/group members — named or anonymous —
      // share the enclosing struct's ordinal sequence. Only struct/interface/enum
      // introduce a fresh ordinal space.
      size_t ord;
      if (hk == HeaderKind::STRUCT || hk == HeaderKind::INTERFACE ||
          hk == HeaderKind::ENUM) {
        ords.add(0);
        ord = ords.size() - 1;
      } else {
        if (parentOrd != SIZE_MAX) {
          ord = parentOrd;
        } else {
          ords.add(0);
          ord = ords.size() - 1;
        }
      }
      frames.add(Frame { indent, indentCols, fk, ord });
    } else {
      put(out, leadingWs);
      // Split off any trailing comment FIRST so the synthesized ';' terminator lands on the
      // code, not after the comment. A field `a Int8  # doc` becomes `a @0 :Int8;  # doc`
      // — terminated — instead of `a @0 :Int8  # doc;`, which left the field unterminated.
      // The original whitespace gap before the comment is preserved. [audit: C2]
      size_t ci = commentStart(trimmed);
      auto code = rtrim(trimmed.first(ci));
      auto gap = trimmed.slice(code.size(), ci);  // whitespace between code and '#'
      auto comment = trimmed.slice(ci, trimmed.size());

      // Members inside struct/enum/interface scopes: the newline is the brace statement
      // terminator, so emit a trailing ';' (unless one is already present). At file level,
      // top-level decls (const/using/@id/annotation) carry their own terminator: pass through.
      bool terminate = (parentKind != FrameKind::FILE);
      size_t before = out.size();
      // A nested declaration (`using`/`const`/`annotation`) inside a struct/interface body
      // is NOT a field: it carries no positional ordinal and its own ';' terminator, so it
      // passes through verbatim. The `terminate` block below adds the ';' if the source
      // omitted it (the newline was the terminator), matching field handling. [audit: nested
      // using/const/annotation]
      if (terminate && isNestedDecl(code)) {
        put(out, code);
      } else {
        switch (parentKind) {
          case FrameKind::ENUM:        emitEnumValue(out, code, ords[parentOrd]); break;
          case FrameKind::INTERFACE:   emitMethod(out, code, ords[parentOrd]); break;
          case FrameKind::STRUCT_LIKE: emitField(out, code, ords[parentOrd]); break;
          case FrameKind::FILE:        put(out, code); break;
        }
      }
      if (terminate) {
        // Add ';' only if the emitted code isn't already terminated — never double it.
        auto emitted = rtrim(out.asPtr().slice(before, out.size()));
        if (!(emitted.size() > 0 && emitted[emitted.size() - 1] == ';')) {
          out.add(';');
        }
      }
      if (comment.size() > 0) { put(out, gap); put(out, comment); }
      if (eol.size() > 0) put(out, eol); else out.add('\n');
    }
  }

  while (frames.size() > 1) {
    Frame& top = frames[frames.size() - 1];
    size_t fi = top.indent;
    frames.removeLast();
    emitClose(out, fi);
  }
  // Trailing transparent lines land after the final close brace.
  flushPending();

  return out.releaseAsArray();
}

}  // namespace compiler
}  // namespace zap
