// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/css_uri_value.h"

#include "testing/gtest/include/gtest/gtest.h"

#include "third_party/blink/renderer/platform/weborigin/kurl.h"

namespace blink {
namespace {

TEST(CSSURIValueTest, ComputedCSSValue) {
  cssvalue::CSSURIValue* rel = MakeGarbageCollected<cssvalue::CSSURIValue>(
      AtomicString("a"), KURL("http://foo.com/a"));
  cssvalue::CSSURIValue* abs =
      rel->ComputedCSSValue(KURL("http://bar.com"), WTF::TextEncoding());
  EXPECT_EQ("url(\"http://bar.com/a\")", abs->CssText());
}

TEST(CSSURIValueTest, AlreadyComputedCSSValue) {
  cssvalue::CSSURIValue* rel = MakeGarbageCollected<cssvalue::CSSURIValue>(
      AtomicString("http://baz.com/a"), KURL("http://baz.com/a"));
  cssvalue::CSSURIValue* abs =
      rel->ComputedCSSValue(KURL("http://bar.com"), WTF::TextEncoding());
  EXPECT_EQ("url(\"http://baz.com/a\")", abs->CssText());
}

TEST(CSSURIValueTest, LocalComputedCSSValue) {
  cssvalue::CSSURIValue* rel = MakeGarbageCollected<cssvalue::CSSURIValue>(
      AtomicString("#a"), KURL("http://baz.com/a"));
  cssvalue::CSSURIValue* abs =
      rel->ComputedCSSValue(KURL("http://bar.com"), WTF::TextEncoding());
  EXPECT_EQ("url(\"#a\")", abs->CssText());
}

TEST(CSSURIValueTest, EmptyComputedCSSValue) {
  cssvalue::CSSURIValue* rel =
      MakeGarbageCollected<cssvalue::CSSURIValue>(g_empty_atom, KURL());
  cssvalue::CSSURIValue* abs =
      rel->ComputedCSSValue(KURL("http://bar.com"), WTF::TextEncoding());
  EXPECT_EQ("url(\"\")", abs->CssText());
}

}  // namespace
}  // namespace blink
