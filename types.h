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

#ifndef KNOWLEDGE_CEREBRA_SENSE_TEXT_CLASSIFIER_LIB2_TYPES_H_
#define KNOWLEDGE_CEREBRA_SENSE_TEXT_CLASSIFIER_LIB2_TYPES_H_

#include <algorithm>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "util/base/logging.h"

namespace libtextclassifier2 {

constexpr int kInvalidIndex = -1;

// Index for a 0-based array of tokens.
using TokenIndex = int;

// Index for a 0-based array of codepoints.
using CodepointIndex = int;

// Marks a span in a sequence of codepoints. The first element is the index of
// the first codepoint of the span, and the second element is the index of the
// codepoint one past the end of the span.
// TODO(b/71982294): Make it a struct.
using CodepointSpan = std::pair<CodepointIndex, CodepointIndex>;

// Marks a span in a sequence of tokens. The first element is the index of the
// first token in the span, and the second element is the index of the token one
// past the end of the span.
// TODO(b/71982294): Make it a struct.
using TokenSpan = std::pair<TokenIndex, TokenIndex>;

// Returns the size of the token span. Assumes that the span is valid.
inline int TokenSpanSize(const TokenSpan& token_span) {
  return token_span.second - token_span.first;
}

// Returns a token span consisting of one token.
inline TokenSpan SingleTokenSpan(int token_index) {
  return {token_index, token_index + 1};
}

// Returns an intersection of two token spans. Assumes that both spans are valid
// and overlapping.
inline TokenSpan IntersectTokenSpans(const TokenSpan& token_span1,
                                     const TokenSpan& token_span2) {
  return {std::max(token_span1.first, token_span2.first),
          std::min(token_span1.second, token_span2.second)};
}

// Returns and expanded token span by adding a certain number of tokens on its
// left and on its right.
inline TokenSpan ExpandTokenSpan(const TokenSpan& token_span,
                                 int num_tokens_left, int num_tokens_right) {
  return {token_span.first - num_tokens_left,
          token_span.second + num_tokens_right};
}

// Token holds a token, its position in the original string and whether it was
// part of the input span.
struct Token {
  std::string value;
  CodepointIndex start;
  CodepointIndex end;

  // Whether the token is a padding token.
  bool is_padding;

  // Default constructor constructs the padding-token.
  Token()
      : value(""), start(kInvalidIndex), end(kInvalidIndex), is_padding(true) {}

  Token(const std::string& arg_value, CodepointIndex arg_start,
        CodepointIndex arg_end)
      : value(arg_value), start(arg_start), end(arg_end), is_padding(false) {}

  bool operator==(const Token& other) const {
    return value == other.value && start == other.start && end == other.end &&
           is_padding == other.is_padding;
  }

  bool IsContainedInSpan(CodepointSpan span) const {
    return start >= span.first && end <= span.second;
  }
};

// Pretty-printing function for Token.
inline logging::LoggingStringStream& operator<<(
    logging::LoggingStringStream& stream, const Token& token) {
  if (!token.is_padding) {
    return stream << "Token(\"" << token.value << "\", " << token.start << ", "
                  << token.end << ")";
  } else {
    return stream << "Token()";
  }
}

// Represents a result of Annotate call.
struct AnnotatedSpan {
  // Unicode codepoint indices in the input string.
  CodepointSpan span = {kInvalidIndex, kInvalidIndex};

  // Classification result for the span.
  std::vector<std::pair<std::string, float>> classification;
};

// Pretty-printing function for AnnotatedSpan.
inline logging::LoggingStringStream& operator<<(
    logging::LoggingStringStream& stream, const AnnotatedSpan& span) {
  std::string best_class;
  float best_score = -1;
  if (!span.classification.empty()) {
    best_class = span.classification[0].first;
    best_score = span.classification[0].second;
  }
  return stream << "Span(" << span.span.first << ", " << span.span.second
                << ", " << best_class << ", " << best_score << ")";
}

// StringPiece analogue for std::vector<T>.
template <class T>
class VectorSpan {
 public:
  VectorSpan() : begin_(), end_() {}
  VectorSpan(const std::vector<T>& v)  // NOLINT(runtime/explicit)
      : begin_(v.begin()), end_(v.end()) {}
  VectorSpan(typename std::vector<T>::const_iterator begin,
             typename std::vector<T>::const_iterator end)
      : begin_(begin), end_(end) {}

  const T& operator[](typename std::vector<T>::size_type i) const {
    return *(begin_ + i);
  }

  int size() const { return end_ - begin_; }
  typename std::vector<T>::const_iterator begin() const { return begin_; }
  typename std::vector<T>::const_iterator end() const { return end_; }
  const float* data() const { return &(*begin_); }

 private:
  typename std::vector<T>::const_iterator begin_;
  typename std::vector<T>::const_iterator end_;
};

}  // namespace libtextclassifier2

#endif  // KNOWLEDGE_CEREBRA_SENSE_TEXT_CLASSIFIER_LIB2_TYPES_H_
