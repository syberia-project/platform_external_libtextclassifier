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

// Inference code for the text classification model.

#ifndef LIBTEXTCLASSIFIER_TEXT_CLASSIFIER_H_
#define LIBTEXTCLASSIFIER_TEXT_CLASSIFIER_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "datetime/parser.h"
#include "feature-processor.h"
#include "model-executor.h"
#include "model_generated.h"
#include "strip-unpaired-brackets.h"
#include "types.h"
#include "util/memory/mmap.h"
#include "util/utf8/unilib.h"

namespace libtextclassifier2 {

struct SelectionOptions {
  // Comma-separated list of locale specification for the input text (BCP 47
  // tags).
  std::string locales;

  static SelectionOptions Default() { return SelectionOptions(); }
};

struct ClassificationOptions {
  // For parsing relative datetimes, the reference now time against which the
  // relative datetimes get resolved.
  // UTC milliseconds since epoch.
  int64 reference_time_ms_utc = 0;

  // Timezone in which the input text was written (format as accepted by ICU).
  std::string reference_timezone;

  // Comma-separated list of locale specification for the input text (BCP 47
  // tags).
  std::string locales;

  static ClassificationOptions Default() { return ClassificationOptions(); }
};

struct AnnotationOptions {
  // For parsing relative datetimes, the reference now time against which the
  // relative datetimes get resolved.
  // UTC milliseconds since epoch.
  int64 reference_time_ms_utc = 0;

  // Timezone in which the input text was written (format as accepted by ICU).
  std::string reference_timezone;

  // Comma-separated list of locale specification for the input text (BCP 47
  // tags).
  std::string locales;

  static AnnotationOptions Default() { return AnnotationOptions(); }
};

// A text processing model that provides text classification, annotation,
// selection suggestion for various types.
// NOTE: This class is not thread-safe.
class TextClassifier {
 public:
  static std::unique_ptr<TextClassifier> FromUnownedBuffer(
      const char* buffer, int size, const UniLib* unilib = nullptr);
  // Takes ownership of the mmap.
  static std::unique_ptr<TextClassifier> FromScopedMmap(
      std::unique_ptr<ScopedMmap>* mmap, const UniLib* unilib = nullptr);
  static std::unique_ptr<TextClassifier> FromFileDescriptor(
      int fd, int offset, int size, const UniLib* unilib = nullptr);
  static std::unique_ptr<TextClassifier> FromFileDescriptor(
      int fd, const UniLib* unilib = nullptr);
  static std::unique_ptr<TextClassifier> FromPath(
      const std::string& path, const UniLib* unilib = nullptr);

  // Returns true if the model is ready for use.
  bool IsInitialized() { return initialized_; }

  // Runs inference for given a context and current selection (i.e. index
  // of the first and one past last selected characters (utf8 codepoint
  // offsets)). Returns the indices (utf8 codepoint offsets) of the selection
  // beginning character and one past selection end character.
  // Returns the original click_indices if an error occurs.
  // NOTE: The selection indices are passed in and returned in terms of
  // UTF8 codepoints (not bytes).
  // Requires that the model is a smart selection model.
  CodepointSpan SuggestSelection(
      const std::string& context, CodepointSpan click_indices,
      const SelectionOptions& options = SelectionOptions::Default()) const;

  // Classifies the selected text given the context string.
  // Returns an empty result if an error occurs.
  std::vector<ClassificationResult> ClassifyText(
      const std::string& context, CodepointSpan selection_indices,
      const ClassificationOptions& options =
          ClassificationOptions::Default()) const;

  // Annotates given input text. The annotations are sorted by their position
  // in the context string and exclude spans classified as 'other'.
  std::vector<AnnotatedSpan> Annotate(
      const std::string& context,
      const AnnotationOptions& options = AnnotationOptions::Default()) const;

  // Exposes the selection feature processor for tests and evaluations.
  const FeatureProcessor& SelectionFeatureProcessorForTests() const;

  // String collection names for various classes.
  static const std::string& kOtherCollection;
  static const std::string& kPhoneCollection;
  static const std::string& kDateCollection;

 protected:
  struct ScoredChunk {
    TokenSpan token_span;
    float score;
  };

  // Constructs and initializes text classifier from given model.
  // Takes ownership of 'mmap', and thus owns the buffer that backs 'model'.
  TextClassifier(std::unique_ptr<ScopedMmap>* mmap, const Model* model,
                 const UniLib* unilib)
      : model_(model),
        mmap_(std::move(*mmap)),
        owned_unilib_(nullptr),
        unilib_(internal::MaybeCreateUnilib(unilib, &owned_unilib_)) {
    ValidateAndInitialize();
  }

  // Constructs, validates and initializes text classifier from given model.
  // Does not own the buffer that backs 'model'.
  explicit TextClassifier(const Model* model, const UniLib* unilib)
      : model_(model),
        owned_unilib_(nullptr),
        unilib_(internal::MaybeCreateUnilib(unilib, &owned_unilib_)) {
    ValidateAndInitialize();
  }

  // Checks that model contains all required fields, and initializes internal
  // datastructures.
  void ValidateAndInitialize();

  // Initializes regular expressions for the regex model.
  bool InitializeRegexModel();

