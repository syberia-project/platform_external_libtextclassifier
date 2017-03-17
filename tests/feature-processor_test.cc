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

#include "smartselect/feature-processor.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace libtextclassifier {
namespace {

using testing::ElementsAreArray;

TEST(FeatureProcessorTest, SplitTokensOnSelectionBoundariesMiddle) {
  std::vector<Token> tokens{Token("Hělló", 0, 5, false),
                            Token("fěěbař@google.com", 6, 23, false),
                            Token("heře!", 24, 29, false)};

  internal::SplitTokensOnSelectionBoundaries({9, 12}, &tokens);

  // clang-format off
  EXPECT_THAT(tokens, ElementsAreArray(
                          {Token("Hělló", 0, 5, false),
                           Token("fěě", 6, 9, false),
                           Token("bař", 9, 12, false),
                           Token("@google.com", 12, 23, false),
                           Token("heře!", 24, 29, false)}));
  // clang-format on
}

TEST(FeatureProcessorTest, SplitTokensOnSelectionBoundariesBegin) {
  std::vector<Token> tokens{Token("Hělló", 0, 5, false),
                            Token("fěěbař@google.com", 6, 23, false),
                            Token("heře!", 24, 29, false)};

  internal::SplitTokensOnSelectionBoundaries({6, 12}, &tokens);

  // clang-format off
  EXPECT_THAT(tokens, ElementsAreArray(
                          {Token("Hělló", 0, 5, false),
                           Token("fěěbař", 6, 12, false),
                           Token("@google.com", 12, 23, false),
                           Token("heře!", 24, 29, false)}));
  // clang-format on
}

TEST(FeatureProcessorTest, SplitTokensOnSelectionBoundariesEnd) {
  std::vector<Token> tokens{Token("Hělló", 0, 5, false),
                            Token("fěěbař@google.com", 6, 23, false),
                            Token("heře!", 24, 29, false)};

  internal::SplitTokensOnSelectionBoundaries({9, 23}, &tokens);

  // clang-format off
  EXPECT_THAT(tokens, ElementsAreArray(
                          {Token("Hělló", 0, 5, false),
                           Token("fěě", 6, 9, false),
                           Token("bař@google.com", 9, 23, false),
                           Token("heře!", 24, 29, false)}));
  // clang-format on
}

TEST(FeatureProcessorTest, SplitTokensOnSelectionBoundariesWhole) {
  std::vector<Token> tokens{Token("Hělló", 0, 5, false),
                            Token("fěěbař@google.com", 6, 23, false),
                            Token("heře!", 24, 29, false)};

  internal::SplitTokensOnSelectionBoundaries({6, 23}, &tokens);

  // clang-format off
  EXPECT_THAT(tokens, ElementsAreArray(
                          {Token("Hělló", 0, 5, false),
                           Token("fěěbař@google.com", 6, 23, false),
                           Token("heře!", 24, 29, false)}));
  // clang-format on
}

TEST(FeatureProcessorTest, SplitTokensOnSelectionBoundariesCrossToken) {
  std::vector<Token> tokens{Token("Hělló", 0, 5, false),
                            Token("fěěbař@google.com", 6, 23, false),
                            Token("heře!", 24, 29, false)};

  internal::SplitTokensOnSelectionBoundaries({2, 9}, &tokens);

  // clang-format off
  EXPECT_THAT(tokens, ElementsAreArray(
                          {Token("Hě", 0, 2, false),
                           Token("lló", 2, 5, false),
                           Token("fěě", 6, 9, false),
                           Token("bař@google.com", 9, 23, false),
                           Token("heře!", 24, 29, false)}));
  // clang-format on
}

TEST(FeatureProcessorTest, KeepLineWithClickFirst) {
  const std::string context = "Fiřst Lině\nSěcond Lině\nThiřd Lině";
  const CodepointSpan span = {0, 5};
  // clang-format off
  std::vector<Token> tokens = {Token("Fiřst", 0, 5),
                               Token("Lině", 6, 10),
                               Token("Sěcond", 11, 17),
                               Token("Lině", 18, 22),
                               Token("Thiřd", 23, 28),
                               Token("Lině", 29, 33)};
  // clang-format on

  // Keeps the first line.
  internal::StripTokensFromOtherLines(context, span, &tokens);
  EXPECT_THAT(tokens,
              ElementsAreArray({Token("Fiřst", 0, 5), Token("Lině", 6, 10)}));
}

TEST(FeatureProcessorTest, KeepLineWithClickSecond) {
  const std::string context = "Fiřst Lině\nSěcond Lině\nThiřd Lině";
  const CodepointSpan span = {18, 22};
  // clang-format off
  std::vector<Token> tokens = {Token("Fiřst", 0, 5),
                               Token("Lině", 6, 10),
                               Token("Sěcond", 11, 17),
                               Token("Lině", 18, 22),
                               Token("Thiřd", 23, 28),
                               Token("Lině", 29, 33)};
  // clang-format on

  // Keeps the first line.
  internal::StripTokensFromOtherLines(context, span, &tokens);
  EXPECT_THAT(tokens, ElementsAreArray(
                          {Token("Sěcond", 11, 17), Token("Lině", 18, 22)}));
}

TEST(FeatureProcessorTest, KeepLineWithClickThird) {
  const std::string context = "Fiřst Lině\nSěcond Lině\nThiřd Lině";
  const CodepointSpan span = {24, 33};
  // clang-format off
  std::vector<Token> tokens = {Token("Fiřst", 0, 5),
                               Token("Lině", 6, 10),
                               Token("Sěcond", 11, 17),
                               Token("Lině", 18, 22),
                               Token("Thiřd", 23, 28),
                               Token("Lině", 29, 33)};
  // clang-format on

  // Keeps the first line.
  internal::StripTokensFromOtherLines(context, span, &tokens);
  EXPECT_THAT(tokens, ElementsAreArray(
                          {Token("Thiřd", 23, 28), Token("Lině", 29, 33)}));
}

TEST(FeatureProcessorTest, KeepLineWithClickSecondWithPipe) {
  const std::string context = "Fiřst Lině|Sěcond Lině\nThiřd Lině";
  const CodepointSpan span = {18, 22};
  // clang-format off
  std::vector<Token> tokens = {Token("Fiřst", 0, 5),
                               Token("Lině", 6, 10),
                               Token("Sěcond", 11, 17),
                               Token("Lině", 18, 22),
                               Token("Thiřd", 23, 28),
                               Token("Lině", 29, 33)};
  // clang-format on

  // Keeps the first line.
  internal::StripTokensFromOtherLines(context, span, &tokens);
  EXPECT_THAT(tokens, ElementsAreArray(
                          {Token("Sěcond", 11, 17), Token("Lině", 18, 22)}));
}

TEST(FeatureProcessorTest, KeepLineWithCrosslineClick) {
  const std::string context = "Fiřst Lině\nSěcond Lině\nThiřd Lině";
  const CodepointSpan span = {5, 23};
  // clang-format off
  std::vector<Token> tokens = {Token("Fiřst", 0, 5),
                               Token("Lině", 6, 10),
                               Token("Sěcond", 18, 23),
                               Token("Lině", 19, 23),
                               Token("Thiřd", 23, 28),
                               Token("Lině", 29, 33)};
  // clang-format on

  // Keeps the first line.
  internal::StripTokensFromOtherLines(context, span, &tokens);
  EXPECT_THAT(tokens, ElementsAreArray(
                          {Token("Fiřst", 0, 5), Token("Lině", 6, 10),
                           Token("Sěcond", 18, 23), Token("Lině", 19, 23),
                           Token("Thiřd", 23, 28), Token("Lině", 29, 33)}));
}

TEST(FeatureProcessorTest, GetFeaturesWithContextDropout) {
  FeatureProcessorOptions options;
  options.set_num_buckets(10);
  options.set_context_size(7);
  options.set_max_selection_span(7);
  options.add_chargram_orders(1);
  options.set_tokenize_on_space(true);
  options.set_context_dropout_probability(0.5);
  options.set_use_variable_context_dropout(true);
  TokenizationCodepointRange* config =
      options.add_tokenization_codepoint_config();
  config->set_start(32);
  config->set_end(33);
  config->set_role(TokenizationCodepointRange::WHITESPACE_SEPARATOR);
  FeatureProcessor feature_processor(options);

  // Test that two subsequent runs with random context dropout produce
  // different features.
  feature_processor.SetRandom(new std::mt19937);

  std::vector<std::vector<std::pair<int, float>>> features;
  std::vector<std::vector<std::pair<int, float>>> features2;
  std::vector<float> extra_features;
  std::vector<CodepointSpan> selection_label_spans;
  int selection_label;
  CodepointSpan selection_codepoint_label;
  int classification_label;
  EXPECT_TRUE(feature_processor.GetFeaturesAndLabels(
      "1 2 3 c o n t e x t X c o n t e x t 1 2 3", {20, 21}, {20, 21}, "",
      &features, &extra_features, &selection_label_spans, &selection_label,
      &selection_codepoint_label, &classification_label));
  EXPECT_TRUE(feature_processor.GetFeaturesAndLabels(
      "1 2 3 c o n t e x t X c o n t e x t 1 2 3", {20, 21}, {20, 21}, "",
      &features2, &extra_features, &selection_label_spans, &selection_label,
      &selection_codepoint_label, &classification_label));

  EXPECT_NE(features, features2);
}

TEST(FeatureProcessorTest, GetFeaturesWithLongerContext) {
  FeatureProcessorOptions options;
  options.set_num_buckets(10);
  options.set_context_size(9);
  options.set_max_selection_span(7);
  options.add_chargram_orders(1);
  options.set_tokenize_on_space(true);
  TokenizationCodepointRange* config =
      options.add_tokenization_codepoint_config();
  config->set_start(32);
  config->set_end(33);
  config->set_role(TokenizationCodepointRange::WHITESPACE_SEPARATOR);
  FeatureProcessor feature_processor(options);

  std::vector<std::vector<std::pair<int, float>>> features;
  std::vector<float> extra_features;
  std::vector<CodepointSpan> selection_label_spans;
  int selection_label;
  CodepointSpan selection_codepoint_label;
  int classification_label;
  EXPECT_TRUE(feature_processor.GetFeaturesAndLabels(
      "1 2 3 c o n t e x t X c o n t e x t 1 2 3", {20, 21}, {20, 21}, "",
      &features, &extra_features, &selection_label_spans, &selection_label,
      &selection_codepoint_label, &classification_label));
  EXPECT_EQ(19, features.size());

  // Should pad the string.
  EXPECT_TRUE(feature_processor.GetFeaturesAndLabels(
      "X", {0, 1}, {0, 1}, "", &features, &extra_features,
      &selection_label_spans, &selection_label, &selection_codepoint_label,
      &classification_label));
  EXPECT_EQ(19, features.size());
}

class TestingFeatureProcessor : public FeatureProcessor {
 public:
  using FeatureProcessor::FeatureProcessor;
  using FeatureProcessor::FindTokensInSelection;
};

TEST(FeatureProcessorTest, FindTokensInSelectionSingleCharacter) {
  FeatureProcessorOptions options;
  options.set_num_buckets(10);
  options.set_context_size(9);
  options.set_max_selection_span(7);
  options.add_chargram_orders(1);
  options.set_tokenize_on_space(true);
  TokenizationCodepointRange* config =
      options.add_tokenization_codepoint_config();
  config->set_start(32);
  config->set_end(33);
  config->set_role(TokenizationCodepointRange::WHITESPACE_SEPARATOR);
  TestingFeatureProcessor feature_processor(options);

  SelectionWithContext selection_with_context;
  selection_with_context.context = "1 2 3 c o n t e x t X c o n t e x t 1 2 3";

  // Selection and click indices of the X in the middle:
  selection_with_context.selection_start = 20;
  selection_with_context.selection_end = 21;
  // clang-format off
  EXPECT_THAT(feature_processor.FindTokensInSelection(
                  feature_processor.Tokenize(selection_with_context.context),
                  selection_with_context),
              ElementsAreArray({Token("X", 20, 21, false)}));
  // clang-format on
}

TEST(FeatureProcessorTest, FindTokensInSelectionInsideTokenBoundary) {
  FeatureProcessorOptions options;
  options.set_num_buckets(10);
  options.set_context_size(9);
  options.set_max_selection_span(7);
  options.add_chargram_orders(1);
  options.set_tokenize_on_space(true);
  TokenizationCodepointRange* config =
      options.add_tokenization_codepoint_config();
  config->set_start(32);
  config->set_end(33);
  config->set_role(TokenizationCodepointRange::WHITESPACE_SEPARATOR);
  TestingFeatureProcessor feature_processor(options);

  SelectionWithContext selection_with_context;
  selection_with_context.context = "I live at 350 Third Street, today.";

  const std::vector<Token> expected_selection = {
      // clang-format off
      Token("350", 10, 13, false),
      Token("Third", 14, 19, false),
      Token("Street,", 20, 27, false),
      // clang-format on
  };

  // Selection: I live at {350 Third Str}eet, today.
  selection_with_context.selection_start = 10;
  selection_with_context.selection_end = 23;
  EXPECT_THAT(feature_processor.FindTokensInSelection(
                  feature_processor.Tokenize(selection_with_context.context),
                  selection_with_context),
              ElementsAreArray(expected_selection));

  // Selection: I live at {350 Third Street,} today.
  selection_with_context.selection_start = 10;
  selection_with_context.selection_end = 27;
  EXPECT_THAT(feature_processor.FindTokensInSelection(
                  feature_processor.Tokenize(selection_with_context.context),
                  selection_with_context),
              ElementsAreArray(expected_selection));

  // Selection: I live at {350 Third Street, }today.
  selection_with_context.selection_start = 10;
  selection_with_context.selection_end = 28;
  EXPECT_THAT(feature_processor.FindTokensInSelection(
                  feature_processor.Tokenize(selection_with_context.context),
                  selection_with_context),
              ElementsAreArray(expected_selection));

  // Selection: I live at {350 Third S}treet, today.
  selection_with_context.selection_start = 10;
  selection_with_context.selection_end = 21;
  EXPECT_THAT(feature_processor.FindTokensInSelection(
                  feature_processor.Tokenize(selection_with_context.context),
                  selection_with_context),
              ElementsAreArray(expected_selection));

  // Test that when crossing the boundary, we select less/more.

  // Selection: I live at {350 Third} Street, today.
  selection_with_context.selection_start = 10;
  selection_with_context.selection_end = 19;
  EXPECT_THAT(feature_processor.FindTokensInSelection(
                  feature_processor.Tokenize(selection_with_context.context),
                  selection_with_context),
              ElementsAreArray({
                  // clang-format off
                  Token("350", 10, 13, false),
                  Token("Third", 14, 19, false),
                  // clang-format on
              }));

  // Selection: I live at {350 Third Street, t}oday.
  selection_with_context.selection_start = 10;
  selection_with_context.selection_end = 29;
  EXPECT_THAT(
      feature_processor.FindTokensInSelection(
          feature_processor.Tokenize(selection_with_context.context),
          selection_with_context),
      ElementsAreArray({
          // clang-format off
                  Token("350", 10, 13, false),
                  Token("Third", 14, 19, false),
                  Token("Street,", 20, 27, false),
                  Token("today.", 28, 34, false),
          // clang-format on
      }));
}

TEST(FeatureProcessorTest, CenterTokenFromClick) {
  int token_index;

  // Exactly aligned indices.
  token_index = internal::CenterTokenFromClick(
      {6, 11}, {Token("Hělló", 0, 5, false), Token("world", 6, 11, false),
                Token("heře!", 12, 17, false)});
  EXPECT_EQ(token_index, 1);

  // Click is contained in a token.
  token_index = internal::CenterTokenFromClick(
      {13, 17}, {Token("Hělló", 0, 5, false), Token("world", 6, 11, false),
                 Token("heře!", 12, 17, false)});
  EXPECT_EQ(token_index, 2);

  // Click spans two tokens.
  token_index = internal::CenterTokenFromClick(
      {6, 17}, {Token("Hělló", 0, 5, false), Token("world", 6, 11, false),
                Token("heře!", 12, 17, false)});
  EXPECT_EQ(token_index, kInvalidIndex);
}

TEST(FeatureProcessorTest, CenterTokenFromMiddleOfSelection) {
  SelectionWithContext selection;
  int token_index;

  // Selection of length 3. Exactly aligned indices.
  token_index = internal::CenterTokenFromMiddleOfSelection(
      {7, 27}, {Token("Token1", 0, 6, false), Token("Token2", 7, 13, false),
                Token("Token3", 14, 20, false), Token("Token4", 21, 27, false),
                Token("Token5", 28, 34, false)});
  EXPECT_EQ(token_index, 2);

  // Selection of length 1 token. Exactly aligned indices.
  token_index = internal::CenterTokenFromMiddleOfSelection(
      {21, 27}, {Token("Token1", 0, 6, false), Token("Token2", 7, 13, false),
                 Token("Token3", 14, 20, false), Token("Token4", 21, 27, false),
                 Token("Token5", 28, 34, false)});
  EXPECT_EQ(token_index, 3);

  // Selection marks sub-token range, with no tokens in it.
  token_index = internal::CenterTokenFromMiddleOfSelection(
      {29, 33}, {Token("Token1", 0, 6, false), Token("Token2", 7, 13, false),
                 Token("Token3", 14, 20, false), Token("Token4", 21, 27, false),
                 Token("Token5", 28, 34, false)});
  EXPECT_EQ(token_index, kInvalidIndex);

  // Selection of length 2. Sub-token indices.
  token_index = internal::CenterTokenFromMiddleOfSelection(
      {3, 25}, {Token("Token1", 0, 6, false), Token("Token2", 7, 13, false),
                Token("Token3", 14, 20, false), Token("Token4", 21, 27, false),
                Token("Token5", 28, 34, false)});
  EXPECT_EQ(token_index, 1);

  // Selection of length 1. Sub-token indices.
  token_index = internal::CenterTokenFromMiddleOfSelection(
      {22, 34}, {Token("Token1", 0, 6, false), Token("Token2", 7, 13, false),
                 Token("Token3", 14, 20, false), Token("Token4", 21, 27, false),
                 Token("Token5", 28, 34, false)});
  EXPECT_EQ(token_index, 4);
}

}  // namespace
}  // namespace libtextclassifier
