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

class TestingTokenFeatureExtractor : public TokenFeatureExtractor {
 public:
  using TokenFeatureExtractor::TokenFeatureExtractor;
  using TokenFeatureExtractor::HashToken;
};

TEST(TokenFeatureExtractorTest, ExtractAscii) {
  TokenFeatureExtractorOptions options;
  options.num_buckets = 1000;
  options.chargram_orders = std::vector<int>{1, 2, 3};
  options.extract_case_feature = true;
  options.unicode_aware_features = false;
  options.extract_selection_mask_feature = true;
  TestingTokenFeatureExtractor extractor(options);

  std::vector<int> sparse_features;
  std::vector<float> dense_features;

  extractor.Extract(Token{"Hello", 0, 5, true}, &sparse_features,
                    &dense_features);

  EXPECT_THAT(sparse_features,
              testing::ElementsAreArray({
                  // clang-format off
                  extractor.HashToken("H"),
                  extractor.HashToken("e"),
                  extractor.HashToken("l"),
                  extractor.HashToken("l"),
                  extractor.HashToken("o"),
                  extractor.HashToken("^H"),
                  extractor.HashToken("He"),
                  extractor.HashToken("el"),
                  extractor.HashToken("ll"),
                  extractor.HashToken("lo"),
                  extractor.HashToken("o$"),
                  extractor.HashToken("^He"),
                  extractor.HashToken("Hel"),
                  extractor.HashToken("ell"),
                  extractor.HashToken("llo"),
                  extractor.HashToken("lo$")
                  // clang-format on
              }));
  EXPECT_THAT(dense_features, testing::ElementsAreArray({1.0, 1.0}));

  sparse_features.clear();
  dense_features.clear();
  extractor.Extract(Token{"world!", 23, 29, false}, &sparse_features,
                    &dense_features);

  EXPECT_THAT(sparse_features,
              testing::ElementsAreArray({
                  // clang-format off
                  extractor.HashToken("w"),
                  extractor.HashToken("o"),
                  extractor.HashToken("r"),
                  extractor.HashToken("l"),
                  extractor.HashToken("d"),
                  extractor.HashToken("!"),
                  extractor.HashToken("^w"),
                  extractor.HashToken("wo"),
                  extractor.HashToken("or"),
                  extractor.HashToken("rl"),
                  extractor.HashToken("ld"),
                  extractor.HashToken("d!"),
                  extractor.HashToken("!$"),
                  extractor.HashToken("^wo"),
                  extractor.HashToken("wor"),
                  extractor.HashToken("orl"),
                  extractor.HashToken("rld"),
                  extractor.HashToken("ld!"),
                  extractor.HashToken("d!$"),
                  // clang-format on
              }));
  EXPECT_THAT(dense_features, testing::ElementsAreArray({-1.0, 0.0}));
}

TEST(TokenFeatureExtractorTest, ExtractUnicode) {
  TokenFeatureExtractorOptions options;
  options.num_buckets = 1000;
  options.chargram_orders = std::vector<int>{1, 2, 3};
  options.extract_case_feature = true;
  options.unicode_aware_features = true;
  options.extract_selection_mask_feature = true;
  TestingTokenFeatureExtractor extractor(options);

  std::vector<int> sparse_features;
  std::vector<float> dense_features;

  extractor.Extract(Token{"Hělló", 0, 5, true}, &sparse_features,
                    &dense_features);

  EXPECT_THAT(sparse_features,
              testing::ElementsAreArray({
                  // clang-format off
                  extractor.HashToken("H"),
                  extractor.HashToken("ě"),
                  extractor.HashToken("l"),
                  extractor.HashToken("l"),
                  extractor.HashToken("ó"),
                  extractor.HashToken("^H"),
                  extractor.HashToken("Hě"),
                  extractor.HashToken("ěl"),
                  extractor.HashToken("ll"),
                  extractor.HashToken("ló"),
                  extractor.HashToken("ó$"),
                  extractor.HashToken("^Hě"),
                  extractor.HashToken("Hěl"),
                  extractor.HashToken("ěll"),
                  extractor.HashToken("lló"),
                  extractor.HashToken("ló$")
                  // clang-format on
              }));
  EXPECT_THAT(dense_features, testing::ElementsAreArray({1.0, 1.0}));

  sparse_features.clear();
  dense_features.clear();
  extractor.Extract(Token{"world!", 23, 29, false}, &sparse_features,
                    &dense_features);

  EXPECT_THAT(sparse_features,
              testing::ElementsAreArray({
                  // clang-format off
                  extractor.HashToken("w"),
                  extractor.HashToken("o"),
                  extractor.HashToken("r"),
                  extractor.HashToken("l"),
                  extractor.HashToken("d"),
                  extractor.HashToken("!"),
                  extractor.HashToken("^w"),
                  extractor.HashToken("wo"),
                  extractor.HashToken("or"),
                  extractor.HashToken("rl"),
                  extractor.HashToken("ld"),
                  extractor.HashToken("d!"),
                  extractor.HashToken("!$"),
                  extractor.HashToken("^wo"),
                  extractor.HashToken("wor"),
                  extractor.HashToken("orl"),
                  extractor.HashToken("rld"),
                  extractor.HashToken("ld!"),
                  extractor.HashToken("d!$"),
                  // clang-format on
              }));
  EXPECT_THAT(dense_features, testing::ElementsAreArray({-1.0, -1.0}));
}

