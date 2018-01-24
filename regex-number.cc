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

#include "regex-number.h"

namespace libtextclassifier2 {

std::unique_ptr<DigitNode> DigitNode::Instance(const Rules& rules) {
  std::unique_ptr<DigitNode> node(new DigitNode());
  if (!node->Init(rules)) {
    return nullptr;
  }
  return node;
}

DigitNode::DigitNode() {}

constexpr const char* kDigits = "DIGITS";

bool DigitNode::Init(const Rules& rules) {
  std::string pattern;
  if (!rules.RuleForName(kDigits, &pattern)) {
    TC_LOG(ERROR) << "failed to load pattern";
    return false;
  }
  return RegexNode<int>::Init(pattern);
}

bool DigitNode::Extract(const std::string& input, Results* result) const {
  UErrorCode status = U_ZERO_ERROR;

  const icu::UnicodeString unicode_context(input.c_str(), input.size(),
                                           "utf-8");
  const std::unique_ptr<icu::RegexMatcher> matcher(
      pattern_->matcher(unicode_context, status));
  if (U_FAILURE(status)) {
    TC_LOG(ERROR) << "failed to compile regex: " << u_errorName(status);
    return false;
  }

  while (matcher->find() && U_SUCCESS(status)) {
    const int start = matcher->start(status);
    if (U_FAILURE(status)) {
      TC_LOG(ERROR) << "failed to bind start: " << u_errorName(status);
      return false;
    }
    const int end = matcher->end(status);
    if (U_FAILURE(status)) {
      TC_LOG(ERROR) << "failed to bind end: " << u_errorName(status);
      return false;
    }
    std::string digit;
    matcher->group(status).toUTF8String(digit);
    if (U_FAILURE(status)) {
      TC_LOG(ERROR) << "failed to bind digit std::string: "
                    << u_errorName(status);
      return false;
    }
    result->push_back(Result(start, end, stoi(digit)));
  }
  return true;
}

constexpr const char* kSignedDigits = "SIGNEDDIGITS";

std::unique_ptr<SignedDigitNode> SignedDigitNode::Instance(const Rules& rules) {
  std::unique_ptr<SignedDigitNode> node(new SignedDigitNode());
  if (!node->Init(rules)) {
    return nullptr;
  }
  return node;
}

SignedDigitNode::SignedDigitNode() {}

bool SignedDigitNode::Init(const Rules& rules) {
  std::string pattern;
  if (!rules.RuleForName(kSignedDigits, &pattern)) {
    TC_LOG(ERROR) << "failed to load pattern";
    return false;
  }
  return RegexNode<int>::Init(pattern);
}

bool SignedDigitNode::Extract(const std::string& input, Results* result) const {
  UErrorCode status = U_ZERO_ERROR;

  const icu::UnicodeString unicode_context(input.c_str(), input.size(),
                                           "utf-8");
  const std::unique_ptr<icu::RegexMatcher> matcher(
      pattern_->matcher(unicode_context, status));
  if (U_FAILURE(status)) {
    TC_LOG(ERROR) << "failed to compile regex: " << u_errorName(status);
    return false;
  }

  while (matcher->find() && U_SUCCESS(status)) {
    const int start = matcher->start(status);
    if (U_FAILURE(status)) {
      TC_LOG(ERROR) << "failed to bind start: " << u_errorName(status);
      return false;
    }
    const int end = matcher->end(status);
    if (U_FAILURE(status)) {
      TC_LOG(ERROR) << "failed to bind end: " << u_errorName(status);
      return false;
    }
    std::string digit;
    matcher->group(status).toUTF8String(digit);
    if (U_FAILURE(status)) {
      TC_LOG(ERROR) << "failed to bind digit std::string: "
                    << u_errorName(status);
      return false;
    }
    result->push_back(Result(start, end, stoi(digit)));
  }
  return true;
}

constexpr const char* kZero = "ZERO";
constexpr const char* kOne = "ONE";
constexpr const char* kTwo = "TWO";
constexpr const char* kThree = "THREE";
constexpr const char* kFour = "FOUR";
constexpr const char* kFive = "FIVE";
constexpr const char* kSix = "SIX";
constexpr const char* kSeven = "SEVEN";
constexpr const char* kEight = "EIGHT";
constexpr const char* kNine = "NINE";
constexpr const char* kTen = "TEN";
constexpr const char* kEleven = "ELEVEN";
constexpr const char* kTwelve = "TWELVE";
constexpr const char* kThirteen = "THIRTEEN";
constexpr const char* kFourteen = "FOURTEEN";
constexpr const char* kFifteen = "FIFTEEN";
constexpr const char* kSixteen = "SIXTEEN";
constexpr const char* kSeventeen = "SEVENTEEN";
constexpr const char* kEighteen = "EIGHTEEN";
constexpr const char* kNineteen = "NINETEEN";
constexpr const char* kTwenty = "TWENTY";
constexpr const char* kThirty = "THIRTY";
constexpr const char* kForty = "FORTY";
constexpr const char* kFifty = "FIFTY";
constexpr const char* kSixty = "SIXTY";
constexpr const char* kSeventy = "SEVENTY";
constexpr const char* kEighty = "EIGHTY";
constexpr const char* kNinety = "NINETY";
constexpr const char* kHundred = "HUNDRED";
constexpr const char* kThousand = "THOUSAND";

std::unique_ptr<NumberNode> NumberNode::Instance(const Rules& rules) {
  const std::vector<std::pair<std::string, int>> name_values = {
      {kZero, 0},      {kOne, 1},         {kTwo, 2},       {kThree, 3},
      {kFour, 4},      {kFive, 5},        {kSix, 6},       {kSeven, 7},
      {kEight, 8},     {kNine, 9},        {kTen, 10},      {kEleven, 11},
      {kTwelve, 12},   {kThirteen, 13},   {kFourteen, 14}, {kFifteen, 15},
      {kSixteen, 16},  {kSeventeen, 17},  {kEighteen, 18}, {kNineteen, 19},
      {kTwenty, 20},   {kThirty, 30},     {kForty, 40},    {kFifty, 50},
      {kSixty, 60},    {kSeventy, 70},    {kEighty, 80},   {kNinety, 90},
      {kHundred, 100}, {kThousand, 1000},
  };
  std::vector<std::unique_ptr<const RegexNode<int>>> alternatives;
  if (!BuildMappings<int>(rules, name_values, &alternatives)) {
    return nullptr;
  }
  std::unique_ptr<NumberNode> node(new NumberNode(std::move(alternatives)));
  if (!node->Init()) {
    return nullptr;
  }
  return node;
}  // namespace libtextclassifier2

bool NumberNode::Init() { return OrRegexNode<int>::Init(); }

NumberNode::NumberNode(
    std::vector<std::unique_ptr<const RegexNode<int>>> alternatives)
    : OrRegexNode<int>(std::move(alternatives)) {}

bool NumberNode::Extract(const std::string& input, Results* result) const {
  UErrorCode status = U_ZERO_ERROR;

  const icu::UnicodeString unicode_context(input.c_str(), input.size(),
                                           "utf-8");
  const std::unique_ptr<icu::RegexMatcher> matcher(
      RegexNode<int>::pattern_->matcher(unicode_context, status));

  OrRegexNode<int>::Results parts;
  int start = 0;
  int end = 0;
  while (matcher->find() && U_SUCCESS(status)) {
    std::string group;
    matcher->group(0, status).toUTF8String(group);
    int span_start = matcher->start(status);
    if (U_FAILURE(status)) {
      TC_LOG(ERROR) << "failed to demarshall start " << u_errorName(status);
      return false;
    }
    int span_end = matcher->end(status);
    if (U_FAILURE(status)) {
      TC_LOG(ERROR) << "failed to demarshall end " << u_errorName(status);
    }
    if (span_start < start) {
      start = span_start;
    }
    if (span_end < end) {
      end = span_end;
    }

    for (auto& child : alternatives_) {
      if (child->Matches(group)) {
        OrRegexNode<int>::Results group_results;
        if (!child->Extract(group, &group_results)) {
          return false;
        }
        for (OrRegexNode<int>::Result span : group_results) {
          parts.push_back(span);
        }
      }
    }
  }
  int sum = 0;
  int running_value = -1;
  // Simple math to make sure we handle written numerical modifiers correctly
  // so that :="fifty one  thousand and one" maps to 51001 and not 50 1 1000 1.
  for (OrRegexNode<int>::Result part : parts) {
    if (running_value >= 0) {
      if (running_value > part.Data()) {
        sum += running_value;
        running_value = part.Data();
      } else {
        running_value *= part.Data();
      }
    } else {
      running_value = part.Data();
    }
  }
  sum += running_value;
  result->push_back(Result(start, end, sum));
  return true;
}
}  // namespace libtextclassifier2
