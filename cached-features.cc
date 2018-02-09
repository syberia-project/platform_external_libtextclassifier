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

int CalculateOutputFeaturesSize(const FeatureProcessorOptions* options,
                                int feature_vector_size) {
  const bool bounds_sensitive_enabled =
      options->bounds_sensitive_features() &&
      options->bounds_sensitive_features()->enabled();

  int num_extracted_tokens = 0;
  if (bounds_sensitive_enabled) {
    const FeatureProcessorOptions_::BoundsSensitiveFeatures* config =
        options->bounds_sensitive_features();
    num_extracted_tokens += config->num_tokens_before();
    num_extracted_tokens += config->num_tokens_inside_left();
    num_extracted_tokens += config->num_tokens_inside_right();
    num_extracted_tokens += config->num_tokens_after();
    if (config->include_inside_bag()) {
      ++num_extracted_tokens;
    }
  } else {
    num_extracted_tokens = 2 * options->context_size() + 1;
  }

  int output_features_size = num_extracted_tokens * feature_vector_size;

  if (bounds_sensitive_enabled &&
      options->bounds_sensitive_features()->include_inside_length()) {
    ++output_features_size;
  }

  return output_features_size;
}

}  // namespace

std::unique_ptr<CachedFeatures> CachedFeatures::Create(
    const TokenSpan& extraction_span,
    const std::vector<std::vector<int>>& sparse_features,
    const std::vector<std::vector<float>>& dense_features,
    const std::vector<int>& padding_sparse_features,
    const std::vector<float>& padding_dense_features,
    const FeatureProcessorOptions* options,
    EmbeddingExecutor* embedding_executor, int feature_vector_size) {
  const int min_feature_version =
      options->bounds_sensitive_features() &&
              options->bounds_sensitive_features()->enabled()
          ? 2
          : 1;
  if (options->feature_version() < min_feature_version) {
    TC_LOG(ERROR) << "Unsupported feature version.";
    return nullptr;
  }

  std::unique_ptr<CachedFeatures> cached_features(new CachedFeatures());
  cached_features->extraction_span_ = extraction_span;
  cached_features->options_ = options;

  cached_features->output_features_size_ =
      CalculateOutputFeaturesSize(options, feature_vector_size);

  cached_features->features_.resize(feature_vector_size *
                                    TokenSpanSize(extraction_span));
  for (int i = 0; i < TokenSpanSize(extraction_span); ++i) {
    if (!PopulateTokenFeatures(/*target_feature_index=*/i, sparse_features[i],
                               dense_features[i], feature_vector_size,
                               embedding_executor,
                               &cached_features->features_)) {
      TC_LOG(ERROR) << "Could not embed sparse token features.";
      return nullptr;
    }
  }

  cached_features->padding_features_.resize(feature_vector_size);
  if (!PopulateTokenFeatures(/*target_feature_index=*/0,
                             padding_sparse_features, padding_dense_features,
                             feature_vector_size, embedding_executor,
                             &cached_features->padding_features_)) {
    TC_LOG(ERROR) << "Could not embed sparse padding token features.";
    return nullptr;
  }

  return cached_features;
}

void CachedFeatures::AppendClickContextFeaturesForClick(
    int click_pos, std::vector<float>* output_features) const {
  click_pos -= extraction_span_.first;

  AppendFeaturesInternal(
      /*intended_span=*/ExpandTokenSpan(SingleTokenSpan(click_pos),
                                        options_->context_size(),
                                        options_->context_size()),
      /*read_mask_span=*/{0, TokenSpanSize(extraction_span_)}, output_features);
}

void CachedFeatures::AppendBoundsSensitiveFeaturesForSpan(
    TokenSpan selected_span, std::vector<float>* output_features) const {
  const FeatureProcessorOptions_::BoundsSensitiveFeatures* config =
      options_->bounds_sensitive_features();

  selected_span.first -= extraction_span_.first;
  selected_span.second -= extraction_span_.first;

  // Append the features for tokens around the left bound. Masks out tokens
  // after the right bound, so that if num_tokens_inside_left goes past it,
  // padding tokens will be used.
  AppendFeaturesInternal(
      /*intended_span=*/{selected_span.first - config->num_tokens_before(),
                         selected_span.first +
                             config->num_tokens_inside_left()},
      /*read_mask_span=*/{0, selected_span.second}, output_features);

  // Append the features for tokens around the right bound. Masks out tokens
  // before the left bound, so that if num_tokens_inside_right goes past it,
  // padding tokens will be used.
  AppendFeaturesInternal(
      /*intended_span=*/{selected_span.second -
                             config->num_tokens_inside_right(),
                         selected_span.second + config->num_tokens_after()},
      /*read_mask_span=*/{selected_span.first, TokenSpanSize(extraction_span_)},
      output_features);

  if (config->include_inside_bag()) {
    AppendBagFeatures(selected_span, output_features);
  }

  if (config->include_inside_length()) {
    output_features->push_back(
        static_cast<float>(TokenSpanSize(selected_span)));
  }
}

void CachedFeatures::AppendFeaturesInternal(
    const TokenSpan& intended_span, const TokenSpan& read_mask_span,
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

void CachedFeatures::AppendBagFeatures(
    const TokenSpan& bag_span, std::vector<float>* output_features) const {
  const int offset = output_features->size();
  output_features->resize(output_features->size() + NumFeaturesPerToken());
  for (int i = bag_span.first; i < bag_span.second; ++i) {
    for (int j = 0; j < NumFeaturesPerToken(); ++j) {
      (*output_features)[offset + j] +=
          features_[i * NumFeaturesPerToken() + j] / TokenSpanSize(bag_span);
    }
  }
}

int CachedFeatures::NumFeaturesPerToken() const {
  return padding_features_.size();
}

}  // namespace libtextclassifier2
