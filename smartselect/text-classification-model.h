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

// Inference code for the feed-forward text classification models.

#ifndef LIBTEXTCLASSIFIER_SMARTSELECT_TEXT_CLASSIFICATION_MODEL_H_
#define LIBTEXTCLASSIFIER_SMARTSELECT_TEXT_CLASSIFICATION_MODEL_H_

#include <memory>
#include <set>
#include <string>

#include "base.h"
#include "common/embedding-network.h"
#include "common/feature-extractor.h"
#include "common/memory_image/embedding-network-params-from-image.h"
#include "smartselect/feature-processor.h"
#include "smartselect/text-classification-model.pb.h"
#include "smartselect/types.h"

namespace libtextclassifier {

// Loads and holds the parameters of the inference network.
//
// This class overrides a couple of methods of EmbeddingNetworkParamsFromImage
// because we only have one embedding matrix for all positions of context,
// whereas the original class would have a separate one for each.
class ModelParams : public nlp_core::EmbeddingNetworkParamsFromImage {
 public:
  static ModelParams* Build(const void* start, uint64 num_bytes);

  const FeatureProcessorOptions& GetFeatureProcessorOptions() const {
    return feature_processor_options_;
  }

  const SelectionModelOptions& GetSelectionModelOptions() const {
    return selection_options_;
  }

 protected:
  int embeddings_size() const override { return context_size_ * 2 + 1; }

  int embedding_num_features_size() const override {
    return context_size_ * 2 + 1;
  }

  int embedding_num_features(int i) const override { return 1; }

  int embeddings_num_rows(int i) const override {
    return EmbeddingNetworkParamsFromImage::embeddings_num_rows(0);
  };

  int embeddings_num_cols(int i) const override {
    return EmbeddingNetworkParamsFromImage::embeddings_num_cols(0);
  };

  const void* embeddings_weights(int i) const override {
    return EmbeddingNetworkParamsFromImage::embeddings_weights(0);
  };

  nlp_core::QuantizationType embeddings_quant_type(int i) const override {
    return EmbeddingNetworkParamsFromImage::embeddings_quant_type(0);
  }

  const nlp_core::float16* embeddings_quant_scales(int i) const override {
    return EmbeddingNetworkParamsFromImage::embeddings_quant_scales(0);
  }

 private:
  ModelParams(const void* start, uint64 num_bytes,
              const SelectionModelOptions& selection_options,
              const FeatureProcessorOptions& feature_processor_options)
      : EmbeddingNetworkParamsFromImage(start, num_bytes),
        selection_options_(selection_options),
        feature_processor_options_(feature_processor_options),
        context_size_(feature_processor_options_.context_size()) {}

  SelectionModelOptions selection_options_;
  FeatureProcessorOptions feature_processor_options_;
  int context_size_;
};

// SmartSelection/Sharing feed-forward model.
class TextClassificationModel {
 public:
  // Loads TextClassificationModel from given file given by an int
  // file descriptor.
  explicit TextClassificationModel(int fd);

  // Runs inference for given a context and current selection (i.e. index
  // of the first and one past last selected characters (utf8 codepoint
  // offsets)). Returns the indices (utf8 codepoint offsets) of the selection
  // beginning character and one past selection end character.
  // Returns the original click_indices if an error occurs.
  // NOTE: The selection indices are passed in and returned in terms of
  // UTF8 codepoints (not bytes).
  // Requires that the model is a smart selection model.
  CodepointSpan SuggestSelection(const std::string& context,
                                 CodepointSpan click_indices) const;

  // Same as above but accepts a selection_with_context. Only used for
  // evaluation.
  CodepointSpan SuggestSelection(
      const SelectionWithContext& selection_with_context) const;

  // Classifies the selected text given the context string.
  // Requires that the model is a smart sharing model.
  // Returns an empty result if an error occurs.
  std::vector<std::pair<std::string, float>> ClassifyText(
      const std::string& context, CodepointSpan click_indices) const;

 protected:
  // Removes punctuation from the beginning and end of the selection and returns
  // the new selection span.
  CodepointSpan StripPunctuation(CodepointSpan selection,
                                 const std::string& context) const;

  // During evaluation we need access to the feature processor.
  FeatureProcessor* SelectionFeatureProcessor() const {
    return selection_feature_processor_.get();
  }

 private:
  bool LoadModels(int fd);

  nlp_core::EmbeddingNetwork::Vector InferInternal(
      const std::string& context, CodepointSpan click_indices,
      CodepointSpan selection_indices,
      const FeatureProcessor& feature_processor,
      const nlp_core::EmbeddingNetwork* network,
      std::vector<CodepointSpan>* selection_label_spans, int* selection_label,
      CodepointSpan* selection_codepoint_label,
      int* classification_label) const;

  // Returns a selection suggestion with a score.
  std::pair<CodepointSpan, float> SuggestSelectionInternal(
      const std::string& context, CodepointSpan click_indices) const;

  // Returns a selection suggestion and makes sure it's symmetric. Internally
  // runs several times SuggestSelectionInternal.
  CodepointSpan SuggestSelectionSymmetrical(const std::string& context,
                                            CodepointSpan click_indices) const;

  bool initialized_;
  std::unique_ptr<ModelParams> selection_params_;
  std::unique_ptr<FeatureProcessor> selection_feature_processor_;
  std::unique_ptr<nlp_core::EmbeddingNetwork> selection_network_;
  std::unique_ptr<FeatureProcessor> sharing_feature_processor_;
  std::unique_ptr<ModelParams> sharing_params_;
  std::unique_ptr<nlp_core::EmbeddingNetwork> sharing_network_;

  SelectionModelOptions selection_options_;
  std::set<int> punctuation_to_strip_;
};

}  // namespace libtextclassifier

#endif  // LIBTEXTCLASSIFIER_SMARTSELECT_TEXT_CLASSIFICATION_MODEL_H_
