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

#include "smartselect/token-feature-extractor.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace libtextclassifier {
namespace {

TEST(TokenFeatureExtractorTest, Extract) {
  TokenFeatureExtractorOptions options;
  options.num_buckets = 10;
  options.chargram_orders = std::vector<int>{1, 2};
  options.extract_case_feature = true;
  options.extract_selection_mask_feature = true;
  TokenFeatureExtractor extractor(options);

  std::vector<int> sparse_features;
  std::vector<float> dense_features;

  extractor.Extract(Token{"Hělló", 0, 5, true}, &sparse_features,
                    &dense_features);

  EXPECT_THAT(
      sparse_features,
      testing::ElementsAreArray({8, 6, 0, 1, 1, 4, 7, 8, 8, 1, 4, 2, 7, 0, 4}));
  EXPECT_THAT(dense_features, testing::ElementsAreArray({1.0, 1.0}));

  sparse_features.clear();
  dense_features.clear();
  extractor.Extract(Token{"world!", 23, 29, false}, &sparse_features,
                    &dense_features);

  EXPECT_THAT(sparse_features, testing::ElementsAreArray(
                                   {9, 3, 3, 1, 5, 6, 7, 3, 5, 5, 2, 3, 7}));
  EXPECT_THAT(dense_features, testing::ElementsAreArray({-1.0, 0.0}));
}

TEST(TokenFeatureExtractorTest, ExtractForPadToken) {
  TokenFeatureExtractorOptions options;
  options.num_buckets = 10;
  options.chargram_orders = std::vector<int>{1, 2};
  options.extract_case_feature = true;
  options.extract_selection_mask_feature = true;

  TokenFeatureExtractor extractor(options);

  std::vector<int> sparse_features;
  std::vector<float> dense_features;

  extractor.Extract(Token(), &sparse_features, &dense_features);

  EXPECT_THAT(sparse_features, testing::ElementsAreArray({5}));
  EXPECT_THAT(dense_features, testing::ElementsAreArray({-1.0, 0.0}));
}

}  // namespace
}  // namespace libtextclassifier