TEST(TokenFeatureExtractorTest, ICUCaseFeature) {
  TokenFeatureExtractorOptions options;
  options.num_buckets = 1000;
  options.chargram_orders = std::vector<int>{1, 2};
  options.extract_case_feature = true;
  options.unicode_aware_features = true;
  options.extract_selection_mask_feature = false;
  TokenFeatureExtractor extractor(options);

  std::vector<int> sparse_features;
  std::vector<float> dense_features;
  extractor.Extract(Token{"Hělló", 0, 5, true}, &sparse_features,
                    &dense_features);
  EXPECT_THAT(dense_features, testing::ElementsAreArray({1.0}));

  sparse_features.clear();
  dense_features.clear();
  extractor.Extract(Token{"world!", 23, 29, false}, &sparse_features,
                    &dense_features);
  EXPECT_THAT(dense_features, testing::ElementsAreArray({-1.0}));

  sparse_features.clear();
  dense_features.clear();
  extractor.Extract(Token{"Ř", 23, 29, false}, &sparse_features,
                    &dense_features);
  EXPECT_THAT(dense_features, testing::ElementsAreArray({1.0}));

  sparse_features.clear();
  dense_features.clear();
  extractor.Extract(Token{"ř", 23, 29, false}, &sparse_features,
                    &dense_features);
  EXPECT_THAT(dense_features, testing::ElementsAreArray({-1.0}));
}

TEST(TokenFeatureExtractorTest, DigitRemapping) {
  TokenFeatureExtractorOptions options;
  options.num_buckets = 1000;
  options.chargram_orders = std::vector<int>{1, 2};
  options.remap_digits = true;
  options.unicode_aware_features = false;
  TokenFeatureExtractor extractor(options);

  std::vector<int> sparse_features;
  std::vector<float> dense_features;
  extractor.Extract(Token{"9:30am", 0, 6, true}, &sparse_features,
                    &dense_features);

  std::vector<int> sparse_features2;
  extractor.Extract(Token{"5:32am", 0, 6, true}, &sparse_features2,
                    &dense_features);
  EXPECT_THAT(sparse_features, testing::ElementsAreArray(sparse_features2));

  extractor.Extract(Token{"10:32am", 0, 6, true}, &sparse_features2,
                    &dense_features);
  EXPECT_THAT(sparse_features,
              testing::Not(testing::ElementsAreArray(sparse_features2)));
}

TEST(TokenFeatureExtractorTest, DigitRemappingUnicode) {
  TokenFeatureExtractorOptions options;
  options.num_buckets = 1000;
  options.chargram_orders = std::vector<int>{1, 2};
  options.remap_digits = true;
  options.unicode_aware_features = true;
  TokenFeatureExtractor extractor(options);

  std::vector<int> sparse_features;
  std::vector<float> dense_features;
  extractor.Extract(Token{"9:30am", 0, 6, true}, &sparse_features,
                    &dense_features);

  std::vector<int> sparse_features2;
  extractor.Extract(Token{"5:32am", 0, 6, true}, &sparse_features2,
                    &dense_features);
  EXPECT_THAT(sparse_features, testing::ElementsAreArray(sparse_features2));

  extractor.Extract(Token{"10:32am", 0, 6, true}, &sparse_features2,
                    &dense_features);
  EXPECT_THAT(sparse_features,
              testing::Not(testing::ElementsAreArray(sparse_features2)));
}

