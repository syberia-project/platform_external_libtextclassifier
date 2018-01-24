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

#include "text-classifier.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace libtextclassifier2 {
namespace {

using testing::ElementsAreArray;
using testing::Pair;

std::string FirstResult(
    const std::vector<std::pair<std::string, float>>& results) {
  if (results.empty()) {
    return "<INVALID RESULTS>";
  }
  return results[0].first;
}

MATCHER_P3(IsAnnotatedSpan, start, end, best_class, "") {
  return testing::Value(arg.span, Pair(start, end)) &&
         testing::Value(FirstResult(arg.classification), best_class);
}

std::string ReadFile(const std::string& file_name) {
  std::ifstream file_stream(file_name);
  return std::string(std::istreambuf_iterator<char>(file_stream), {});
}

std::string GetModelPath() {
  return LIBTEXTCLASSIFIER_TEST_DATA_DIR;
}

TEST(TextClassifierTest, EmbeddingExecutorLoadingFails) {
  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromPath(GetModelPath() + "wrong_embeddings.fb");
  EXPECT_FALSE(classifier);
}

TEST(TextClassifierTest, ClassifyText) {
  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromPath(GetModelPath() + "test_model.fb");
  ASSERT_TRUE(classifier);

  EXPECT_EQ("other",
            FirstResult(classifier->ClassifyText(
                "this afternoon Barack Obama gave a speech at", {15, 27})));
  EXPECT_EQ("other",
            FirstResult(classifier->ClassifyText("you@android.com", {0, 15})));
  EXPECT_EQ("other", FirstResult(classifier->ClassifyText(
                         "Contact me at you@android.com", {14, 29})));
  EXPECT_EQ("phone", FirstResult(classifier->ClassifyText(
                         "Call me at (800) 123-456 today", {11, 24})));
  EXPECT_EQ("other", FirstResult(classifier->ClassifyText(
                         "Visit www.google.com every today!", {6, 20})));

  // More lines.
  EXPECT_EQ("other",
            FirstResult(classifier->ClassifyText(
                "this afternoon Barack Obama gave a speech at|Visit "
                "www.google.com every today!|Call me at (800) 123-456 today.",
                {15, 27})));
  EXPECT_EQ("other",
            FirstResult(classifier->ClassifyText(
                "this afternoon Barack Obama gave a speech at|Visit "
                "www.google.com every today!|Call me at (800) 123-456 today.",
                {51, 65})));
  EXPECT_EQ("phone",
            FirstResult(classifier->ClassifyText(
                "this afternoon Barack Obama gave a speech at|Visit "
                "www.google.com every today!|Call me at (800) 123-456 today.",
                {90, 103})));

  // Single word.
  EXPECT_EQ("other", FirstResult(classifier->ClassifyText("obama", {0, 5})));
  EXPECT_EQ("other", FirstResult(classifier->ClassifyText("asdf", {0, 4})));
  EXPECT_EQ("<INVALID RESULTS>",
            FirstResult(classifier->ClassifyText("asdf", {0, 0})));

  // Junk.
  EXPECT_EQ("<INVALID RESULTS>",
            FirstResult(classifier->ClassifyText("", {0, 0})));
  EXPECT_EQ("<INVALID RESULTS>", FirstResult(classifier->ClassifyText(
                                     "a\n\n\n\nx x x\n\n\n\n\n\n", {1, 5})));
}

TEST(TextClassifierTest, PhoneFiltering) {
  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromPath(GetModelPath() + "test_model.fb");
  ASSERT_TRUE(classifier);

  EXPECT_EQ("phone", FirstResult(classifier->ClassifyText(
                         "phone: (123) 456 789", {7, 20})));
  EXPECT_EQ("phone", FirstResult(classifier->ClassifyText(
                         "phone: (123) 456 789,0001112", {7, 25})));
  EXPECT_EQ("other", FirstResult(classifier->ClassifyText(
                         "phone: (123) 456 789,0001112", {7, 28})));
}

TEST(TextClassifierTest, SuggestSelection) {
  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromPath(GetModelPath() + "test_model.fb");
  ASSERT_TRUE(classifier);

  EXPECT_EQ(classifier->SuggestSelection(
                "this afternoon Barack Obama gave a speech at", {15, 21}),
            std::make_pair(15, 21));

  // Try passing whole string.
  // If more than 1 token is specified, we should return back what entered.
  EXPECT_EQ(
      classifier->SuggestSelection("350 Third Street, Cambridge", {0, 27}),
      std::make_pair(0, 27));

  // Single letter.
  EXPECT_EQ(classifier->SuggestSelection("a", {0, 1}), std::make_pair(0, 1));

  // Single word.
  EXPECT_EQ(classifier->SuggestSelection("asdf", {0, 4}), std::make_pair(0, 4));

  EXPECT_EQ(
      classifier->SuggestSelection("call me at 857 225 3556 today", {11, 14}),
      std::make_pair(11, 23));

  // Unpaired bracket stripping.
  EXPECT_EQ(
      classifier->SuggestSelection("call me at (857) 225 3556 today", {11, 16}),
      std::make_pair(11, 25));
  EXPECT_EQ(
      classifier->SuggestSelection("call me at (857 225 3556 today", {11, 15}),
      std::make_pair(12, 24));
  EXPECT_EQ(
      classifier->SuggestSelection("call me at 857 225 3556) today", {11, 14}),
      std::make_pair(11, 23));
  EXPECT_EQ(
      classifier->SuggestSelection("call me at )857 225 3556( today", {11, 15}),
      std::make_pair(12, 24));

  // If the resulting selection would be empty, the original span is returned.
  EXPECT_EQ(classifier->SuggestSelection("call me at )( today", {11, 13}),
            std::make_pair(11, 13));
  EXPECT_EQ(classifier->SuggestSelection("call me at ( today", {11, 12}),
            std::make_pair(11, 12));
  EXPECT_EQ(classifier->SuggestSelection("call me at ) today", {11, 12}),
            std::make_pair(11, 12));
}

TEST(TextClassifierTest, SuggestSelectionsAreSymmetric) {
  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromPath(GetModelPath() + "test_model.fb");
  ASSERT_TRUE(classifier);

  EXPECT_EQ(classifier->SuggestSelection("350 Third Street, Cambridge", {0, 3}),
            std::make_pair(0, 27));
  EXPECT_EQ(classifier->SuggestSelection("350 Third Street, Cambridge", {4, 9}),
            std::make_pair(0, 27));
  EXPECT_EQ(
      classifier->SuggestSelection("350 Third Street, Cambridge", {10, 16}),
      std::make_pair(0, 27));
  EXPECT_EQ(classifier->SuggestSelection("a\nb\nc\n350 Third Street, Cambridge",
                                         {16, 22}),
            std::make_pair(6, 33));
}

TEST(TextClassifierTest, SuggestSelectionWithNewLine) {
  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromPath(GetModelPath() + "test_model.fb");
  ASSERT_TRUE(classifier);

  EXPECT_EQ(classifier->SuggestSelection("abc\n857 225 3556", {4, 7}),
            std::make_pair(4, 16));
  EXPECT_EQ(classifier->SuggestSelection("857 225 3556\nabc", {0, 3}),
            std::make_pair(0, 12));
}

TEST(TextClassifierTest, SuggestSelectionWithPunctuation) {
  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromPath(GetModelPath() + "test_model.fb");
  ASSERT_TRUE(classifier);

  // From the right.
  EXPECT_EQ(classifier->SuggestSelection(
                "this afternoon BarackObama, gave a speech at", {15, 26}),
            std::make_pair(15, 26));

  // From the right multiple.
  EXPECT_EQ(classifier->SuggestSelection(
                "this afternoon BarackObama,.,.,, gave a speech at", {15, 26}),
            std::make_pair(15, 26));

  // From the left multiple.
  EXPECT_EQ(classifier->SuggestSelection(
                "this afternoon ,.,.,,BarackObama gave a speech at", {21, 32}),
            std::make_pair(21, 32));

  // From both sides.
  EXPECT_EQ(classifier->SuggestSelection(
                "this afternoon !BarackObama,- gave a speech at", {16, 27}),
            std::make_pair(16, 27));
}

TEST(TextClassifierTest, SuggestSelectionNoCrashWithJunk) {
  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromPath(GetModelPath() + "test_model.fb");
  ASSERT_TRUE(classifier);

  // Try passing in bunch of invalid selections.
  EXPECT_EQ(classifier->SuggestSelection("", {0, 27}), std::make_pair(0, 27));
  EXPECT_EQ(classifier->SuggestSelection("", {-10, 27}),
            std::make_pair(-10, 27));
  EXPECT_EQ(classifier->SuggestSelection("Word 1 2 3 hello!", {0, 27}),
            std::make_pair(0, 27));
  EXPECT_EQ(classifier->SuggestSelection("Word 1 2 3 hello!", {-30, 300}),
            std::make_pair(-30, 300));
  EXPECT_EQ(classifier->SuggestSelection("Word 1 2 3 hello!", {-10, -1}),
            std::make_pair(-10, -1));
  EXPECT_EQ(classifier->SuggestSelection("Word 1 2 3 hello!", {100, 17}),
            std::make_pair(100, 17));
}

TEST(TextClassifierTest, Annotate) {
  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromPath(GetModelPath() + "test_model.fb");
  ASSERT_TRUE(classifier);

  const std::string test_string =
      "& saw Barak Obama today .. 350 Third Street, Cambridge\nand my phone "
      "number is 853 225 3556.";
  EXPECT_THAT(classifier->Annotate(test_string),
              ElementsAreArray({
                  IsAnnotatedSpan(0, 0, "<INVALID RESULTS>"),
                  IsAnnotatedSpan(2, 5, "other"),
                  IsAnnotatedSpan(6, 11, "other"),
                  IsAnnotatedSpan(12, 17, "other"),
                  IsAnnotatedSpan(18, 23, "other"),
                  IsAnnotatedSpan(24, 24, "<INVALID RESULTS>"),
                  IsAnnotatedSpan(27, 54, "address"),
                  IsAnnotatedSpan(55, 58, "other"),
                  IsAnnotatedSpan(59, 61, "other"),
                  IsAnnotatedSpan(62, 67, "other"),
                  IsAnnotatedSpan(68, 74, "other"),
                  IsAnnotatedSpan(75, 77, "other"),
                  IsAnnotatedSpan(78, 90, "phone"),
              }));
}

// TODO(jacekj): Test the regex functionality.

}  // namespace
}  // namespace libtextclassifier2
