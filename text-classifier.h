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

#ifndef KNOWLEDGE_CEREBRA_SENSE_TEXT_CLASSIFIER_LIB2_TEXT_CLASSIFIER_H_
#define KNOWLEDGE_CEREBRA_SENSE_TEXT_CLASSIFIER_LIB2_TEXT_CLASSIFIER_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "feature-processor.h"
#include "model-executor.h"
#include "model_generated.h"
#include "strip-unpaired-brackets.h"
#include "types.h"
#include "util/memory/mmap.h"
#include "util/utf8/unilib.h"

namespace libtextclassifier2 {

// A text processing model that provides text classification, annotation,
// selection suggestion for various types.
// NOTE: This class is not thread-safe.
class TextClassifier {
 public:
  static std::unique_ptr<TextClassifier> FromUnownedBuffer(const char* buffer,
                                                           int size);
  // Takes ownership of the mmap.
  static std::unique_ptr<TextClassifier> FromScopedMmap(
      std::unique_ptr<ScopedMmap>* mmap);
  static std::unique_ptr<TextClassifier> FromFileDescriptor(int fd, int offset,
                                                            int size);
  static std::unique_ptr<TextClassifier> FromFileDescriptor(int fd);
  static std::unique_ptr<TextClassifier> FromPath(const std::string& path);

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
  CodepointSpan SuggestSelection(const std::string& context,
                                 CodepointSpan click_indices) const;

  // Classifies the selected text given the context string.
  // Returns an empty result if an error occurs.
  std::vector<std::pair<std::string, float>> ClassifyText(
      const std::string& context, CodepointSpan selection_indices) const;

  // Annotates given input text. The annotations should cover the whole input
  // context except for whitespaces, and are sorted by their position in the
  // context string.
  std::vector<AnnotatedSpan> Annotate(const std::string& context) const;

 protected:
  // Constructs and initializes text classifier from given model.
  // Takes ownership of 'mmap', and thus owns the buffer that backs 'model'.
  TextClassifier(std::unique_ptr<ScopedMmap>* mmap, const Model* model)
      : model_(model), mmap_(std::move(*mmap)), unilib_(new UniLib()) {
    ValidateAndInitialize();
  }

  // Constructs, validates and initializes text classifier from given model.
  // Does not own the buffer that backs 'model'.
  explicit TextClassifier(const Model* model)
      : model_(model), unilib_(new UniLib()) {
    ValidateAndInitialize();
  }

  // Checks that model contains all required fields, and initializes internal
  // datastructures.
  void ValidateAndInitialize();

  // Initializes regular expressions for the regex model.
  void InitializeRegexModel();

  // Groups the tokens into chunks. A chunk is a token span that should be the
  // suggested selection when any of its contained tokens is clicked. The chunks
  // are non-overlapping and are sorted by their position in the context string.
  // "num_tokens" is the total number of tokens available (as this method does
  // not need the actual vector of tokens).
  // "span_of_interest" is a span of all the tokens that could be clicked.
  // The resulting chunks all have to overlap with it and they cover this span
  // completely. The first and last chunk might extend beyond it.
  // The chunks vector is cleared before filling.
  bool Chunk(int num_tokens, const TokenSpan& span_of_interest,
             const CachedFeatures& cached_features,
             std::vector<TokenSpan>* chunks) const;

  // Collection name for other.
  const std::string kOtherCollection = "other";

  // Collection name for phone.
  const std::string kPhoneCollection = "phone";

  const Model* model_;

  std::unique_ptr<ModelExecutor> selection_executor_;
  std::unique_ptr<ModelExecutor> classification_executor_;
  std::unique_ptr<EmbeddingExecutor> embedding_executor_;

  std::unique_ptr<FeatureProcessor> selection_feature_processor_;
  std::unique_ptr<FeatureProcessor> classification_feature_processor_;

 private:
  struct CompiledRegexPattern {
    std::string collection_name;
    std::unique_ptr<UniLib::RegexPattern> pattern;
  };

  std::unique_ptr<ScopedMmap> mmap_;
  bool initialized_ = false;
  std::vector<CompiledRegexPattern> regex_patterns_;
  std::unique_ptr<UniLib> unilib_;
};

// Interprets the buffer as a Model flatbuffer and returns it for reading.
const Model* ViewModel(const void* buffer, int size);

}  // namespace libtextclassifier2

#endif  // KNOWLEDGE_CEREBRA_SENSE_TEXT_CLASSIFIER_LIB2_TEXT_CLASSIFIER_H_
