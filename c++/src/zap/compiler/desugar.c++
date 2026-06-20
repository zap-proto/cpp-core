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

// Number of leading ' ' (space) characters. Tabs are not treated as indentation
// (consistent with the lexer, which does not expand tabs); a tab simply stops the count.
size_t leadingSpaces(kj::ArrayPtr<const char> line) {
  size_t n = 0;
  while (n < line.size() && line[n] == ' ') ++n;
  return n;
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

// Drop a trailing '#' comment (outside double-quoted strings).
kj::ArrayPtr<const char> stripTrailingComment(kj::ArrayPtr<const char> t) {
  bool inStr = false;
  for (size_t i = 0; i < t.size(); ++i) {
    char c = t[i];
    if (c == '"') inStr = !inStr;
    else if (c == '#' && !inStr) return t.first(i);
  }
  return t;
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

// Find explicit `@<digits>` ordinal token (outside strings). Returns true + value.
bool explicitOrdinal(kj::ArrayPtr<const char> t, uint32_t& outVal) {
  bool inStr = false;
  for (size_t i = 0; i < t.size(); ++i) {
    char c = t[i];
    if (c == '"') { inStr = !inStr; continue; }
    if (c == '@' && !inStr) {
      size_t j = i + 1;
      uint32_t v = 0;
      while (j < t.size() && isDigit(t[j])) { v = v * 10 + (uint32_t)(t[j] - '0'); ++j; }
      if (j > i + 1) { outVal = v; return true; }
    }
  }
  return false;
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

HeaderKind classifyHeader(kj::ArrayPtr<const char> t) {
  auto w = firstWord(t);
  if (equals(w, "struct")) return HeaderKind::STRUCT;
  if (equals(w, "interface")) return HeaderKind::INTERFACE;
  if (equals(w, "enum")) return HeaderKind::ENUM;
  if (equals(w, "union")) return HeaderKind::UNION;
  if (equals(w, "group")) return HeaderKind::GROUP;

  // `name :union` / `name :group`
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
  uint32_t explicitN;
  if (explicitOrdinal(t, explicitN)) {
    next = explicitN + 1;
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
  uint32_t explicitN;
  if (explicitOrdinal(t, explicitN)) {
    next = explicitN + 1;
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
  uint32_t explicitN;
  if (explicitOrdinal(t, explicitN)) {
    next = explicitN + 1;
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
  size_t indent;
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
  frames.add(Frame { 0, FrameKind::FILE, SIZE_MAX });
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
    size_t indent = leadingSpaces(content);
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

    // Dedent: close whitespace frames whose indent >= current indent.
    while (frames.size() > 1) {
      Frame& top = frames[frames.size() - 1];
      if (top.kind == FrameKind::FILE) break;
      if (indent <= top.indent) {
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
      put(out, leadingWs);
      put(out, trimmed);
      put(out, " {");
      if (eol.size() > 0) put(out, eol); else out.add('\n');

      FrameKind fk;
      switch (hk) {
        case HeaderKind::INTERFACE: fk = FrameKind::INTERFACE; break;
        case HeaderKind::ENUM:      fk = FrameKind::ENUM; break;
        default:                    fk = FrameKind::STRUCT_LIKE; break;
      }

      // Ordinal ownership (Cap'n Proto): union/group members — named or anonymous —
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
      frames.add(Frame { indent, fk, ord });
    } else {
      put(out, leadingWs);
      // Members inside struct/enum/interface scopes: the newline is the brace statement
      // terminator, so emit a trailing ';' (unless one is already present). At file level,
      // top-level decls (const/using/@id/annotation) carry their own terminator: pass through.
      bool terminate = (parentKind != FrameKind::FILE);
      size_t before = out.size();
      switch (parentKind) {
        case FrameKind::ENUM:        emitEnumValue(out, trimmed, ords[parentOrd]); break;
        case FrameKind::INTERFACE:   emitMethod(out, trimmed, ords[parentOrd]); break;
        case FrameKind::STRUCT_LIKE: emitField(out, trimmed, ords[parentOrd]); break;
        case FrameKind::FILE:        put(out, trimmed); break;
      }
      if (terminate) {
        // Inspect the just-emitted member text (after any trailing '#' comment is ignored)
        // and add ';' only if not already terminated — never double it.
        auto emitted = rtrim(stripTrailingComment(out.asPtr().slice(before, out.size())));
        if (!(emitted.size() > 0 && emitted[emitted.size() - 1] == ';')) {
          out.add(';');
        }
      }
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
