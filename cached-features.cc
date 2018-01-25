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

#include "tensor-view.h"
#include "util/base/logging.h"

namespace libtextclassifier2 {

namespace {

// Populates the features for one token into the target vector at an offset
// corresponding to the given token index. It builds the features to populate by
// embedding the sparse features and combining them with the dense featues.
// Embeds sparse features and the features of one token into the features
// vector.
bool PopulateTokenFeatures(int target_feature_index,
                           const std::vector<int>& sparse_features,
                           const std::vector<float>& dense_features,
                           int feature_vector_size,
                           EmbeddingExecutor* embedding_executor,
                           std::vector<float>* target_features) {
  const int sparse_embedding_size = feature_vector_size - dense_features.size();
  float* dest =
      target_features->data() + target_feature_index * feature_vector_size;

  // Embed sparse features.
  if (!embedding_executor->AddEmbedding(
          TensorView<int>(sparse_features.data(),
                          {static_cast<int>(sparse_features.size())}),
          dest, sparse_embedding_size)) {
    return false;
  }

  // Copy dense features.
  for (int j = 0; j < dense_features.size(); ++j) {
    dest[sparse_embedding_size + j] = dense_features[j];
  }

  return true;
}

}  // namespace

CachedFeatures::CachedFeatures(
    const TokenSpan& extraction_span,
    const std::vector<std::vector<int>>& sparse_features,
    const std::vector<std::vector<float>>& dense_features,
    const std::vector<int>& padding_sparse_features,
    const std::vector<float>& padding_dense_features,
    const FeatureProcessorOptions_::BoundsSensitiveFeatures* config,
    EmbeddingExecutor* embedding_executor, int feature_vector_size)
    : extraction_span_(extraction_span), config_(config) {
  int num_extracted_tokens = 0;
  num_extracted_tokens += config->num_tokens_before();
  num_extracted_tokens += config->num_tokens_inside_left();
  num_extracted_tokens += config->num_tokens_inside_right();
  num_extracted_tokens += config->num_tokens_after();
  if (config->include_inside_bag()) {
    ++num_extracted_tokens;
  }
  output_features_size_ = num_extracted_tokens * feature_vector_size;
  if (config->include_inside_length()) {
    ++output_features_size_;
  }

  features_.resize(feature_vector_size * TokenSpanSize(extraction_span));
  for (int i = 0; i < TokenSpanSize(extraction_span); ++i) {
    if (!PopulateTokenFeatures(/*target_feature_index=*/i, sparse_features[i],
                               dense_features[i], feature_vector_size,
                               embedding_executor, &features_)) {
      TC_LOG(ERROR) << "Could not embed sparse token features.";
      return;
    }
  }

  padding_features_.resize(feature_vector_size);
  if (!PopulateTokenFeatures(/*target_feature_index=*/0,
                             padding_sparse_features, padding_dense_features,
                             feature_vector_size, embedding_executor,
                             &padding_features_)) {
    TC_LOG(ERROR) << "Could not embed sparse padding token features.";
    return;
  }
}

std::vector<float> CachedFeatures::Get(TokenSpan selected_span) const {
  selected_span.first -= extraction_span_.first;
  selected_span.second -= extraction_span_.first;

  std::vector<float> output_features;
  output_features.reserve(output_features_size_);

  // Append the features for tokens around the left bound. Masks out tokens
  // after the right bound, so that if num_tokens_inside_left goes past it,
  // padding tokens will be used.
  AppendFeatures(
      /*intended_span=*/{selected_span.first - config_->num_tokens_before(),
                         selected_span.first +
                             config_->num_tokens_inside_left()},
      /*read_mask_span=*/{0, selected_span.second}, &output_features);

  // Append the features for tokens around the right bound. Masks out tokens
  // before the left bound, so that if num_tokens_inside_right goes past it,
  // padding tokens will be used.
  AppendFeatures(
      /*intended_span=*/{selected_span.second -
                             config_->num_tokens_inside_right(),
                         selected_span.second + config_->num_tokens_after()},
      /*read_mask_span=*/{selected_span.first, TokenSpanSize(extraction_span_)},
      &output_features);

  if (config_->include_inside_bag()) {
    AppendSummedFeatures(selected_span, &output_features);
  }

  if (config_->include_inside_length()) {
    output_features.push_back(static_cast<float>(TokenSpanSize(selected_span)));
  }

  return output_features;
}

void CachedFeatures::AppendFeatures(const TokenSpan& intended_span,
                                    const TokenSpan& read_mask_span,
                                    std::vector<float>* output_features) const {
  const TokenSpan copy_span =
      IntersectTokenSpans(intended_span, read_mask_span);
  for (int i = intended_span.first; i < copy_span.first; ++i) {
    AppendPaddingFeatures(output_features);
  }
  output_features->insert(
      output_features->end(),
      features_.begin() + copy_span.first * NumFeaturesPerToken(),
      features_.begin() + copy_span.second * NumFeaturesPerToken());
  for (int i = copy_span.second; i < intended_span.second; ++i) {
    AppendPaddingFeatures(output_features);
  }
}

void CachedFeatures::AppendPaddingFeatures(
    std::vector<float>* output_features) const {
  output_features->insert(output_features->end(), padding_features_.begin(),
                          padding_features_.end());
}

void CachedFeatures::AppendSummedFeatures(
    const TokenSpan& summing_span, std::vector<float>* output_features) const {
  const int offset = output_features->size();
  output_features->resize(output_features->size() + NumFeaturesPerToken());
  for (int i = summing_span.first; i < summing_span.second; ++i) {
    for (int j = 0; j < NumFeaturesPerToken(); ++j) {
      (*output_features)[offset + j] +=
          features_[i * NumFeaturesPerToken() + j];
    }
  }
}

int CachedFeatures::NumFeaturesPerToken() const {
  return padding_features_.size();
}

}  // namespace libtextclassifier2
