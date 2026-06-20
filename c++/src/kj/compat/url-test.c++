// Copyright (c) 2017 Cloudflare, Inc. and contributors
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

#include "url.h"
#include <kj/debug.h>
#include <kj/test.h>

namespace kj {
namespace {

Url parseAndCheck(kj::StringPtr originalText, kj::StringPtr expectedRestringified = nullptr,
                  Url::Options options = {}) {
  if (expectedRestringified == nullptr) expectedRestringified = originalText;
  auto url = Url::parse(originalText, Url::REMOTE_HREF, options);
  KJ_EXPECT(kj::str(url) == expectedRestringified, url, originalText, expectedRestringified);
  // Make sure clones also restringify to the expected string.
  auto clone = url.clone();
  KJ_EXPECT(kj::str(clone) == expectedRestringified, clone, originalText, expectedRestringified);
  return url;
}

static constexpr Url::Options NO_DECODE {
  false,  // percentDecode
  false,  // allowEmpty
};

static constexpr Url::Options ALLOW_EMPTY {
  true,    // percentDecode
  true,    // allowEmpty
};

KJ_TEST("parse / stringify URL") {
  {
    auto url = parseAndCheck("https://zap.org");
    KJ_EXPECT(url.scheme == "https");
    KJ_EXPECT(url.userInfo == kj::none);
    KJ_EXPECT(url.host == "zap.org");
    KJ_EXPECT(url.path == nullptr);
    KJ_EXPECT(!url.hasTrailingSlash);
    KJ_EXPECT(url.query == nullptr);
    KJ_EXPECT(url.fragment == kj::none);
  }

  {
    auto url = parseAndCheck("https://zap.org:80");
    KJ_EXPECT(url.scheme == "https");
    KJ_EXPECT(url.userInfo == kj::none);
    KJ_EXPECT(url.host == "zap.org:80");
    KJ_EXPECT(url.path == nullptr);
    KJ_EXPECT(!url.hasTrailingSlash);
    KJ_EXPECT(url.query == nullptr);
    KJ_EXPECT(url.fragment == kj::none);
  }

  {
    auto url = parseAndCheck("https://zap.org/");
    KJ_EXPECT(url.scheme == "https");
    KJ_EXPECT(url.userInfo == kj::none);
    KJ_EXPECT(url.host == "zap.org");
    KJ_EXPECT(url.path == nullptr);
    KJ_EXPECT(url.hasTrailingSlash);
    KJ_EXPECT(url.query == nullptr);
    KJ_EXPECT(url.fragment == kj::none);
  }

  {
    auto url = parseAndCheck("https://zap.org/foo/bar");
    KJ_EXPECT(url.scheme == "https");
    KJ_EXPECT(url.userInfo == kj::none);
    KJ_EXPECT(url.host == "zap.org");
    KJ_EXPECT(url.path.asPtr() == kj::ArrayPtr<const StringPtr>({"foo", "bar"}));
    KJ_EXPECT(!url.hasTrailingSlash);
    KJ_EXPECT(url.query == nullptr);
    KJ_EXPECT(url.fragment == kj::none);
  }

  {
    auto url = parseAndCheck("https://zap.org/foo/bar/");
    KJ_EXPECT(url.scheme == "https");
    KJ_EXPECT(url.userInfo == kj::none);
    KJ_EXPECT(url.host == "zap.org");
    KJ_EXPECT(url.path.asPtr() == kj::ArrayPtr<const StringPtr>({"foo", "bar"}));
    KJ_EXPECT(url.hasTrailingSlash);
    KJ_EXPECT(url.query == nullptr);
    KJ_EXPECT(url.fragment == kj::none);
  }

  {
    auto url = parseAndCheck("https://zap.org/foo/bar?baz=qux&corge#garply");
    KJ_EXPECT(url.scheme == "https");
    KJ_EXPECT(url.userInfo == kj::none);
    KJ_EXPECT(url.host == "zap.org");
    KJ_EXPECT(url.path.asPtr() == kj::ArrayPtr<const StringPtr>({"foo", "bar"}));
    KJ_EXPECT(!url.hasTrailingSlash);
    KJ_ASSERT(url.query.size() == 2);
    KJ_EXPECT(url.query[0].name == "baz");
    KJ_EXPECT(url.query[0].value == "qux");
    KJ_EXPECT(url.query[1].name == "corge");
    KJ_EXPECT(url.query[1].value == nullptr);
    KJ_EXPECT(KJ_ASSERT_NONNULL(url.fragment) == "garply");
  }

  {
    auto url = parseAndCheck("https://zap.org/foo/bar?baz=qux&corge=#garply");
    KJ_EXPECT(url.scheme == "https");
    KJ_EXPECT(url.userInfo == kj::none);
    KJ_EXPECT(url.host == "zap.org");
    KJ_EXPECT(url.path.asPtr() == kj::ArrayPtr<const StringPtr>({"foo", "bar"}));
    KJ_EXPECT(!url.hasTrailingSlash);
    KJ_ASSERT(url.query.size() == 2);
    KJ_EXPECT(url.query[0].name == "baz");
    KJ_EXPECT(url.query[0].value == "qux");
    KJ_EXPECT(url.query[1].name == "corge");
    KJ_EXPECT(url.query[1].value == nullptr);
    KJ_EXPECT(KJ_ASSERT_NONNULL(url.fragment) == "garply");
  }

  {
    auto url = parseAndCheck("https://zap.org/foo/bar?baz=&corge=grault#garply");
    KJ_EXPECT(url.scheme == "https");
    KJ_EXPECT(url.userInfo == kj::none);
    KJ_EXPECT(url.host == "zap.org");
    KJ_EXPECT(url.path.asPtr() == kj::ArrayPtr<const StringPtr>({"foo", "bar"}));
    KJ_EXPECT(!url.hasTrailingSlash);
    KJ_ASSERT(url.query.size() == 2);
    KJ_EXPECT(url.query[0].name == "baz");
    KJ_EXPECT(url.query[0].value == "");
    KJ_EXPECT(url.query[1].name == "corge");
    KJ_EXPECT(url.query[1].value == "grault");
    KJ_EXPECT(KJ_ASSERT_NONNULL(url.fragment) == "garply");
  }

  {
    auto url = parseAndCheck("https://zap.org/foo/bar/?baz=qux&corge=grault#garply");
    KJ_EXPECT(url.scheme == "https");
    KJ_EXPECT(url.userInfo == kj::none);
    KJ_EXPECT(url.host == "zap.org");
    KJ_EXPECT(url.path.asPtr() == kj::ArrayPtr<const StringPtr>({"foo", "bar"}));
    KJ_EXPECT(url.hasTrailingSlash);
    KJ_ASSERT(url.query.size() == 2);
    KJ_EXPECT(url.query[0].name == "baz");
    KJ_EXPECT(url.query[0].value == "qux");
    KJ_EXPECT(url.query[1].name == "corge");
    KJ_EXPECT(url.query[1].value == "grault");
    KJ_EXPECT(KJ_ASSERT_NONNULL(url.fragment) == "garply");
  }

  {
    auto url = parseAndCheck("https://zap.org/foo/bar?baz=qux#garply");
    KJ_EXPECT(url.scheme == "https");
    KJ_EXPECT(url.userInfo == kj::none);
    KJ_EXPECT(url.host == "zap.org");
    KJ_EXPECT(url.path.asPtr() == kj::ArrayPtr<const StringPtr>({"foo", "bar"}));
    KJ_EXPECT(!url.hasTrailingSlash);
    KJ_ASSERT(url.query.size() == 1);
    KJ_EXPECT(url.query[0].name == "baz");
    KJ_EXPECT(url.query[0].value == "qux");
    KJ_EXPECT(KJ_ASSERT_NONNULL(url.fragment) == "garply");
  }

  {
    auto url = parseAndCheck("https://zap.org/foo?bar%20baz=qux+quux",
                             "https://zap.org/foo?bar+baz=qux+quux");
    KJ_ASSERT(url.query.size() == 1);
    KJ_EXPECT(url.query[0].name == "bar baz");
    KJ_EXPECT(url.query[0].value == "qux quux");
  }

  {
    auto url = parseAndCheck("https://zap.org/foo/bar#garply");
    KJ_EXPECT(url.scheme == "https");
    KJ_EXPECT(url.userInfo == kj::none);
    KJ_EXPECT(url.host == "zap.org");
    KJ_EXPECT(url.path.asPtr() == kj::ArrayPtr<const StringPtr>({"foo", "bar"}));
    KJ_EXPECT(!url.hasTrailingSlash);
    KJ_EXPECT(url.query == nullptr);
    KJ_EXPECT(KJ_ASSERT_NONNULL(url.fragment) == "garply");
  }

  {
    auto url = parseAndCheck("https://zap.org/foo/bar/#garply");
    KJ_EXPECT(url.scheme == "https");
    KJ_EXPECT(url.userInfo == kj::none);
    KJ_EXPECT(url.host == "zap.org");
    KJ_EXPECT(url.path.asPtr() == kj::ArrayPtr<const StringPtr>({"foo", "bar"}));
    KJ_EXPECT(url.hasTrailingSlash);
    KJ_EXPECT(url.query == nullptr);
    KJ_EXPECT(KJ_ASSERT_NONNULL(url.fragment) == "garply");
  }

  {
    auto url = parseAndCheck("https://zap.org#garply");
    KJ_EXPECT(url.scheme == "https");
    KJ_EXPECT(url.userInfo == kj::none);
    KJ_EXPECT(url.host == "zap.org");
    KJ_EXPECT(url.path == nullptr);
    KJ_EXPECT(!url.hasTrailingSlash);
    KJ_EXPECT(url.query == nullptr);
    KJ_EXPECT(KJ_ASSERT_NONNULL(url.fragment) == "garply");
  }

  {
    auto url = parseAndCheck("https://zap.org/#garply");
    KJ_EXPECT(url.scheme == "https");
    KJ_EXPECT(url.userInfo == kj::none);
    KJ_EXPECT(url.host == "zap.org");
    KJ_EXPECT(url.path == nullptr);
    KJ_EXPECT(url.hasTrailingSlash);
    KJ_EXPECT(url.query == nullptr);
    KJ_EXPECT(KJ_ASSERT_NONNULL(url.fragment) == "garply");
  }

  {
    auto url = parseAndCheck("https://foo@zap.org");
    KJ_EXPECT(url.scheme == "https");
    auto& user = KJ_ASSERT_NONNULL(url.userInfo);
    KJ_EXPECT(user.username == "foo");
    KJ_EXPECT(user.password == kj::none);
    KJ_EXPECT(url.host == "zap.org");
    KJ_EXPECT(url.path == nullptr);
    KJ_EXPECT(!url.hasTrailingSlash);
    KJ_EXPECT(url.query == nullptr);
    KJ_EXPECT(url.fragment == kj::none);
  }

  {
    auto url = parseAndCheck("https://$foo&:12+,34@zap.org");
    KJ_EXPECT(url.scheme == "https");
    auto& user = KJ_ASSERT_NONNULL(url.userInfo);
    KJ_EXPECT(user.username == "$foo&");
    KJ_EXPECT(KJ_ASSERT_NONNULL(user.password) == "12+,34");
    KJ_EXPECT(url.host == "zap.org");
    KJ_EXPECT(url.path == nullptr);
    KJ_EXPECT(!url.hasTrailingSlash);
    KJ_EXPECT(url.query == nullptr);
    KJ_EXPECT(url.fragment == kj::none);
  }

  {
    auto url = parseAndCheck("https://[2001:db8::1234]:80/foo");
    KJ_EXPECT(url.scheme == "https");
    KJ_EXPECT(url.userInfo == kj::none);
    KJ_EXPECT(url.host == "[2001:db8::1234]:80");
    KJ_EXPECT(url.path.asPtr() == kj::ArrayPtr<const StringPtr>({"foo"}));
    KJ_EXPECT(!url.hasTrailingSlash);
    KJ_EXPECT(url.query == nullptr);
    KJ_EXPECT(url.fragment == kj::none);
  }

  {
    auto url = parseAndCheck("https://zap.org/foo%2Fbar/baz");
    KJ_EXPECT(url.path.asPtr() == kj::ArrayPtr<const StringPtr>({"foo/bar", "baz"}));
  }

  parseAndCheck("https://zap.org/foo/bar?", "https://zap.org/foo/bar");
  parseAndCheck("https://zap.org/foo/bar?#", "https://zap.org/foo/bar#");
  parseAndCheck("https://zap.org/foo/bar#");

  // Scheme and host are forced to lower-case.
  parseAndCheck("hTtP://zAp.org/fOo/bAr", "http://zap.org/fOo/bAr");

  // URLs with underscores in their hostnames are allowed, but you probably shouldn't use them. They
  // are not valid domain names.
  parseAndCheck("https://bad_domain.zap.org/");

  // Make sure URLs with %-encoded '%' signs in their userinfo, path, query, and fragment components
  // get correctly re-encoded.
  parseAndCheck("https://foo%25bar:baz%25qux@zap.org/");
  parseAndCheck("https://zap.org/foo%25bar");
  parseAndCheck("https://zap.org/?foo%25bar=baz%25qux");
  parseAndCheck("https://zap.org/#foo%25bar");

  // Make sure redundant /'s and &'s aren't collapsed when options.removeEmpty is false.
  parseAndCheck("https://zap.org/foo//bar///test//?foo=bar&&baz=qux&", nullptr, ALLOW_EMPTY);

  // "." and ".." are still processed, though.
  parseAndCheck("https://zap.org/foo//../bar/.",
                "https://zap.org/foo/bar/", ALLOW_EMPTY);

  {
    auto url = parseAndCheck("https://foo/", nullptr, ALLOW_EMPTY);
    KJ_EXPECT(url.path.size() == 0);
    KJ_EXPECT(url.hasTrailingSlash);
  }

  {
    auto url = parseAndCheck("https://foo/bar/", nullptr, ALLOW_EMPTY);
    KJ_EXPECT(url.path.size() == 1);
    KJ_EXPECT(url.hasTrailingSlash);
  }
}

KJ_TEST("URL percent encoding") {
  parseAndCheck(
      "https://b%6fb:%61bcd@zapr%6fto.org/f%6fo?b%61r=b%61z#q%75x",
      "https://bob:abcd@zap.org/foo?bar=baz#qux");

  parseAndCheck(
      "https://b\001b:\001bcd@zap.org/f\001o?b\001r=b\001z#q\001x",
      "https://b%01b:%01bcd@zap.org/f%01o?b%01r=b%01z#q%01x");

  parseAndCheck(
      "https://b b: bcd@zap.org/f o?b r=b z#q x",
      "https://b%20b:%20bcd@zap.org/f%20o?b+r=b+z#q%20x");

  parseAndCheck(
      "https://zap.org/foo?bar=baz#@?#^[\\]{|}",
      "https://zap.org/foo?bar=baz#@?#^[\\]{|}");

  // All permissible non-alphanumeric, non-separator path characters.
  parseAndCheck(
      "https://zap.org/!$&'()*+,-.:;=@[]^_|~",
      "https://zap.org/!$&'()*+,-.:;=@[]^_|~");
}

KJ_TEST("parse / stringify URL w/o decoding") {
  {
    auto url = parseAndCheck("https://zap.org/foo%2Fbar/baz", nullptr, NO_DECODE);
    KJ_EXPECT(url.path.asPtr() == kj::ArrayPtr<const StringPtr>({"foo%2Fbar", "baz"}));
  }

  {
    // This case would throw an exception without NO_DECODE.
    Url url = parseAndCheck("https://zap.org/R%20%26%20S?%foo=%QQ", nullptr, NO_DECODE);
    KJ_EXPECT(url.scheme == "https");
    KJ_EXPECT(url.host == "zap.org");
    KJ_EXPECT(url.path.asPtr() == kj::ArrayPtr<const StringPtr>({"R%20%26%20S"}));
    KJ_EXPECT(!url.hasTrailingSlash);
    KJ_ASSERT(url.query.size() == 1);
    KJ_EXPECT(url.query[0].name == "%foo");
    KJ_EXPECT(url.query[0].value == "%QQ");
  }
}

KJ_TEST("URL relative paths") {
  parseAndCheck(
      "https://zap.org/foo//bar",
      "https://zap.org/foo/bar");

  parseAndCheck(
      "https://zap.org/foo/./bar",
      "https://zap.org/foo/bar");

  parseAndCheck(
      "https://zap.org/foo/bar//",
      "https://zap.org/foo/bar/");

  parseAndCheck(
      "https://zap.org/foo/bar/.",
      "https://zap.org/foo/bar/");

  parseAndCheck(
      "https://zap.org/foo/baz/../bar",
      "https://zap.org/foo/bar");

  parseAndCheck(
      "https://zap.org/foo/bar/baz/..",
      "https://zap.org/foo/bar/");

  parseAndCheck(
      "https://zap.org/..",
      "https://zap.org/");

  parseAndCheck(
      "https://zap.org/foo/../..",
      "https://zap.org/");
}

KJ_TEST("URL for HTTP request") {
  {
    Url url = Url::parse("https://bob:1234@zap.org/foo/bar?baz=qux#corge");
    KJ_EXPECT(url.toString(Url::REMOTE_HREF) ==
        "https://bob:1234@zap.org/foo/bar?baz=qux#corge");
    KJ_EXPECT(url.toString(Url::HTTP_PROXY_REQUEST) == "https://zap.org/foo/bar?baz=qux");
    KJ_EXPECT(url.toString(Url::HTTP_REQUEST) == "/foo/bar?baz=qux");
  }

  {
    Url url = Url::parse("https://zap.org");
    KJ_EXPECT(url.toString(Url::REMOTE_HREF) == "https://zap.org");
    KJ_EXPECT(url.toString(Url::HTTP_PROXY_REQUEST) == "https://zap.org");
    KJ_EXPECT(url.toString(Url::HTTP_REQUEST) == "/");
  }

  {
    Url url = Url::parse("/foo/bar?baz=qux&corge", Url::HTTP_REQUEST);
    KJ_EXPECT(url.scheme == nullptr);
    KJ_EXPECT(url.host == nullptr);
    KJ_EXPECT(url.path.asPtr() == kj::ArrayPtr<const StringPtr>({"foo", "bar"}));
    KJ_EXPECT(!url.hasTrailingSlash);
    KJ_ASSERT(url.query.size() == 2);
    KJ_EXPECT(url.query[0].name == "baz");
    KJ_EXPECT(url.query[0].value == "qux");
    KJ_EXPECT(url.query[1].name == "corge");
    KJ_EXPECT(url.query[1].value == nullptr);
  }

  {
    Url url = Url::parse("https://zap.org/foo/bar?baz=qux&corge", Url::HTTP_PROXY_REQUEST);
    KJ_EXPECT(url.scheme == "https");
    KJ_EXPECT(url.host == "zap.org");
    KJ_EXPECT(url.path.asPtr() == kj::ArrayPtr<const StringPtr>({"foo", "bar"}));
    KJ_EXPECT(!url.hasTrailingSlash);
    KJ_ASSERT(url.query.size() == 2);
    KJ_EXPECT(url.query[0].name == "baz");
    KJ_EXPECT(url.query[0].value == "qux");
    KJ_EXPECT(url.query[1].name == "corge");
    KJ_EXPECT(url.query[1].value == nullptr);
  }

  {
    // '#' is allowed in path components in the HTTP_REQUEST context.
    Url url = Url::parse("/foo#bar", Url::HTTP_REQUEST);
    KJ_EXPECT(url.toString(Url::HTTP_REQUEST) == "/foo%23bar");
    KJ_EXPECT(url.scheme == nullptr);
    KJ_EXPECT(url.host == nullptr);
    KJ_EXPECT(url.path.asPtr() == kj::ArrayPtr<const StringPtr>{"foo#bar"});
    KJ_EXPECT(!url.hasTrailingSlash);
    KJ_EXPECT(url.query == nullptr);
    KJ_EXPECT(url.fragment == kj::none);
  }

  {
    // '#' is allowed in path components in the HTTP_PROXY_REQUEST context.
    Url url = Url::parse("https://zap.org/foo#bar", Url::HTTP_PROXY_REQUEST);
    KJ_EXPECT(url.toString(Url::HTTP_PROXY_REQUEST) == "https://zap.org/foo%23bar");
    KJ_EXPECT(url.scheme == "https");
    KJ_EXPECT(url.host == "zap.org");
    KJ_EXPECT(url.path.asPtr() == kj::ArrayPtr<const StringPtr>{"foo#bar"});
    KJ_EXPECT(!url.hasTrailingSlash);
    KJ_EXPECT(url.query == nullptr);
    KJ_EXPECT(url.fragment == kj::none);
  }

  {
    // '#' is allowed in query components in the HTTP_REQUEST context.
    Url url = Url::parse("/?foo=bar#123", Url::HTTP_REQUEST);
    KJ_EXPECT(url.toString(Url::HTTP_REQUEST) == "/?foo=bar%23123");
    KJ_EXPECT(url.scheme == nullptr);
    KJ_EXPECT(url.host == nullptr);
    KJ_EXPECT(url.path == nullptr);
    KJ_EXPECT(url.hasTrailingSlash);
    KJ_ASSERT(url.query.size() == 1);
    KJ_EXPECT(url.query[0].name == "foo");
    KJ_EXPECT(url.query[0].value == "bar#123");
    KJ_EXPECT(url.fragment == kj::none);
  }

  {
    // '#' is allowed in query components in the HTTP_PROXY_REQUEST context.
    Url url = Url::parse("https://zap.org/?foo=bar#123", Url::HTTP_PROXY_REQUEST);
    KJ_EXPECT(url.toString(Url::HTTP_PROXY_REQUEST) == "https://zap.org/?foo=bar%23123");
    KJ_EXPECT(url.scheme == "https");
    KJ_EXPECT(url.host == "zap.org");
    KJ_EXPECT(url.path == nullptr);
    KJ_EXPECT(url.hasTrailingSlash);
    KJ_ASSERT(url.query.size() == 1);
    KJ_EXPECT(url.query[0].name == "foo");
    KJ_EXPECT(url.query[0].value == "bar#123");
    KJ_EXPECT(url.fragment == kj::none);
  }
}

KJ_TEST("parse URL failure") {
  KJ_EXPECT(Url::tryParse("ht/tps://zap.org") == kj::none);
  KJ_EXPECT(Url::tryParse("zap.org") == kj::none);
  KJ_EXPECT(Url::tryParse("https:foo") == kj::none);

  // percent-decode errors
  KJ_EXPECT(Url::tryParse("https://zap.org/f%nno") == kj::none);
  KJ_EXPECT(Url::tryParse("https://zap.org/foo?b%nnr=baz") == kj::none);

  // components not valid in context
  KJ_EXPECT(Url::tryParse("https://zap.org/foo", Url::HTTP_REQUEST) == kj::none);
  KJ_EXPECT(Url::tryParse("https://bob:123@zap.org/foo", Url::HTTP_PROXY_REQUEST) == kj::none);
  KJ_EXPECT(Url::tryParse("https://zap.org#/foo", Url::HTTP_PROXY_REQUEST) == kj::none);
}

void parseAndCheckRelative(kj::StringPtr base, kj::StringPtr relative, kj::StringPtr expected,
                           Url::Options options = {}) {
  auto parsed = Url::parse(base, Url::REMOTE_HREF, options).parseRelative(relative);
  KJ_EXPECT(kj::str(parsed) == expected, parsed, expected);
  auto clone = parsed.clone();
  KJ_EXPECT(kj::str(clone) == expected, clone, expected);
}

KJ_TEST("parse relative URL") {
  parseAndCheckRelative("https://zap.org/foo/bar?baz=qux#corge",
                        "#grault",
                        "https://zap.org/foo/bar?baz=qux#grault");
  parseAndCheckRelative("https://zap.org/foo/bar?baz#corge",
                        "#grault",
                        "https://zap.org/foo/bar?baz#grault");
  parseAndCheckRelative("https://zap.org/foo/bar?baz=#corge",
                        "#grault",
                        "https://zap.org/foo/bar?baz=#grault");
  parseAndCheckRelative("https://zap.org/foo/bar?baz=qux#corge",
                        "?grault",
                        "https://zap.org/foo/bar?grault");
  parseAndCheckRelative("https://zap.org/foo/bar?baz=qux#corge",
                        "?grault=",
                        "https://zap.org/foo/bar?grault=");
  parseAndCheckRelative("https://zap.org/foo/bar?baz=qux#corge",
                        "?grault+garply=waldo",
                        "https://zap.org/foo/bar?grault+garply=waldo");
  parseAndCheckRelative("https://zap.org/foo/bar?baz=qux#corge",
                        "grault",
                        "https://zap.org/foo/grault");
  parseAndCheckRelative("https://zap.org/foo/bar?baz=qux#corge",
                        "/grault",
                        "https://zap.org/grault");
  parseAndCheckRelative("https://zap.org/foo/bar?baz=qux#corge",
                        "//grault",
                        "https://grault");
  parseAndCheckRelative("https://zap.org/foo/bar?baz=qux#corge",
                        "//grault/garply",
                        "https://grault/garply");
  parseAndCheckRelative("https://zap.org/foo/bar?baz=qux#corge",
                        "http:/grault",
                        "http://zap.org/grault");
  parseAndCheckRelative("https://zap.org/foo/bar?baz=qux#corge",
                        "/http:/grault",
                        "https://zap.org/http:/grault");
  parseAndCheckRelative("https://zap.org/",
                        "/foo/../bar",
                        "https://zap.org/bar");
}

KJ_TEST("parse relative URL w/o decoding") {
  // This case would throw an exception without NO_DECODE.
  parseAndCheckRelative("https://zap.org/R%20%26%20S?%foo=%QQ",
                        "%ANOTH%ERBAD%URL",
                        "https://zap.org/%ANOTH%ERBAD%URL", NO_DECODE);
}

KJ_TEST("parse relative URL failure") {
  auto base = Url::parse("https://example.com/");
  KJ_EXPECT(base.tryParseRelative("https://[not a host]") == kj::none);
}

}  // namespace
}  // namespace kj