TEST(TokenFeatureExtractorTest, RegexFeatures) {
  TokenFeatureExtractorOptions options;
  options.num_buckets = 1000;
  options.chargram_orders = std::vector<int>{1, 2};
  options.remap_digits = false;
  options.unicode_aware_features = false;
  options.regexp_features.push_back("^[a-z]+$");  // all lower case.
  options.regexp_features.push_back("^[0-9]+$");  // all digits.
  TokenFeatureExtractor extractor(options);

  std::vector<int> sparse_features;
  std::vector<float> dense_features;
  extractor.Extract(Token{"abCde", 0, 6, true}, &sparse_features,
                    &dense_features);
  EXPECT_THAT(dense_features, testing::ElementsAreArray({-1.0, -1.0}));

  dense_features.clear();
  extractor.Extract(Token{"abcde", 0, 6, true}, &sparse_features,
                    &dense_features);
  EXPECT_THAT(dense_features, testing::ElementsAreArray({1.0, -1.0}));

  dense_features.clear();
  extractor.Extract(Token{"12c45", 0, 6, true}, &sparse_features,
                    &dense_features);
  EXPECT_THAT(dense_features, testing::ElementsAreArray({-1.0, -1.0}));

  dense_features.clear();
  extractor.Extract(Token{"12345", 0, 6, true}, &sparse_features,
                    &dense_features);
  EXPECT_THAT(dense_features, testing::ElementsAreArray({-1.0, 1.0}));
}

TEST(TokenFeatureExtractorTest, ExtractInvalidUTF8) {
  TokenFeatureExtractorOptions options;
  options.num_buckets = 1000;
  options.chargram_orders = std::vector<int>{1, 2, 3, 4, 5, 100};
  options.extract_case_feature = true;
  options.unicode_aware_features = true;
  options.extract_selection_mask_feature = true;
  TestingTokenFeatureExtractor extractor(options);

  // Test that this runs. ASAN should catch problems.
  std::vector<int> sparse_features;
  std::vector<float> dense_features;
  extractor.Extract(Token{"\xf0👶👶¾👶🏿\xf0", 0, 7, true},
                    &sparse_features, &dense_features);
}

TEST(TokenFeatureExtractorTest, ExtractTooLongWord) {
  TokenFeatureExtractorOptions options;
  options.num_buckets = 1000;
  options.chargram_orders = std::vector<int>{22};
  options.extract_case_feature = true;
  options.unicode_aware_features = true;
  options.extract_selection_mask_feature = true;
  TestingTokenFeatureExtractor extractor(options);

  // Test that this runs. ASAN should catch problems.
  std::vector<int> sparse_features;
  std::vector<float> dense_features;
  extractor.Extract(Token{"abcdefghijklmnopqřstuvwxyz", 0, 0, true},
                    &sparse_features, &dense_features);

  EXPECT_THAT(sparse_features,
              testing::ElementsAreArray({
                  // clang-format off
                  extractor.HashToken("^abcdefghij\1qřstuvwxyz"),
                  extractor.HashToken("abcdefghij\1qřstuvwxyz$"),
                  // clang-format on
              }));
}

TEST(TokenFeatureExtractorTest, ExtractForPadToken) {
  TokenFeatureExtractorOptions options;
  options.num_buckets = 1000;
  options.chargram_orders = std::vector<int>{1, 2};
  options.extract_case_feature = true;
  options.unicode_aware_features = false;
  options.extract_selection_mask_feature = true;

  TestingTokenFeatureExtractor extractor(options);

  std::vector<int> sparse_features;
  std::vector<float> dense_features;

  extractor.Extract(Token(), &sparse_features, &dense_features);

  EXPECT_THAT(sparse_features,
              testing::ElementsAreArray({extractor.HashToken("<PAD>")}));
  EXPECT_THAT(dense_features, testing::ElementsAreArray({-1.0, 0.0}));
}

}  // namespace
}  // namespace libtextclassifier
