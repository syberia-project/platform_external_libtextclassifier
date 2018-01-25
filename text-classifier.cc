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

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iterator>
#include <numeric>

#include "util/base/logging.h"
#include "util/math/softmax.h"
#include "util/utf8/unicodetext.h"

namespace libtextclassifier2 {
namespace {
const Model* LoadAndVerifyModel(const void* addr, int size) {
  const Model* model = flatbuffers::GetRoot<Model>(addr);

  flatbuffers::Verifier verifier(reinterpret_cast<const uint8_t*>(addr), size);
  if (model->Verify(verifier)) {
    return model;
  } else {
    return nullptr;
  }
}
}  // namespace

std::unique_ptr<TextClassifier> TextClassifier::FromUnownedBuffer(
    const char* buffer, int size) {
  const Model* model = LoadAndVerifyModel(buffer, size);
  if (model == nullptr) {
    return nullptr;
  }

  auto classifier = std::unique_ptr<TextClassifier>(new TextClassifier(model));
  if (!classifier->IsInitialized()) {
    return nullptr;
  }

  return classifier;
}

std::unique_ptr<TextClassifier> TextClassifier::FromScopedMmap(
    std::unique_ptr<ScopedMmap>* mmap) {
  if (!(*mmap)->handle().ok()) {
    TC_VLOG(1) << "Mmap failed.";
    return nullptr;
  }

  const Model* model = LoadAndVerifyModel((*mmap)->handle().start(),
                                          (*mmap)->handle().num_bytes());
  if (!model) {
    TC_LOG(ERROR) << "Model verification failed.";
    return nullptr;
  }

  auto classifier =
      std::unique_ptr<TextClassifier>(new TextClassifier(mmap, model));
  if (!classifier->IsInitialized()) {
    return nullptr;
  }

  return classifier;
}

std::unique_ptr<TextClassifier> TextClassifier::FromFileDescriptor(int fd,
                                                                   int offset,
                                                                   int size) {
  std::unique_ptr<ScopedMmap> mmap(new ScopedMmap(fd, offset, size));
  return FromScopedMmap(&mmap);
}

std::unique_ptr<TextClassifier> TextClassifier::FromFileDescriptor(int fd) {
  std::unique_ptr<ScopedMmap> mmap(new ScopedMmap(fd));
  return FromScopedMmap(&mmap);
}

std::unique_ptr<TextClassifier> TextClassifier::FromPath(
    const std::string& path) {
  std::unique_ptr<ScopedMmap> mmap(new ScopedMmap(path));
  return FromScopedMmap(&mmap);
}

void TextClassifier::ValidateAndInitialize() {
  if (model_ == nullptr) {
    TC_LOG(ERROR) << "No model specified.";
    initialized_ = false;
    return;
  }

  if (!model_->selection_options()) {
    TC_LOG(ERROR) << "No selection options.";
    initialized_ = false;
    return;
  }

  if (!model_->classification_options()) {
    TC_LOG(ERROR) << "No classification options.";
    initialized_ = false;
    return;
  }

  if (!model_->selection_feature_options()) {
    TC_LOG(ERROR) << "No selection feature options.";
    initialized_ = false;
    return;
  }

  if (!model_->classification_feature_options()) {
    TC_LOG(ERROR) << "No classification feature options.";
    initialized_ = false;
    return;
  }

  if (!model_->classification_feature_options()->bounds_sensitive_features()) {
    TC_LOG(ERROR) << "No classification bounds sensitive feature options.";
    initialized_ = false;
    return;
  }

  if (!model_->selection_feature_options()->bounds_sensitive_features()) {
    TC_LOG(ERROR) << "No selection bounds sensitive feature options.";
    initialized_ = false;
    return;
  }

  if (!model_->selection_model()) {
    TC_LOG(ERROR) << "No selection model.";
    initialized_ = false;
    return;
  }

  if (!model_->embedding_model()) {
    TC_LOG(ERROR) << "No embedding model.";
    initialized_ = false;
    return;
  }

  if (!model_->classification_model()) {
    TC_LOG(ERROR) << "No clf model.";
    initialized_ = false;
    return;
  }

  if (model_->regex_options()) {
    InitializeRegexModel();
  }

  embedding_executor_.reset(new TFLiteEmbeddingExecutor(
      flatbuffers::GetRoot<tflite::Model>(model_->embedding_model()->data())));
  if (!embedding_executor_ || !embedding_executor_->IsReady()) {
    TC_LOG(ERROR) << "Could not initialize embedding executor.";
    initialized_ = false;
    return;
  }
  selection_executor_.reset(new ModelExecutor(
      flatbuffers::GetRoot<tflite::Model>(model_->selection_model()->data())));
  if (!selection_executor_) {
    TC_LOG(ERROR) << "Could not initialize selection executor.";
    initialized_ = false;
    return;
  }
  classification_executor_.reset(
      new ModelExecutor(flatbuffers::GetRoot<tflite::Model>(
          model_->classification_model()->data())));
  if (!classification_executor_) {
    TC_LOG(ERROR) << "Could not initialize classification executor.";
    initialized_ = false;
    return;
  }

  selection_feature_processor_.reset(
      new FeatureProcessor(model_->selection_feature_options(), unilib_.get()));
  classification_feature_processor_.reset(new FeatureProcessor(
      model_->classification_feature_options(), unilib_.get()));

  initialized_ = true;
}

void TextClassifier::InitializeRegexModel() {
  if (!model_->regex_options()->patterns()) {
    initialized_ = false;
    TC_LOG(ERROR) << "No patterns in the regex config.";
    return;
  }

  // Initialize pattern recognizers.
  for (const auto& regex_pattern : *model_->regex_options()->patterns()) {
    std::unique_ptr<UniLib::RegexPattern> compiled_pattern(
        unilib_->CreateRegexPattern(regex_pattern->pattern()->c_str()));

    if (!compiled_pattern) {
      TC_LOG(WARNING) << "Failed to load pattern"
                      << regex_pattern->pattern()->str();
      continue;
    }

    regex_patterns_.push_back(
        {regex_pattern->collection_name()->str(), std::move(compiled_pattern)});
  }
}

namespace {

int CountDigits(const std::string& str, CodepointSpan selection_indices) {
  int count = 0;
  int i = 0;
  const UnicodeText unicode_str = UTF8ToUnicodeText(str, /*do_copy=*/false);
  for (auto it = unicode_str.begin(); it != unicode_str.end(); ++it, ++i) {
    if (i >= selection_indices.first && i < selection_indices.second &&
        isdigit(*it)) {
      ++count;
    }
  }
  return count;
}

std::string ExtractSelection(const std::string& context,
                             CodepointSpan selection_indices) {
  const UnicodeText context_unicode =
      UTF8ToUnicodeText(context, /*do_copy=*/false);
  auto selection_begin = context_unicode.begin();
  std::advance(selection_begin, selection_indices.first);
  auto selection_end = context_unicode.begin();
  std::advance(selection_end, selection_indices.second);
  return UnicodeText::UTF8Substring(selection_begin, selection_end);
}

}  // namespace

CodepointSpan TextClassifier::SuggestSelection(
    const std::string& context, CodepointSpan click_indices) const {
  if (!initialized_) {
    TC_LOG(ERROR) << "Not initialized";
    return click_indices;
  }

  const int context_codepoint_size =
      UTF8ToUnicodeText(context, /*do_copy=*/false).size();

  if (click_indices.first < 0 || click_indices.second < 0 ||
      click_indices.first >= context_codepoint_size ||
      click_indices.second > context_codepoint_size ||
      click_indices.first >= click_indices.second) {
    TC_VLOG(1) << "Trying to run SuggestSelection with invalid indices: "
               << click_indices.first << " " << click_indices.second;
    return click_indices;
  }

  std::vector<Token> tokens;
  int click_pos;
  selection_feature_processor_->TokenizeAndFindClick(context, click_indices,
                                                     &tokens, &click_pos);
  if (click_pos == kInvalidIndex) {
    TC_VLOG(1) << "Could not calculate the click position.";
    return click_indices;
  }

  const int symmetry_context_size =
      model_->selection_options()->symmetry_context_size();
  const int max_selection_span =
      selection_feature_processor_->GetOptions()->max_selection_span();
  const FeatureProcessorOptions_::BoundsSensitiveFeatures*
      bounds_sensitive_features =
          model_->selection_feature_options()->bounds_sensitive_features();

  // The symmetry context span is the clicked token with symmetry_context_size
  // tokens on either side.
  const TokenSpan symmetry_context_span = IntersectTokenSpans(
      ExpandTokenSpan(SingleTokenSpan(click_pos),
                      /*num_tokens_left=*/symmetry_context_size,
                      /*num_tokens_right=*/symmetry_context_size),
      {0, tokens.size()});

  // The extraction span is the symmetry context span expanded to include
  // max_selection_span tokens on either side, which is how far a selection can
  // stretch from the click, plus a relevant number of tokens outside of the
  // bounds of the selection.
  const TokenSpan extraction_span = IntersectTokenSpans(
      ExpandTokenSpan(symmetry_context_span,
                      /*num_tokens_left=*/max_selection_span +
                          bounds_sensitive_features->num_tokens_before(),
                      /*num_tokens_right=*/max_selection_span +
                          bounds_sensitive_features->num_tokens_after()),
      {0, tokens.size()});

  std::unique_ptr<CachedFeatures> cached_features;
  if (!classification_feature_processor_->ExtractFeatures(
          tokens, extraction_span, embedding_executor_.get(),
          classification_feature_processor_->EmbeddingSize() +
              classification_feature_processor_->DenseFeaturesCount(),
          &cached_features)) {
    TC_LOG(ERROR) << "Could not extract features.";
    return click_indices;
  }

  std::vector<TokenSpan> chunks;
  if (!Chunk(tokens.size(), /*span_of_interest=*/symmetry_context_span,
             *cached_features, &chunks)) {
    TC_LOG(ERROR) << "Could not chunk.";
    return click_indices;
  }

  CodepointSpan result = click_indices;
  for (const TokenSpan& chunk : chunks) {
    if (chunk.first <= click_pos && click_pos < chunk.second) {
      result = selection_feature_processor_->StripBoundaryCodepoints(
          context, TokenSpanToCodepointSpan(tokens, chunk));
      break;
    }
  }

  if (model_->selection_options()->strip_unpaired_brackets()) {
    const CodepointSpan stripped_result =
        StripUnpairedBrackets(context, result, *unilib_);
    if (stripped_result.first != stripped_result.second) {
      result = stripped_result;
    }
  }

  return result;
}

std::vector<std::pair<std::string, float>> TextClassifier::ClassifyText(
    const std::string& context, CodepointSpan selection_indices) const {
  if (!initialized_) {
    TC_LOG(ERROR) << "Not initialized";
    return {};
  }

  if (std::get<0>(selection_indices) >= std::get<1>(selection_indices)) {
    TC_VLOG(1) << "Trying to run ClassifyText with invalid indices: "
               << std::get<0>(selection_indices) << " "
               << std::get<1>(selection_indices);
    return {};
  }

  // Check whether any of the regular expressions match.
  const std::string selection_text =
      ExtractSelection(context, selection_indices);
  for (const CompiledRegexPattern& regex_pattern : regex_patterns_) {
    if (regex_pattern.pattern->Matches(selection_text)) {
      return {{regex_pattern.collection_name, 1.0}};
    }
  }

  const FeatureProcessorOptions_::BoundsSensitiveFeatures*
      bounds_sensitive_features =
          model_->classification_feature_options()->bounds_sensitive_features();

  std::vector<Token> tokens;
  classification_feature_processor_->TokenizeAndFindClick(
      context, selection_indices, &tokens, /*click_pos=*/nullptr);
  const TokenSpan selection_token_span =
      CodepointSpanToTokenSpan(tokens, selection_indices);

  if (selection_token_span.first == kInvalidIndex ||
      selection_token_span.second == kInvalidIndex) {
    return {};
  }

  // The extraction span is the selection span expanded to include a relevant
  // number of tokens outside of the bounds of the selection.
  const TokenSpan extraction_span = IntersectTokenSpans(
      ExpandTokenSpan(selection_token_span,
                      bounds_sensitive_features->num_tokens_before(),
                      bounds_sensitive_features->num_tokens_after()),
      {0, tokens.size()});

  std::unique_ptr<CachedFeatures> cached_features;
  if (!classification_feature_processor_->ExtractFeatures(
          tokens, extraction_span, embedding_executor_.get(),
          classification_feature_processor_->EmbeddingSize() +
              classification_feature_processor_->DenseFeaturesCount(),
          &cached_features)) {
    TC_LOG(ERROR) << "Could not extract features.";
    return {};
  }

  const std::vector<float> features =
      cached_features->Get(selection_token_span);

  TensorView<float> logits =
      classification_executor_->ComputeLogits(TensorView<float>(
          features.data(), {1, static_cast<int>(features.size())}));
  if (!logits.is_valid()) {
    TC_LOG(ERROR) << "Couldn't compute logits.";
    return {};
  }

  if (logits.dims() != 2 || logits.dim(0) != 1 ||
      logits.dim(1) != classification_feature_processor_->NumCollections()) {
    TC_LOG(ERROR) << "Mismatching output";
    return {};
  }

  const std::vector<float> scores =
      ComputeSoftmax(logits.data(), logits.dim(1));

  std::vector<std::pair<std::string, float>> result(scores.size());
  for (int i = 0; i < scores.size(); i++) {
    result[i] = {classification_feature_processor_->LabelToCollection(i),
                 scores[i]};
  }
  std::sort(result.begin(), result.end(),
            [](const std::pair<std::string, float>& a,
               const std::pair<std::string, float>& b) {
              return a.second > b.second;
            });

  // Phone class sanity check.
  if (result.begin()->first == kPhoneCollection) {
    const int digit_count = CountDigits(context, selection_indices);
    if (digit_count <
            model_->classification_options()->phone_min_num_digits() ||
        digit_count >
            model_->classification_options()->phone_max_num_digits()) {
      return {{kOtherCollection, 1.0}};
    }
  }

  return result;
}

std::vector<AnnotatedSpan> TextClassifier::Annotate(
    const std::string& context) const {
  const UnicodeText context_unicode = UTF8ToUnicodeText(context,
                                                        /*do_copy=*/false);

  std::vector<TokenSpan> chunks;
  for (const UnicodeTextRange& line :
       selection_feature_processor_->SplitContext(context_unicode)) {
    const std::string line_str =
        UnicodeText::UTF8Substring(line.first, line.second);

    std::vector<Token> tokens;
    selection_feature_processor_->TokenizeAndFindClick(
        line_str, {0, std::distance(line.first, line.second)}, &tokens,
        /*click_pos=*/nullptr);
    const TokenSpan full_line_span = {0, tokens.size()};

    std::unique_ptr<CachedFeatures> cached_features;
    if (!classification_feature_processor_->ExtractFeatures(
            tokens, full_line_span, embedding_executor_.get(),
            classification_feature_processor_->EmbeddingSize() +
                classification_feature_processor_->DenseFeaturesCount(),
            &cached_features)) {
      TC_LOG(ERROR) << "Could not extract features.";
      continue;
    }

    std::vector<TokenSpan> local_chunks;
    if (!Chunk(tokens.size(), /*span_of_interest=*/full_line_span,
               *cached_features, &local_chunks)) {
      TC_LOG(ERROR) << "Could not chunk.";
      continue;
    }

    const int offset = std::distance(context_unicode.begin(), line.first);
    for (const TokenSpan& chunk : local_chunks) {
      const CodepointSpan codepoint_span =
          selection_feature_processor_->StripBoundaryCodepoints(
              line_str, TokenSpanToCodepointSpan(tokens, chunk));
      chunks.push_back(
          {codepoint_span.first + offset, codepoint_span.second + offset});
    }
  }

  std::vector<AnnotatedSpan> result;
  for (const CodepointSpan& chunk : chunks) {
    result.emplace_back();
    result.back().span = chunk;
    result.back().classification = ClassifyText(context, chunk);
  }
  return result;
}

bool TextClassifier::Chunk(int num_tokens, const TokenSpan& span_of_interest,
                           const CachedFeatures& cached_features,
                           std::vector<TokenSpan>* chunks) const {
  const int max_selection_span =
      selection_feature_processor_->GetOptions()->max_selection_span();
  const int max_chunk_length = selection_feature_processor_->GetOptions()
                                       ->selection_reduced_output_space()
                                   ? max_selection_span + 1
                                   : 2 * max_selection_span + 1;

  struct ScoredChunk {
    bool operator<(const ScoredChunk& that) const { return score < that.score; }

    TokenSpan token_span;
    float score;
  };

  // The inference span is the span of interest expanded to include
  // max_selection_span tokens on either side, which is how far a selection can
  // stretch from the click.
  const TokenSpan inference_span = IntersectTokenSpans(
      ExpandTokenSpan(span_of_interest,
                      /*num_tokens_left=*/max_selection_span,
                      /*num_tokens_right=*/max_selection_span),
      {0, num_tokens});

  std::vector<ScoredChunk> scored_chunks;
  // Iterate over chunk candidates that:
  //   - Are contained in the inference span
  //   - Have a non-empty intersection with the span of interest
  //   - Are at least one token long
  //   - Are not longer than the maximum chunk length
  for (int start = inference_span.first; start < span_of_interest.second;
       ++start) {
    const int leftmost_end_index = std::max(start, span_of_interest.first) + 1;
    for (int end = leftmost_end_index;
         end <= inference_span.second && end - start <= max_chunk_length;
         ++end) {
      const std::vector<float> features = cached_features.Get({start, end});
      TensorView<float> logits =
          selection_executor_->ComputeLogits(TensorView<float>(
              features.data(), {1, static_cast<int>(features.size())}));

      if (!logits.is_valid()) {
        TC_LOG(ERROR) << "Couldn't compute logits.";
        return false;
      }

      if (logits.dims() != 2 || logits.dim(0) != 1 || logits.dim(1) != 1) {
        TC_LOG(ERROR) << "Mismatching output";
        return false;
      }

      scored_chunks.push_back(ScoredChunk{{start, end}, logits.data()[0]});
    }
  }

  std::sort(scored_chunks.rbegin(), scored_chunks.rend());

  // Traverse the candidate chunks from highest-scoring to lowest-scoring. Pick
  // them greedily as long as they do not overlap with any previously picked
  // chunks.
  std::vector<bool> token_used(TokenSpanSize(inference_span));
  chunks->clear();
  for (const ScoredChunk& scored_chunk : scored_chunks) {
    bool feasible = true;
    for (int i = scored_chunk.token_span.first;
         i < scored_chunk.token_span.second; ++i) {
      if (token_used[i - inference_span.first]) {
        feasible = false;
        break;
      }
    }

    if (!feasible) {
      continue;
    }

    for (int i = scored_chunk.token_span.first;
         i < scored_chunk.token_span.second; ++i) {
      token_used[i - inference_span.first] = true;
    }

    chunks->push_back(scored_chunk.token_span);
  }

  std::sort(chunks->begin(), chunks->end());

  return true;
}

const Model* ViewModel(const void* buffer, int size) {
  if (!buffer) {
    return nullptr;
  }

  return LoadAndVerifyModel(buffer, size);
}

}  // namespace libtextclassifier2