  // Resolves conflicts in the list of candidates by removing some overlapping
  // ones. Returns indices of the surviving ones.
  // NOTE: Assumes that the candidates are sorted according to their position in
  // the span.
  bool ResolveConflicts(const std::vector<AnnotatedSpan>& candidates,
                        const std::string& context,
                        std::vector<int>* result) const;

  // Resolves one conflict between candidates on indices 'start_index'
  // (inclusive) and 'end_index' (exclusive). Assigns the winning candidate
  // indices to 'chosen_indices'. Returns false if a problem arises.
  bool ResolveConflict(const std::string& context,
                       const std::vector<AnnotatedSpan>& candidates,
                       int start_index, int end_index,
                       std::vector<int>* chosen_indices) const;

  // Gets selection candidates from the ML model.
  bool ModelSuggestSelection(const UnicodeText& context_unicode,
                             CodepointSpan click_indices,
                             std::vector<AnnotatedSpan>* result) const;

  // Classifies the selected text given the context string with the
  // classification model.
  // Returns true if no error occurred.
  bool ModelClassifyText(
      const std::string& context, CodepointSpan selection_indices,
      std::vector<ClassificationResult>* classification_results) const;

  // Classifies the selected text with the regular expressions models.
  // Returns true if any regular expression matched and the result was set.
  bool RegexClassifyText(const std::string& context,
                         CodepointSpan selection_indices,
                         ClassificationResult* classification_result) const;

  // Classifies the selected text with the date time model.
  // Returns true if there was a match and the result was set.
  bool DatetimeClassifyText(const std::string& context,
                            CodepointSpan selection_indices,
                            const ClassificationOptions& options,
                            ClassificationResult* classification_result) const;

  // Chunks given input text with the selection model and classifies the spans
  // with the classification model.
  // The annotations are sorted by their position in the context string and
  // exclude spans classified as 'other'.
  bool ModelAnnotate(const std::string& context,
                     std::vector<AnnotatedSpan>* result) const;

  // Groups the tokens into chunks. A chunk is a token span that should be the
  // suggested selection when any of its contained tokens is clicked. The chunks
  // are non-overlapping and are sorted by their position in the context string.
  // "num_tokens" is the total number of tokens available (as this method does
  // not need the actual vector of tokens).
  // "span_of_interest" is a span of all the tokens that could be clicked.
  // The resulting chunks all have to overlap with it and they cover this span
  // completely. The first and last chunk might extend beyond it.
  // The chunks vector is cleared before filling.
  bool ModelChunk(int num_tokens, const TokenSpan& span_of_interest,
                  const CachedFeatures& cached_features,
                  std::vector<TokenSpan>* chunks) const;

  // A helper method for ModelChunk(). It generates scored chunk candidates for
  // a click context model.
  // NOTE: The returned chunks can (and most likely do) overlap.
  bool ModelClickContextScoreChunks(
      int num_tokens, const TokenSpan& span_of_interest,
      const CachedFeatures& cached_features,
      std::vector<ScoredChunk>* scored_chunks) const;

  // A helper method for ModelChunk(). It generates scored chunk candidates for
  // a bounds-sensitive model.
  // NOTE: The returned chunks can (and most likely do) overlap.
  bool ModelBoundsSensitiveScoreChunks(
      int num_tokens, const TokenSpan& span_of_interest,
      const TokenSpan& inference_span, const CachedFeatures& cached_features,
      std::vector<ScoredChunk>* scored_chunks) const;

  // Produces chunks isolated by a set of regular expressions.
  bool RegexChunk(const UnicodeText& context_unicode,
                  const std::vector<int>& rules,
                  std::vector<AnnotatedSpan>* result) const;

  // Produces chunks from the datetime parser.
  bool DatetimeChunk(const UnicodeText& context_unicode,
                     int64 reference_time_ms_utc,
                     const std::string& reference_timezone,
                     const std::string& locales,
                     std::vector<AnnotatedSpan>* result) const;

  const Model* model_;

  std::unique_ptr<ModelExecutor> selection_executor_;
  std::unique_ptr<ModelExecutor> classification_executor_;
  std::unique_ptr<EmbeddingExecutor> embedding_executor_;

  std::unique_ptr<FeatureProcessor> selection_feature_processor_;
  std::unique_ptr<FeatureProcessor> classification_feature_processor_;

  std::unique_ptr<DatetimeParser> datetime_parser_;

 private:
  struct CompiledRegexPattern {
    std::string collection_name;
    float target_classification_score;
    float priority_score;
    std::unique_ptr<UniLib::RegexPattern> pattern;
  };

  std::unique_ptr<ScopedMmap> mmap_;
  bool initialized_ = false;

  std::vector<CompiledRegexPattern> regex_patterns_;

  // Indices into regex_patterns_ for the different modes.
  std::vector<int> annotation_regex_patterns_, classification_regex_patterns_,
      selection_regex_patterns_;

  std::unique_ptr<UniLib> owned_unilib_;
  const UniLib* unilib_;
};

// Interprets the buffer as a Model flatbuffer and returns it for reading.
const Model* ViewModel(const void* buffer, int size);

}  // namespace libtextclassifier2

#endif  // LIBTEXTCLASSIFIER_TEXT_CLASSIFIER_H_
