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

// UniLib implementation with the help of ICU. UniLib is basically a wrapper
// around the ICU functionality.

#ifndef KNOWLEDGE_CEREBRA_SENSE_TEXT_CLASSIFIER_LIB2_UTIL_UTF8_UNILIB_ICU_H_
#define KNOWLEDGE_CEREBRA_SENSE_TEXT_CLASSIFIER_LIB2_UTIL_UTF8_UNILIB_ICU_H_

#include <memory>
#include <string>

#include "util/base/integral_types.h"
#include "unicode/brkiter.h"
#include "unicode/errorcode.h"
#include "unicode/regex.h"
#include "unicode/uchar.h"

namespace libtextclassifier2 {

class UniLib {
 public:
  bool IsOpeningBracket(char32 codepoint) const;
  bool IsClosingBracket(char32 codepoint) const;
  bool IsWhitespace(char32 codepoint) const;
  bool IsDigit(char32 codepoint) const;
  bool IsUpper(char32 codepoint) const;

  char32 ToLower(char32 codepoint) const;
  char32 GetPairedBracket(char32 codepoint) const;

  class RegexPattern {
   public:
    // Returns true if the whole input matches with the regex.
    bool Matches(const std::string& text);

   protected:
    friend class UniLib;
    explicit RegexPattern(std::unique_ptr<icu::RegexPattern> pattern)
        : pattern_(std::move(pattern)) {}

   private:
    std::unique_ptr<icu::RegexPattern> pattern_;
  };

  class BreakIterator {
   public:
    int Next();

    static constexpr int kDone = -1;

   protected:
    friend class UniLib;
    explicit BreakIterator(const std::string& text);

   private:
    std::unique_ptr<icu::BreakIterator> break_iterator_;
  };

  std::unique_ptr<RegexPattern> CreateRegexPattern(
      const std::string& regex) const;
  std::unique_ptr<BreakIterator> CreateBreakIterator(
      const std::string& text) const;
};

}  // namespace libtextclassifier2

#endif  // KNOWLEDGE_CEREBRA_SENSE_TEXT_CLASSIFIER_LIB2_UTIL_UTF8_UNILIB_ICU_H_
