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

#include "smartselect/text-classification-model.h"

#include <fcntl.h>
#include <stdio.h>
#include <memory>
#include <string>

#include "base.h"
#include "gtest/gtest.h"

namespace libtextclassifier {
namespace {

std::string GetModelPath() {
  return "tests/testdata/smartselection.model";
}

TEST(TextClassificationModelTest, SuggestSelection) {
  const std::string model_path = GetModelPath();
  int fd = open(model_path.c_str(), O_RDONLY);
  std::unique_ptr<TextClassificationModel> ff_model(
      new TextClassificationModel(fd));
  close(fd);

  std::tuple<int, int> selection;
  selection = ff_model->SuggestSelection(
      "this afternoon Barack Obama gave a speech at", {15, 21});
  EXPECT_EQ(15, std::get<0>(selection));
  EXPECT_EQ(27, std::get<1>(selection));

  // Try passing whole string.
  selection =
      ff_model->SuggestSelection("350 Third Street, Cambridge", {0, 27});
  // If more than 1 token is specified, we should return back what entered.
  EXPECT_EQ(0, std::get<0>(selection));
  EXPECT_EQ(27, std::get<1>(selection));
}

TEST(TextClassificationModelTest, SuggestSelectionsAreSymmetric) {
  const std::string model_path = GetModelPath();
  int fd = open(model_path.c_str(), O_RDONLY);
  std::unique_ptr<TextClassificationModel> model(
      new TextClassificationModel(fd));
  close(fd);

  EXPECT_EQ(std::make_pair(0, 27),
            model->SuggestSelection("350 Third Street, Cambridge", {0, 3}));
  EXPECT_EQ(std::make_pair(0, 27),
            model->SuggestSelection("350 Third Street, Cambridge", {4, 9}));
  EXPECT_EQ(std::make_pair(0, 27),
            model->SuggestSelection("350 Third Street, Cambridge", {10, 16}));
  EXPECT_EQ(std::make_pair(6, 33),
            model->SuggestSelection("a\nb\nc\n350 Third Street, Cambridge",
                                    {16, 22}));
}

TEST(TextClassificationModelTest, SuggestSelectionWithPunctuation) {
  const std::string model_path = GetModelPath();
  int fd = open(model_path.c_str(), O_RDONLY);
  std::unique_ptr<TextClassificationModel> model(
      new TextClassificationModel(fd));
  close(fd);

  std::tuple<int, int> selection;

  // From the right.
  selection = model->SuggestSelection(
      "this afternoon Barack Obama, gave a speech at", {15, 21});
  EXPECT_EQ(15, std::get<0>(selection));
  EXPECT_EQ(27, std::get<1>(selection));

  // From the right multiple.
  selection = model->SuggestSelection(
      "this afternoon Barack Obama,.,.,, gave a speech at", {15, 21});
  EXPECT_EQ(15, std::get<0>(selection));
  EXPECT_EQ(27, std::get<1>(selection));

  // From the left multiple.
  selection = model->SuggestSelection(
      "this afternoon ,.,.,,Barack Obama gave a speech at", {21, 27});
  EXPECT_EQ(21, std::get<0>(selection));
  EXPECT_EQ(27, std::get<1>(selection));

  // From both sides.
  selection = model->SuggestSelection(
      "this afternoon !Barack Obama,- gave a speech at", {16, 22});
  EXPECT_EQ(16, std::get<0>(selection));
  EXPECT_EQ(28, std::get<1>(selection));
}

class TestingTextClassificationModel
    : public libtextclassifier::TextClassificationModel {
 public:
  explicit TestingTextClassificationModel(int fd)
      : libtextclassifier::TextClassificationModel(fd) {}

  CodepointSpan TestStripPunctuation(CodepointSpan selection,
                                     const std::string& context) const {
    return StripPunctuation(selection, context);
  }
};

TEST(TextClassificationModelTest, StripPunctuation) {
  const std::string model_path = GetModelPath();
  int fd = open(model_path.c_str(), O_RDONLY);
  std::unique_ptr<TestingTextClassificationModel> model(
      new TestingTextClassificationModel(fd));
  close(fd);

  EXPECT_EQ(std::make_pair(3, 10),
            model->TestStripPunctuation({0, 10}, ".,-abcd.()"));
  EXPECT_EQ(std::make_pair(0, 6),
            model->TestStripPunctuation({0, 6}, "(abcd)"));
  EXPECT_EQ(std::make_pair(1, 5),
            model->TestStripPunctuation({0, 6}, "[abcd]"));
  EXPECT_EQ(std::make_pair(1, 5),
            model->TestStripPunctuation({0, 6}, "{abcd}"));

  // Invalid indices
  EXPECT_EQ(std::make_pair(-1, 523),
            model->TestStripPunctuation({-1, 523}, "a"));
  EXPECT_EQ(std::make_pair(-1, -1), model->TestStripPunctuation({-1, -1}, "a"));
  EXPECT_EQ(std::make_pair(0, -1), model->TestStripPunctuation({0, -1}, "a"));
}

TEST(TextClassificationModelTest, SuggestSelectionNoCrashWithJunk) {
  const std::string model_path = GetModelPath();
  int fd = open(model_path.c_str(), O_RDONLY);
  std::unique_ptr<TextClassificationModel> ff_model(
      new TextClassificationModel(fd));
  close(fd);

  std::tuple<int, int> selection;

  // Try passing in bunch of invalid selections.
  selection = ff_model->SuggestSelection("", {0, 27});
  // If more than 1 token is specified, we should return back what entered.
  EXPECT_EQ(0, std::get<0>(selection));
  EXPECT_EQ(27, std::get<1>(selection));

  selection = ff_model->SuggestSelection("", {-10, 27});
  // If more than 1 token is specified, we should return back what entered.
  EXPECT_EQ(-10, std::get<0>(selection));
  EXPECT_EQ(27, std::get<1>(selection));

  selection = ff_model->SuggestSelection("Word 1 2 3 hello!", {0, 27});
  // If more than 1 token is specified, we should return back what entered.
  EXPECT_EQ(0, std::get<0>(selection));
  EXPECT_EQ(27, std::get<1>(selection));

  selection = ff_model->SuggestSelection("Word 1 2 3 hello!", {-30, 300});
  // If more than 1 token is specified, we should return back what entered.
  EXPECT_EQ(-30, std::get<0>(selection));
  EXPECT_EQ(300, std::get<1>(selection));

  selection = ff_model->SuggestSelection("Word 1 2 3 hello!", {-10, -1});
  // If more than 1 token is specified, we should return back what entered.
  EXPECT_EQ(-10, std::get<0>(selection));
  EXPECT_EQ(-1, std::get<1>(selection));

  selection = ff_model->SuggestSelection("Word 1 2 3 hello!", {100, 17});
  // If more than 1 token is specified, we should return back what entered.
  EXPECT_EQ(100, std::get<0>(selection));
  EXPECT_EQ(17, std::get<1>(selection));
}

namespace {

std::string FindBestResult(std::vector<std::pair<std::string, float>> results) {
  std::sort(results.begin(), results.end(),
            [](const std::pair<std::string, float> a,
               const std::pair<std::string, float> b) {
              return a.second > b.second;
            });
  return results[0].first;
}

}  // namespace

TEST(TextClassificationModelTest, ClassifyText) {
  const std::string model_path = GetModelPath();
  int fd = open(model_path.c_str(), O_RDONLY);
  std::unique_ptr<TextClassificationModel> model(
      new TextClassificationModel(fd));
  close(fd);

  EXPECT_EQ("other",
            FindBestResult(model->ClassifyText(
                "this afternoon Barack Obama gave a speech at", {15, 27})));
  EXPECT_EQ("email",
            FindBestResult(model->ClassifyText("you@android.com", {0, 15})));
  EXPECT_EQ("email", FindBestResult(model->ClassifyText(
                         "Contact me at you@android.com", {14, 29})));
  EXPECT_EQ("phone", FindBestResult(model->ClassifyText(
                         "Call me at (800) 123-456 today", {11, 24})));
  EXPECT_EQ("url", FindBestResult(model->ClassifyText(
                       "Visit www.google.com every today!", {6, 20})));
}

}  // namespace
}  // namespace libtextclassifier
