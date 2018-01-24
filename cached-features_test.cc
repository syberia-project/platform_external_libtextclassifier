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

#include "cached-features.h"

#include "model-executor.h"
#include "tensor-view.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::ElementsAreArray;
using testing::FloatEq;
using testing::Matcher;

namespace libtextclassifier2 {
namespace {

Matcher<std::vector<float>> ElementsAreFloat(const std::vector<float>& values) {
  std::vector<Matcher<float>> matchers;
  for (const float value : values) {
    matchers.push_back(FloatEq(value));
  }
  return ElementsAreArray(matchers);
}

// EmbeddingExecutor that always returns features based on
class FakeEmbeddingExecutor : public EmbeddingExecutor {
 public:
  bool AddEmbedding(const TensorView<int>& sparse_features, float* dest,
                    int dest_size) override {
    TC_CHECK_GE(dest_size, 2);
    EXPECT_EQ(sparse_features.size(), 1);

    dest[0] = sparse_features.data()[0] * 11.0f;
    dest[1] = -sparse_features.data()[0] * 11.0f;
    return true;
  }

 private:
  std::vector<float> storage_;
};

TEST(CachedFeaturesTest, Simple) {
  FeatureProcessorOptions_::BoundsSensitiveFeaturesT config;
  config.enabled = true;
  config.num_tokens_before = 2;
  config.num_tokens_inside_left = 2;
  config.num_tokens_inside_right = 2;
  config.num_tokens_after = 2;
  config.include_inside_bag = true;
  config.include_inside_length = true;
  flatbuffers::FlatBufferBuilder builder;
  builder.Finish(CreateBoundsSensitiveFeatures(builder, &config));
  flatbuffers::DetachedBuffer config_fb = builder.Release();

  std::vector<std::vector<int>> sparse_features(9);
  for (int i = 0; i < sparse_features.size(); ++i) {
    sparse_features[i].push_back(i + 1);
  }
  std::vector<std::vector<float>> dense_features(9);
  for (int i = 0; i < dense_features.size(); ++i) {
    dense_features[i].push_back((i + 1) * 0.1);
  }

  std::vector<int> padding_sparse_features = {10203};
  std::vector<float> padding_dense_features = {321.0};

  FakeEmbeddingExecutor executor;
  const CachedFeatures cached_features(
      {3, 9}, sparse_features, dense_features, padding_sparse_features,
      padding_dense_features,
      flatbuffers::GetRoot<FeatureProcessorOptions_::BoundsSensitiveFeatures>(
          config_fb.data()),
      &executor, /*feature_vector_size=*/3);

  EXPECT_THAT(cached_features.Get({5, 8}),
              ElementsAreFloat({11.0,  -11.0,  0.1, 22.0,     -22.0,     0.2,
                                33.0,  -33.0,  0.3, 44.0,     -44.0,     0.4,
                                44.0,  -44.0,  0.4, 55.0,     -55.0,     0.5,
                                66.0,  -66.0,  0.6, 112233.0, -112233.0, 321.0,
                                132.0, -132.0, 1.2, 3.0}));

  EXPECT_THAT(
      cached_features.Get({5, 7}),
      ElementsAreFloat({11.0,  -11.0, 0.1,   22.0,  -22.0, 0.2,   33.0,
                        -33.0, 0.3,   44.0,  -44.0, 0.4,   33.0,  -33.0,
                        0.3,   44.0,  -44.0, 0.4,   55.0,  -55.0, 0.5,
                        66.0,  -66.0, 0.6,   77.0,  -77.0, 0.7,   2.0}));

  EXPECT_THAT(
      cached_features.Get({6, 8}),
      ElementsAreFloat({22.0,     -22.0,     0.2,   33.0,  -33.0, 0.3,   44.0,
                        -44.0,    0.4,       55.0,  -55.0, 0.5,   44.0,  -44.0,
                        0.4,      55.0,      -55.0, 0.5,   66.0,  -66.0, 0.6,
                        112233.0, -112233.0, 321.0, 99.0,  -99.0, 0.9,   2.0}));

  EXPECT_THAT(
      cached_features.Get({6, 7}),
      ElementsAreFloat({22.0,     -22.0,     0.2,   33.0,     -33.0,     0.3,
                        44.0,     -44.0,     0.4,   112233.0, -112233.0, 321.0,
                        112233.0, -112233.0, 321.0, 44.0,     -44.0,     0.4,
                        55.0,     -55.0,     0.5,   66.0,     -66.0,     0.6,
                        44.0,     -44.0,     0.4,   1.0}));
}

}  // namespace
}  // namespace libtextclassifier2
