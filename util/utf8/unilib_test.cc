/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "util/utf8/unilib.h"
#include "util/utf8/unicodetext.h"

#include "util/base/logging.h"

#include "gtest/gtest.h"

namespace libtextclassifier2 {
namespace {

TEST(UniLibTest, CharacterClassesAscii) {
  CREATE_UNILIB_FOR_TESTING
  EXPECT_TRUE(unilib.IsOpeningBracket('('));
  EXPECT_TRUE(unilib.IsClosingBracket(')'));
  EXPECT_FALSE(unilib.IsWhitespace(')'));
  EXPECT_TRUE(unilib.IsWhitespace(' '));
  EXPECT_FALSE(unilib.IsDigit(')'));
  EXPECT_TRUE(unilib.IsDigit('0'));
  EXPECT_TRUE(unilib.IsDigit('9'));
  EXPECT_FALSE(unilib.IsUpper(')'));
  EXPECT_TRUE(unilib.IsUpper('A'));
  EXPECT_TRUE(unilib.IsUpper('Z'));
  EXPECT_EQ(unilib.ToLower('A'), 'a');
  EXPECT_EQ(unilib.ToLower('Z'), 'z');
  EXPECT_EQ(unilib.ToLower(')'), ')');
  EXPECT_EQ(unilib.GetPairedBracket(')'), '(');
  EXPECT_EQ(unilib.GetPairedBracket('}'), '{');
}

#ifndef LIBTEXTCLASSIFIER_UNILIB_DUMMY
TEST(UniLibTest, CharacterClassesUnicode) {
  CREATE_UNILIB_FOR_TESTING
  EXPECT_TRUE(unilib.IsOpeningBracket(0x0F3C));  // TIBET ANG KHANG GYON
  EXPECT_TRUE(unilib.IsClosingBracket(0x0F3D));  // TIBET ANG KHANG GYAS
  EXPECT_FALSE(unilib.IsWhitespace(0x23F0));     // ALARM CLOCK
  EXPECT_TRUE(unilib.IsWhitespace(0x2003));      // EM SPACE
  EXPECT_FALSE(unilib.IsDigit(0xA619));          // VAI SYMBOL JONG
  EXPECT_TRUE(unilib.IsDigit(0xA620));           // VAI DIGIT ZERO
  EXPECT_TRUE(unilib.IsDigit(0xA629));           // VAI DIGIT NINE
  EXPECT_FALSE(unilib.IsDigit(0xA62A));          // VAI SYLLABLE NDOLE MA
  EXPECT_FALSE(unilib.IsUpper(0x0211));          // SMALL R WITH DOUBLE GRAVE
  EXPECT_TRUE(unilib.IsUpper(0x0212));           // CAPITAL R WITH DOUBLE GRAVE
  EXPECT_TRUE(unilib.IsUpper(0x0391));           // GREEK CAPITAL ALPHA
  EXPECT_TRUE(unilib.IsUpper(0x03AB));           // GREEK CAPITAL UPSILON W DIAL
  EXPECT_FALSE(unilib.IsUpper(0x03AC));          // GREEK SMALL ALPHA WITH TONOS
  EXPECT_EQ(unilib.ToLower(0x0391), 0x03B1);     // GREEK ALPHA
  EXPECT_EQ(unilib.ToLower(0x03AB), 0x03CB);     // GREEK UPSILON WITH DIALYTIKA
  EXPECT_EQ(unilib.ToLower(0x03C0), 0x03C0);     // GREEK SMALL PI

  EXPECT_EQ(unilib.GetPairedBracket(0x0F3C), 0x0F3D);
  EXPECT_EQ(unilib.GetPairedBracket(0x0F3D), 0x0F3C);
}
#endif  // ndef LIBTEXTCLASSIFIER_UNILIB_DUMMY

TEST(UniLibTest, Regex) {
  CREATE_UNILIB_FOR_TESTING
  const UnicodeText regex_pattern =
      UTF8ToUnicodeText("[0-9]+", /*do_copy=*/true);
  std::unique_ptr<UniLib::RegexPattern> pattern =
      unilib.CreateRegexPattern(regex_pattern);
  const UnicodeText input = UTF8ToUnicodeText("hello 0123", /*do_copy=*/false);
  int status;
  std::unique_ptr<UniLib::RegexMatcher> matcher = pattern->Matcher(input);
  TC_LOG(INFO) << matcher->Matches(&status);
  TC_LOG(INFO) << matcher->Find(&status);
  TC_LOG(INFO) << matcher->Start(0, &status);
  TC_LOG(INFO) << matcher->End(0, &status);
  TC_LOG(INFO) << matcher->Group(0, &status).size_codepoints();
}

TEST(UniLibTest, BreakIterator) {
  CREATE_UNILIB_FOR_TESTING
  const UnicodeText text = UTF8ToUnicodeText("some text", /*do_copy=*/false);
  std::unique_ptr<UniLib::BreakIterator> iterator =
      unilib.CreateBreakIterator(text);
  TC_LOG(INFO) << iterator->Next();
  TC_LOG(INFO) << UniLib::BreakIterator::kDone;
}

TEST(UniLibTest, IntegerParse) {
#if !defined LIBTEXTCLASSIFIER_UNILIB_JAVAICU
  CREATE_UNILIB_FOR_TESTING;
  int result;
  EXPECT_TRUE(
      unilib.ParseInt32(UTF8ToUnicodeText("123", /*do_copy=*/false), &result));
  EXPECT_EQ(result, 123);
#endif
}

TEST(UniLibTest, IntegerParseFullWidth) {
#if defined LIBTEXTCLASSIFIER_UNILIB_ICU
  CREATE_UNILIB_FOR_TESTING;
  int result;
  // The input string here is full width
  EXPECT_TRUE(unilib.ParseInt32(UTF8ToUnicodeText("１２３", /*do_copy=*/false),
                                &result));
  EXPECT_EQ(result, 123);
#endif
}

TEST(UniLibTest, IntegerParseFullWidthWithAlpha) {
#if defined LIBTEXTCLASSIFIER_UNILIB_ICU
  CREATE_UNILIB_FOR_TESTING;
  int result;
  // The input string here is full width
  EXPECT_FALSE(unilib.ParseInt32(UTF8ToUnicodeText("１a３", /*do_copy=*/false),
                                 &result));
#endif
}
}  // namespace
}  // namespace libtextclassifier2
