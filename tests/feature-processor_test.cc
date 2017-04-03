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

class TestingFeatureProcessor : public FeatureProcessor {
 public:
  using FeatureProcessor::FeatureProcessor;
  using FeatureProcessor::SpanToLabel;
};

TEST(FeatureProcessorTest, SpanToLabel) {
  FeatureProcessorOptions options;
  options.set_context_size(1);
  options.set_max_selection_span(1);
  options.set_tokenize_on_space(true);
  options.set_snap_label_span_boundaries_to_containing_tokens(false);

  TokenizationCodepointRange* config =
      options.add_tokenization_codepoint_config();
  config->set_start(32);
  config->set_end(33);
  config->set_role(TokenizationCodepointRange::WHITESPACE_SEPARATOR);

  TestingFeatureProcessor feature_processor(options);
  std::vector<Token> tokens = feature_processor.Tokenize("one, two, three");
  ASSERT_EQ(3, tokens.size());
  int label;
  ASSERT_TRUE(feature_processor.SpanToLabel({5, 8}, tokens, &label));
  EXPECT_EQ(kInvalidLabel, label);
  ASSERT_TRUE(feature_processor.SpanToLabel({5, 9}, tokens, &label));
  EXPECT_NE(kInvalidLabel, label);
  TokenSpan token_span;
  feature_processor.LabelToTokenSpan(label, &token_span);
  EXPECT_EQ(0, token_span.first);
  EXPECT_EQ(0, token_span.second);

  // Reconfigure with snapping enabled.
  options.set_snap_label_span_boundaries_to_containing_tokens(true);
  TestingFeatureProcessor feature_processor2(options);
  int label2;
  ASSERT_TRUE(feature_processor2.SpanToLabel({5, 8}, tokens, &label2));
  EXPECT_EQ(label, label2);
  ASSERT_TRUE(feature_processor2.SpanToLabel({6, 9}, tokens, &label2));
  EXPECT_EQ(label, label2);
  ASSERT_TRUE(feature_processor2.SpanToLabel({5, 9}, tokens, &label2));
  EXPECT_EQ(label, label2);

  // Cross a token boundary.
  ASSERT_TRUE(feature_processor2.SpanToLabel({4, 9}, tokens, &label2));
  EXPECT_EQ(kInvalidLabel, label2);
  ASSERT_TRUE(feature_processor2.SpanToLabel({5, 10}, tokens, &label2));
  EXPECT_EQ(kInvalidLabel, label2);

  // Multiple tokens.
  options.set_context_size(2);
  options.set_max_selection_span(2);
  TestingFeatureProcessor feature_processor3(options);
  tokens = feature_processor3.Tokenize("zero, one, two, three, four");
  ASSERT_TRUE(feature_processor3.SpanToLabel({6, 15}, tokens, &label2));
  EXPECT_NE(kInvalidLabel, label2);
  feature_processor3.LabelToTokenSpan(label2, &token_span);
  EXPECT_EQ(1, token_span.first);
  EXPECT_EQ(0, token_span.second);

  int label3;
  ASSERT_TRUE(feature_processor3.SpanToLabel({6, 14}, tokens, &label3));
  EXPECT_EQ(label2, label3);
  ASSERT_TRUE(feature_processor3.SpanToLabel({6, 13}, tokens, &label3));
  EXPECT_EQ(label2, label3);
  ASSERT_TRUE(feature_processor3.SpanToLabel({7, 13}, tokens, &label3));
  EXPECT_EQ(label2, label3);
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

  // Some invalid ones.
  token_index = internal::CenterTokenFromMiddleOfSelection({7, 27}, {});
  EXPECT_EQ(token_index, -1);
}

TEST(FeatureProcessorTest, GetFeaturesForSharing) {
  FeatureProcessorOptions options;
  options.set_num_buckets(10);
  options.set_context_size(9);
  options.set_max_selection_span(7);
  options.add_chargram_orders(1);
  options.set_tokenize_on_space(true);
  options.set_center_token_selection_method(
      FeatureProcessorOptions::CENTER_TOKEN_MIDDLE_OF_SELECTION);
  options.set_only_use_line_with_click(true);
  options.set_split_tokens_on_selection_boundaries(true);
  options.set_extract_selection_mask_feature(true);
  TokenizationCodepointRange* config =
      options.add_tokenization_codepoint_config();
  config->set_start(32);
  config->set_end(33);
  config->set_role(TokenizationCodepointRange::WHITESPACE_SEPARATOR);
  config = options.add_tokenization_codepoint_config();
  config->set_start(10);
  config->set_end(11);
  config->set_role(TokenizationCodepointRange::WHITESPACE_SEPARATOR);
  FeatureProcessor feature_processor(options);

  std::vector<std::vector<std::pair<int, float>>> features;
  std::vector<float> extra_features;
  std::vector<CodepointSpan> selection_label_spans;
  int selection_label;
  CodepointSpan selection_codepoint_label;
  int classification_label;
  EXPECT_TRUE(feature_processor.GetFeaturesAndLabels(
      "line 1\nline2\nsome entity\n line 4", {13, 24}, {13, 24}, "", &features,
      &extra_features, &selection_label_spans, &selection_label,
      &selection_codepoint_label, &classification_label));
  EXPECT_EQ(19, features.size());
}

}  // namespace
}  // namespace libtextclassifier
