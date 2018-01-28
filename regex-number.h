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

#ifndef KNOWLEDGE_CEREBRA_SENSE_TEXT_CLASSIFIER_LIB2_REGEX_NUMBER_H_
#define KNOWLEDGE_CEREBRA_SENSE_TEXT_CLASSIFIER_LIB2_REGEX_NUMBER_H_

#include "regex-base.h"

namespace libtextclassifier2 {

extern const char* const kZero;
extern const char* const kOne;
extern const char* const kTwo;
extern const char* const kThree;
extern const char* const kFour;
extern const char* const kFive;
extern const char* const kSix;
extern const char* const kSeven;
extern const char* const kEight;
extern const char* const kNine;
extern const char* const kTen;
extern const char* const kEleven;
extern const char* const kTwelve;
extern const char* const kThirteen;
extern const char* const kFourteen;
extern const char* const kFifteen;
extern const char* const kSixteen;
extern const char* const kSeventeen;
extern const char* const kEighteen;
extern const char* const kNineteen;
extern const char* const kTwenty;
extern const char* const kThirty;
extern const char* const kForty;
extern const char* const kFifty;
extern const char* const kSixty;
extern const char* const kSeventy;
extern const char* const kEighty;
extern const char* const kNinety;
extern const char* const kHundred;
extern const char* const kThousand;

extern const char* const kDigits;
extern const char* const kSignedDigits;

// Class that encapsulates a unsigned integer matching and extract values of
// SpanResults of type int.
// Thread-safe.
class DigitNode : public RegexNode<int> {
 public:
  typedef typename RegexNode<int>::Result Result;
  typedef typename RegexNode<int>::Results Results;

  // Factory method for yielding a pointer to a DigitNode implementation.
  static std::unique_ptr<DigitNode> Instance(const Rules& rules);
  bool Extract(const std::string& input, Results* result) const override;

 protected:
  DigitNode();
  bool Init(const Rules& rules);
};

// Class that encapsulates a signed integer matching and extract values of
// SpanResults of type int.
// Thread-safe.
class SignedDigitNode : public RegexNode<int> {
 public:
  typedef typename RegexNode<int>::Result Result;
  typedef typename RegexNode<int>::Results Results;

  // Factory method for yielding a pointer to a DigitNode implementation.
  static std::unique_ptr<SignedDigitNode> Instance(const Rules& rules);
  bool Extract(const std::string& input, Results* result) const override;

 protected:
  SignedDigitNode();
  bool Init(const Rules& rules);
};

// Class that encapsulates a simple natural language integer matching and
// extract values of SpanResults of type int.
// Thread-safe.
class NumberNode : public OrRegexNode<int> {
 public:
  typedef typename OrRegexNode<int>::Result Result;
  typedef typename OrRegexNode<int>::Results Results;

  static std::unique_ptr<NumberNode> Instance(const Rules& rules);
  bool Extract(const std::string& input, Results* result) const override;

 protected:
  explicit NumberNode(
      std::vector<std::unique_ptr<const RegexNode<int>>> alternatives);
  bool Init();
};

}  // namespace libtextclassifier2
#endif  // KNOWLEDGE_CEREBRA_SENSE_TEXT_CLASSIFIER_LIB2_REGEX_NUMBER_H_
