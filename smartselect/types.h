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

#ifndef LIBTEXTCLASSIFIER_SMARTSELECT_TYPES_H_
#define LIBTEXTCLASSIFIER_SMARTSELECT_TYPES_H_

#include <ostream>
#include <string>
#include <utility>

namespace libtextclassifier {

constexpr int kInvalidIndex = -1;

// Index for a 0-based array of tokens.
using TokenIndex = int;

// Index for a 0-based array of codepoints.
using CodepointIndex = int;

// Marks a span in a sequence of codepoints. The first element is the index of
// the first codepoint of the span, and the second element is the index of the
// codepoint one past the end of the span.
using CodepointSpan = std::pair<CodepointIndex, CodepointIndex>;

// Marks a span in a sequence of tokens. The first element is the index of the
// first token in the span, and the second element is the index of the token one
// past the end of the span.
using TokenSpan = std::pair<TokenIndex, TokenIndex>;

// Token holds a token, its position in the original string and whether it was
// part of the input span.
struct Token {
  std::string value;
  CodepointIndex start;
  CodepointIndex end;

  // Whether the token was in the input span.
  bool is_in_span;

  // Whether the token is a padding token.
  bool is_padding;

  // Default constructor constructs the padding-token.
  Token()
      : value(""),
        start(kInvalidIndex),
        end(kInvalidIndex),
        is_in_span(false),
        is_padding(true) {}

  Token(const std::string& arg_value, CodepointIndex arg_start,
        CodepointIndex arg_end)
      : Token(arg_value, arg_start, arg_end, false) {}

  Token(const std::string& arg_value, CodepointIndex arg_start,
        CodepointIndex arg_end, bool is_in_span)
      : value(arg_value),
        start(arg_start),
        end(arg_end),
        is_in_span(is_in_span),
        is_padding(false) {}

  bool operator==(const Token& other) const {
    return value == other.value && start == other.start && end == other.end &&
           is_in_span == other.is_in_span && is_padding == other.is_padding;
  }
};

// Pretty-printing function for Token.
inline std::ostream& operator<<(std::ostream& os, const Token& token) {
  return os << "Token(\"" << token.value << "\", " << token.start << ", "
            << token.end << ", is_in_span=" << token.is_in_span
            << ", is_padding=" << token.is_padding << ")";
}

// Represents a selection.
struct SelectionWithContext {
  SelectionWithContext()
      : context(""),
        selection_start(-1),
        selection_end(-1),
        click_start(-1),
        click_end(-1) {}

  // UTF8 encoded context.
  std::string context;

  // Codepoint index to the context where selection starts.
  CodepointIndex selection_start;

  // Codepoint index to the context one past where selection ends.
  CodepointIndex selection_end;

  // Codepoint index to the context where click starts.
  CodepointIndex click_start;

  // Codepoint index to the context one past where click ends.
  CodepointIndex click_end;

  // Type of the selection.
  std::string collection;

  CodepointSpan GetClickSpan() const { return {click_start, click_end}; }

  CodepointSpan GetSelectionSpan() const {
    return {selection_start, selection_end};
  }
};

}  // namespace libtextclassifier

#endif  // LIBTEXTCLASSIFIER_TYPES_H_
