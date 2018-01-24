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

#ifndef KNOWLEDGE_CEREBRA_SENSE_TEXT_CLASSIFIER_LIB2_CACHED_FEATURES_H_
#define KNOWLEDGE_CEREBRA_SENSE_TEXT_CLASSIFIER_LIB2_CACHED_FEATURES_H_

#include <memory>
#include <vector>

#include "model-executor.h"
#include "model_generated.h"
#include "types.h"

namespace libtextclassifier2 {

// Holds state for extracting features across multiple calls and reusing them.
// Assumes that features for each Token are independent.
class CachedFeatures {
 public:
  CachedFeatures(
      const TokenSpan& extraction_span,
      const std::vector<std::vector<int>>& sparse_features,
      const std::vector<std::vector<float>>& dense_features,
      const std::vector<int>& padding_sparse_features,
      const std::vector<float>& padding_dense_features,
      const FeatureProcessorOptions_::BoundsSensitiveFeatures* config,
      EmbeddingExecutor* embedding_executor, int feature_vector_size);

  // Gets a vector of features for the given token span.
  std::vector<float> Get(TokenSpan selected_span) const;

 private:
  // Appends token features to the output. The intended_span specifies which
  // tokens' features should be used in principle. The read_mask_span restricts
  // which tokens are actually read. For tokens outside of the read_mask_span,
  // padding tokens are used instead.
  void AppendFeatures(const TokenSpan& intended_span,
                      const TokenSpan& read_mask_span,
                      std::vector<float>* output_features) const;

  // Appends features of one padding token to the output.
  void AppendPaddingFeatures(std::vector<float>* output_features) const;

  // Appends the features of tokens from the given span to the output. The
  // features are summed so that the appended features have the size
  // corresponding to one token.
  void AppendSummedFeatures(const TokenSpan& summing_span,
                            std::vector<float>* output_features) const;

  int NumFeaturesPerToken() const;

  const TokenSpan extraction_span_;
  const FeatureProcessorOptions_::BoundsSensitiveFeatures* config_;
  int output_features_size_;
  std::vector<float> features_;
  std::vector<float> padding_features_;
};

}  // namespace libtextclassifier2

#endif  // KNOWLEDGE_CEREBRA_SENSE_TEXT_CLASSIFIER_LIB2_CACHED_FEATURES_H_
