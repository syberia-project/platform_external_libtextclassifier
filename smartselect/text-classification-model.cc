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

#include <cmath>
#include <iterator>
#include <numeric>

#include "common/embedding-network.h"
#include "common/feature-extractor.h"
#include "common/memory_image/embedding-network-params-from-image.h"
#include "common/memory_image/memory-image-reader.h"
#include "common/mmap.h"
#include "common/softmax.h"
#include "smartselect/text-classification-model.pb.h"
#include "util/base/logging.h"
#include "util/utf8/unicodetext.h"

namespace libtextclassifier {

using nlp_core::EmbeddingNetwork;
using nlp_core::EmbeddingNetworkProto;
using nlp_core::FeatureVector;
using nlp_core::MemoryImageReader;
using nlp_core::MmapFile;
using nlp_core::MmapHandle;

CodepointSpan TextClassificationModel::StripPunctuation(
    CodepointSpan selection, const std::string& context) const {
  UnicodeText context_unicode = UTF8ToUnicodeText(context, /*do_copy=*/false);
  int context_length =
      std::distance(context_unicode.begin(), context_unicode.end());

  // Check that the indices are valid.
  if (selection.first < 0 || selection.first > context_length ||
      selection.second < 0 || selection.second > context_length) {
    return selection;
  }

  UnicodeText::const_iterator it;
  for (it = context_unicode.begin(), std::advance(it, selection.first);
       punctuation_to_strip_.find(*it) != punctuation_to_strip_.end();
       ++it, ++selection.first) {
  }

  for (it = context_unicode.begin(), std::advance(it, selection.second - 1);
       punctuation_to_strip_.find(*it) != punctuation_to_strip_.end();
       --it, --selection.second) {
  }

  return selection;
}

TextClassificationModel::TextClassificationModel(int fd) {
  initialized_ = LoadModels(fd);
  if (!initialized_) {
    TC_LOG(ERROR) << "Failed to load models";
    return;
  }

  selection_options_ = selection_params_->GetSelectionModelOptions();
  for (const int codepoint : selection_options_.punctuation_to_strip()) {
    punctuation_to_strip_.insert(codepoint);
  }
}

bool TextClassificationModel::LoadModels(int fd) {
  MmapHandle mmap_handle = MmapFile(fd);
  if (!mmap_handle.ok()) {
    return false;
  }

  // Read the length of the selection model.
  const char* model_data = reinterpret_cast<const char*>(mmap_handle.start());
  uint32 selection_model_length =
      LittleEndian::ToHost32(*reinterpret_cast<const uint32*>(model_data));
  model_data += sizeof(selection_model_length);

  selection_params_.reset(
      ModelParamsBuilder(model_data, selection_model_length, nullptr));
  if (!selection_params_.get()) {
    return false;
  }
  selection_network_.reset(new EmbeddingNetwork(selection_params_.get()));
  selection_feature_processor_.reset(
      new FeatureProcessor(selection_params_->GetFeatureProcessorOptions()));

  model_data += selection_model_length;
  uint32 sharing_model_length =
      LittleEndian::ToHost32(*reinterpret_cast<const uint32*>(model_data));
  model_data += sizeof(sharing_model_length);
  sharing_params_.reset(
      ModelParamsBuilder(model_data, sharing_model_length,
                         selection_params_->GetEmbeddingParams()));
  if (!sharing_params_.get()) {
    return false;
  }
  sharing_network_.reset(new EmbeddingNetwork(sharing_params_.get()));
  sharing_feature_processor_.reset(
      new FeatureProcessor(sharing_params_->GetFeatureProcessorOptions()));

  return true;
}

EmbeddingNetwork::Vector TextClassificationModel::InferInternal(
    const std::string& context, CodepointSpan span,
    const FeatureProcessor& feature_processor, const EmbeddingNetwork* network,
    std::vector<CodepointSpan>* selection_label_spans) const {
  std::vector<FeatureVector> features;
  std::vector<float> extra_features;
  const bool features_computed = feature_processor.GetFeatures(
      context, span, &features, &extra_features, selection_label_spans);

  EmbeddingNetwork::Vector scores;
  if (!features_computed) {
    TC_LOG(ERROR) << "Features not computed";
    return scores;
  }
  network->ComputeFinalScores(features, extra_features, &scores);
  return scores;
}

CodepointSpan TextClassificationModel::SuggestSelection(
    const std::string& context, CodepointSpan click_indices) const {
  if (!initialized_) {
    TC_LOG(ERROR) << "Not initialized";
    return click_indices;
  }

  if (std::get<0>(click_indices) >= std::get<1>(click_indices)) {
    TC_LOG(ERROR) << "Trying to run SuggestSelection with invalid indices:"
                  << std::get<0>(click_indices) << " "
                  << std::get<1>(click_indices);
    return click_indices;
  }

  CodepointSpan result;
  if (selection_options_.enforce_symmetry()) {
    result = SuggestSelectionSymmetrical(context, click_indices);
  } else {
    float score;
    std::tie(result, score) = SuggestSelectionInternal(context, click_indices);
  }

  if (selection_options_.strip_punctuation()) {
    result = StripPunctuation(result, context);
  }

  return result;
}

std::pair<CodepointSpan, float>
TextClassificationModel::SuggestSelectionInternal(
    const std::string& context, CodepointSpan click_indices) const {
  if (!initialized_) {
    TC_LOG(ERROR) << "Not initialized";
    return {click_indices, -1.0};
  }

  std::vector<CodepointSpan> selection_label_spans;
  EmbeddingNetwork::Vector scores =
      InferInternal(context, click_indices, *selection_feature_processor_,
                    selection_network_.get(), &selection_label_spans);

  if (!scores.empty()) {
    scores = nlp_core::ComputeSoftmax(scores);
    const int prediction =
        std::max_element(scores.begin(), scores.end()) - scores.begin();
    std::pair<CodepointIndex, CodepointIndex> selection =
        selection_label_spans[prediction];

    if (selection.first == kInvalidIndex || selection.second == kInvalidIndex) {
      TC_LOG(ERROR) << "Invalid indices predicted, returning input: "
                    << prediction << " " << selection.first << " "
                    << selection.second;
      return {click_indices, -1.0};
    }

    return {{selection.first, selection.second}, scores[prediction]};
  } else {
    TC_LOG(ERROR) << "Returning default selection: scores.size() = "
                  << scores.size();
    return {click_indices, -1.0};
  }
}

namespace {

int GetClickTokenIndex(const std::vector<Token>& tokens,
                       CodepointSpan click_indices) {
  TokenSpan span = CodepointSpanToTokenSpan(tokens, click_indices);
  if (span.second - span.first == 1) {
    return span.first;
  } else {
    for (int i = 0; i < tokens.size(); i++) {
      if (tokens[i].start <= click_indices.first &&
          tokens[i].end >= click_indices.second) {
        return i;
      }
    }
    return kInvalidIndex;
  }
}

}  // namespace

// Implements a greedy-search-like algorithm for making selections symmetric.
//
// Steps:
// 1. Get a set of selection proposals from places around the clicked word.
// 2. For each proposal (going from highest-scoring), check if the tokens that
//    the proposal selects are still free, otherwise claims them, if a proposal
//    that contains the clicked token is found, it is returned as the
//    suggestion.
//
// This algorithm should ensure that if a selection is proposed, it does not
// matter which word of it was tapped - all of them will lead to the same
// selection.
CodepointSpan TextClassificationModel::SuggestSelectionSymmetrical(
    const std::string& context, CodepointSpan click_indices) const {
  std::vector<Token> tokens = selection_feature_processor_->Tokenize(context);
  internal::StripTokensFromOtherLines(context, click_indices, &tokens);

  // const int click_index = GetClickTokenIndex(tokens, click_indices);
  const int click_index = internal::CenterTokenFromClick(click_indices, tokens);
  if (click_index == kInvalidIndex) {
    return click_indices;
  }

  const int symmetry_context_size = selection_options_.symmetry_context_size();

  // Scan in the symmetry context for selection span proposals.
  std::vector<std::pair<CodepointSpan, float>> proposals;
  for (int i = -symmetry_context_size; i < symmetry_context_size + 1; ++i) {
    const int token_index = click_index + i;
    if (token_index >= 0 && token_index < tokens.size()) {
      float score;
      CodepointSpan span;
      std::tie(span, score) = SuggestSelectionInternal(
          context, {tokens[token_index].start, tokens[token_index].end});
      proposals.push_back({span, score});
    }
  }

  // Sort selection span proposals by their respective probabilities.
  std::sort(
      proposals.begin(), proposals.end(),
      [](std::pair<CodepointSpan, float> a, std::pair<CodepointSpan, float> b) {
        return a.second > b.second;
      });

  // Go from the highest-scoring proposal and claim tokens. Tokens are marked as
  // claimed by the higher-scoring selection proposals, so that the
  // lower-scoring ones cannot use them. Returns the selection proposal if it
  // contains the clicked token.
  std::vector<int> used_tokens(tokens.size(), 0);
  for (auto span_result : proposals) {
    TokenSpan span = CodepointSpanToTokenSpan(tokens, span_result.first);
    if (span.first != kInvalidIndex && span.second != kInvalidIndex) {
      bool feasible = true;
      for (int i = span.first; i < span.second; i++) {
        if (used_tokens[i] != 0) {
          feasible = false;
          break;
        }
      }

      if (feasible) {
        if (span.first <= click_index && span.second > click_index) {
          return {span_result.first.first, span_result.first.second};
        }
        for (int i = span.first; i < span.second; i++) {
          used_tokens[i] = 1;
        }
      }
    }
  }

  return {click_indices.first, click_indices.second};
}

CodepointSpan TextClassificationModel::SuggestSelection(
    const SelectionWithContext& selection_with_context) const {
  CodepointSpan click_indices = {selection_with_context.click_start,
                                 selection_with_context.click_end};

  // If click_indices are unspecified, select the first token.
  if (click_indices == CodepointSpan({kInvalidIndex, kInvalidIndex})) {
    click_indices = selection_feature_processor_->ClickRandomTokenInSelection(
        selection_with_context);
  }

  return SuggestSelection(selection_with_context.context, click_indices);
}

std::vector<std::pair<std::string, float>>
TextClassificationModel::ClassifyText(const std::string& context,
                                      CodepointSpan selection_indices) const {
  if (!initialized_) {
    TC_LOG(ERROR) << "Not initialized";
    return {};
  }

  EmbeddingNetwork::Vector scores =
      InferInternal(context, selection_indices, *sharing_feature_processor_,
                    sharing_network_.get(), nullptr);
  if (scores.empty()) {
    TC_LOG(ERROR) << "Using default class";
    return {};
  }
  if (!scores.empty() &&
      scores.size() == sharing_feature_processor_->NumCollections()) {
    scores = nlp_core::ComputeSoftmax(scores);

    std::vector<std::pair<std::string, float>> result;
    for (int i = 0; i < scores.size(); i++) {
      result.push_back(
          {sharing_feature_processor_->LabelToCollection(i), scores[i]});
    }
    std::sort(result.begin(), result.end(),
              [](const std::pair<std::string, float>& a,
                 const std::pair<std::string, float>& b) {
                return a.second > b.second;
              });
    return result;
  } else {
    TC_LOG(ERROR) << "Using default class: scores.size() = " << scores.size();
    return {};
  }
}

}  // namespace libtextclassifier
