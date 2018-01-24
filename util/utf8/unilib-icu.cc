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

#include "util/utf8/unilib-icu.h"

#include "util/base/logging.h"

namespace libtextclassifier2 {

bool UniLib::IsOpeningBracket(char32 codepoint) const {
  return u_getIntPropertyValue(codepoint, UCHAR_BIDI_PAIRED_BRACKET_TYPE) ==
         U_BPT_OPEN;
}

bool UniLib::IsClosingBracket(char32 codepoint) const {
  return u_getIntPropertyValue(codepoint, UCHAR_BIDI_PAIRED_BRACKET_TYPE) ==
         U_BPT_CLOSE;
}

bool UniLib::IsWhitespace(char32 codepoint) const {
  return u_isWhitespace(codepoint);
}

bool UniLib::IsDigit(char32 codepoint) const { return u_isdigit(codepoint); }

bool UniLib::IsUpper(char32 codepoint) const { return u_isupper(codepoint); }

char32 UniLib::ToLower(char32 codepoint) const { return u_tolower(codepoint); }

char32 UniLib::GetPairedBracket(char32 codepoint) const {
  return u_getBidiPairedBracket(codepoint);
}

bool UniLib::RegexPattern::Matches(const std::string& text) {
  const icu::UnicodeString unicode_text(text.c_str(), text.size(), "utf-8");
  UErrorCode status;
  status = U_ZERO_ERROR;
  std::unique_ptr<icu::RegexMatcher> matcher(
      pattern_->matcher(unicode_text, status));
  if (U_FAILURE(status) || !matcher) {
    return false;
  }

  status = U_ZERO_ERROR;
  const bool result = matcher->matches(/*startIndex=*/0, status);
  if (U_FAILURE(status)) {
    return false;
  }

  return result;
}

constexpr int UniLib::BreakIterator::kDone;

UniLib::BreakIterator::BreakIterator(const std::string& text) {
  icu::ErrorCode status;
  break_iterator_.reset(
      icu::BreakIterator::createWordInstance(icu::Locale("en"), status));
  if (!status.isSuccess()) {
    break_iterator_.reset();
    return;
  }

  const icu::UnicodeString unicode_text = icu::UnicodeString::fromUTF8(text);
  break_iterator_->setText(unicode_text);
}

int UniLib::BreakIterator::Next() {
  const int result = break_iterator_->next();
  if (result == icu::BreakIterator::DONE) {
    return BreakIterator::kDone;
  } else {
    return result;
  }
}

std::unique_ptr<UniLib::RegexPattern> UniLib::CreateRegexPattern(
    const std::string& regex) const {
  UErrorCode status = U_ZERO_ERROR;
  std::unique_ptr<icu::RegexPattern> pattern(icu::RegexPattern::compile(
      icu::UnicodeString(regex.c_str(), regex.size(), "utf-8"), /*flags=*/0,
      status));
  if (U_FAILURE(status) || !pattern) {
    return nullptr;
  }
  return std::unique_ptr<UniLib::RegexPattern>(
      new UniLib::RegexPattern(std::move(pattern)));
}

std::unique_ptr<UniLib::BreakIterator> UniLib::CreateBreakIterator(
    const std::string& text) const {
  return std::unique_ptr<UniLib::BreakIterator>(
      new UniLib::BreakIterator(text));
}

}  // namespace libtextclassifier2
