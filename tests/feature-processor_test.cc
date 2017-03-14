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
  SelectionWithContext selection;
  selection.context = "Fiřst Lině\nSěcond Lině\nThiřd Lině";

  // Keeps the first line.
  selection.click_start = 0;
  selection.click_end = 5;
  selection.selection_start = 6;
  selection.selection_end = 10;

  SelectionWithContext line_selection;
  int shift;
  std::tie(line_selection, shift) = internal::ExtractLineWithClick(selection);

  EXPECT_EQ(line_selection.context, "Fiřst Lině");
  EXPECT_EQ(line_selection.click_start, 0);
  EXPECT_EQ(line_selection.click_end, 5);
  EXPECT_EQ(line_selection.selection_start, 6);
  EXPECT_EQ(line_selection.selection_end, 10);
  EXPECT_EQ(shift, 0);
}

TEST(FeatureProcessorTest, KeepLineWithClickSecond) {
  SelectionWithContext selection;
  selection.context = "Fiřst Lině\nSěcond Lině\nThiřd Lině";

  // Keeps the second line.
  selection.click_start = 11;
  selection.click_end = 17;
  selection.selection_start = 18;
  selection.selection_end = 22;

  SelectionWithContext line_selection;
  int shift;
  std::tie(line_selection, shift) = internal::ExtractLineWithClick(selection);

  EXPECT_EQ(line_selection.context, "Sěcond Lině");
  EXPECT_EQ(line_selection.click_start, 0);
  EXPECT_EQ(line_selection.click_end, 6);
  EXPECT_EQ(line_selection.selection_start, 7);
  EXPECT_EQ(line_selection.selection_end, 11);
  EXPECT_EQ(shift, 11);
}

TEST(FeatureProcessorTest, KeepLineWithClickThird) {
  SelectionWithContext selection;
  selection.context = "Fiřst Lině\nSěcond Lině\nThiřd Lině";

  // Keeps the third line.
  selection.click_start = 29;
  selection.click_end = 33;
  selection.selection_start = 23;
  selection.selection_end = 28;

  SelectionWithContext line_selection;
  int shift;
  std::tie(line_selection, shift) = internal::ExtractLineWithClick(selection);

  EXPECT_EQ(line_selection.context, "Thiřd Lině");
  EXPECT_EQ(line_selection.click_start, 6);
  EXPECT_EQ(line_selection.click_end, 10);
  EXPECT_EQ(line_selection.selection_start, 0);
  EXPECT_EQ(line_selection.selection_end, 5);
  EXPECT_EQ(shift, 23);
}

TEST(FeatureProcessorTest, KeepLineWithClickSecondWithPipe) {
  SelectionWithContext selection;
  selection.context = "Fiřst Lině|Sěcond Lině\nThiřd Lině";

  // Keeps the second line.
  selection.click_start = 11;
  selection.click_end = 17;
  selection.selection_start = 18;
  selection.selection_end = 22;

  SelectionWithContext line_selection;
  int shift;
  std::tie(line_selection, shift) = internal::ExtractLineWithClick(selection);

  EXPECT_EQ(line_selection.context, "Sěcond Lině");
  EXPECT_EQ(line_selection.click_start, 0);
  EXPECT_EQ(line_selection.click_end, 6);
  EXPECT_EQ(line_selection.selection_start, 7);
  EXPECT_EQ(line_selection.selection_end, 11);
  EXPECT_EQ(shift, 11);
}

TEST(FeatureProcessorTest, KeepLineWithCrosslineClick) {
  SelectionWithContext selection;
  selection.context = "Fiřst Lině\nSěcond Lině\nThiřd Lině";

  // Selects across lines, so KeepLine should not do any changes.
  selection.click_start = 6;
  selection.click_end = 17;
  selection.selection_start = 0;
  selection.selection_end = 22;

  SelectionWithContext line_selection;
  int shift;
  std::tie(line_selection, shift) = internal::ExtractLineWithClick(selection);

  EXPECT_EQ(line_selection.context, "Fiřst Lině\nSěcond Lině\nThiřd Lině");
  EXPECT_EQ(line_selection.click_start, 6);
  EXPECT_EQ(line_selection.click_end, 17);
  EXPECT_EQ(line_selection.selection_start, 0);
  EXPECT_EQ(line_selection.selection_end, 22);
  EXPECT_EQ(shift, 0);
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

  SelectionWithContext selection_with_context;
  selection_with_context.context = "1 2 3 c o n t e x t X c o n t e x t 1 2 3";
  // Selection and click indices of the X in the middle:
  selection_with_context.selection_start = 20;
  selection_with_context.selection_end = 21;
  selection_with_context.click_start = 20;
  selection_with_context.click_end = 21;

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
      selection_with_context, &features, &extra_features,
      &selection_label_spans, &selection_label, &selection_codepoint_label,
      &classification_label));
  EXPECT_TRUE(feature_processor.GetFeaturesAndLabels(
      selection_with_context, &features2, &extra_features,
      &selection_label_spans, &selection_label, &selection_codepoint_label,
      &classification_label));

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

  SelectionWithContext selection_with_context;
  selection_with_context.context = "1 2 3 c o n t e x t X c o n t e x t 1 2 3";
  // Selection and click indices of the X in the middle:
  selection_with_context.selection_start = 20;
  selection_with_context.selection_end = 21;
  selection_with_context.click_start = 20;
  selection_with_context.click_end = 21;

  std::vector<std::vector<std::pair<int, float>>> features;
  std::vector<float> extra_features;
  std::vector<CodepointSpan> selection_label_spans;
  int selection_label;
  CodepointSpan selection_codepoint_label;
  int classification_label;
  EXPECT_TRUE(feature_processor.GetFeaturesAndLabels(
      selection_with_context, &features, &extra_features,
      &selection_label_spans, &selection_label, &selection_codepoint_label,
      &classification_label));
  EXPECT_EQ(19, features.size());

  // Should pad the string.
  selection_with_context.context = "X";
  selection_with_context.selection_start = 0;
  selection_with_context.selection_end = 1;
  selection_with_context.click_start = 0;
  selection_with_context.click_end = 1;
  EXPECT_TRUE(feature_processor.GetFeaturesAndLabels(
      selection_with_context, &features, &extra_features,
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

}  // namespace
}  // namespace libtextclassifier
