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

#ifndef LIBTEXTCLASSIFIER_COMMON_EMBEDDING_FEATURE_EXTRACTOR_H_
#define LIBTEXTCLASSIFIER_COMMON_EMBEDDING_FEATURE_EXTRACTOR_H_

#include <memory>
#include <string>
#include <vector>

#include "common/feature-extractor.h"
#include "common/task-context.h"
#include "common/workspace.h"
#include "util/base/logging.h"
#include "util/base/macros.h"

namespace libtextclassifier {
namespace nlp_core {

// An EmbeddingFeatureExtractor manages the extraction of features for
// embedding-based models. It wraps a sequence of underlying classes of feature
// extractors, along with associated predicate maps. Each class of feature
// extractors is associated with a name, e.g., "words", "labels", "tags".
//
// The class is split between a generic abstract version,
// GenericEmbeddingFeatureExtractor (that can be initialized without knowing the
// signature of the ExtractFeatures method) and a typed version.
//
// The predicate maps must be initialized before use: they can be loaded using
// Read() or updated via UpdateMapsForExample.
class GenericEmbeddingFeatureExtractor {
 public:
  GenericEmbeddingFeatureExtractor() {}
  virtual ~GenericEmbeddingFeatureExtractor() {}

  // Get the prefix std::string to put in front of all arguments, so they don't
  // conflict with other embedding models.
  virtual const std::string ArgPrefix() const = 0;

  // Initializes predicate maps and embedding space names that are common for
  // all embedding-based feature extractors.
  virtual bool Init(TaskContext *context);

  // Requests workspace for the underlying feature extractors. This is
  // implemented in the typed class.
  virtual void RequestWorkspaces(WorkspaceRegistry *registry) = 0;

  // Returns number of embedding spaces.
  int NumEmbeddings() const { return embedding_dims_.size(); }

  // Number of predicates for the embedding at a given index (vocabulary size).
  // Returns -1 if index is out of bounds.
  int EmbeddingSize(int index) const {
    const GenericFeatureExtractor *extractor = generic_feature_extractor(index);
    return (extractor == nullptr) ? -1 : extractor->GetDomainSize();
  }

  // Returns the dimensionality of the embedding space.
  int EmbeddingDims(int index) const { return embedding_dims_[index]; }

  // Accessor for embedding dims (dimensions of the embedding spaces).
  const std::vector<int> &embedding_dims() const { return embedding_dims_; }

  const std::vector<std::string> &embedding_fml() const {
    return embedding_fml_;
  }

  // Get parameter name by concatenating the prefix and the original name.
  std::string GetParamName(const std::string &param_name) const {
    std::string full_name = ArgPrefix();
    full_name.push_back('_');
    full_name.append(param_name);
    return full_name;
  }

 protected:
  // Provides the generic class with access to the templated extractors. This is
  // used to get the type information out of the feature extractor without
  // knowing the specific calling arguments of the extractor itself.
  // Returns nullptr for an out-of-bounds idx.
  virtual const GenericFeatureExtractor *generic_feature_extractor(
      int idx) const = 0;

 private:
  // Embedding space names for parameter sharing.
  std::vector<std::string> embedding_names_;

  // FML strings for each feature extractor.
  std::vector<std::string> embedding_fml_;

  // Size of each of the embedding spaces (maximum predicate id).
  std::vector<int> embedding_sizes_;

  // Embedding dimensions of the embedding spaces (i.e. 32, 64 etc.)
  std::vector<int> embedding_dims_;

  TC_DISALLOW_COPY_AND_ASSIGN(GenericEmbeddingFeatureExtractor);
};

// Templated, object-specific implementation of the
// EmbeddingFeatureExtractor. EXTRACTOR should be a FeatureExtractor<OBJ,
// ARGS...> class that has the appropriate FeatureTraits() to ensure that
// locator type features work.
//
// Note: for backwards compatibility purposes, this always reads the FML spec
// from "<prefix>_features".
template <class EXTRACTOR, class OBJ, class... ARGS>
class EmbeddingFeatureExtractor : public GenericEmbeddingFeatureExtractor {
 public:
  // Initializes all predicate maps, feature extractors, etc.
  bool Init(TaskContext *context) override {
    if (!GenericEmbeddingFeatureExtractor::Init(context)) {
      return false;
    }
    feature_extractors_.resize(embedding_fml().size());
    for (int i = 0; i < embedding_fml().size(); ++i) {
      feature_extractors_[i].reset(new EXTRACTOR());
      if (!feature_extractors_[i]->Parse(embedding_fml()[i])) {
        return false;
      }
      if (!feature_extractors_[i]->Setup(context)) {
        return false;
      }
    }
    for (auto &feature_extractor : feature_extractors_) {
      if (!feature_extractor->Init(context)) {
        return false;
      }
    }
    return true;
  }

  // Requests workspaces from the registry. Must be called after Init(), and
  // before Preprocess().
  void RequestWorkspaces(WorkspaceRegistry *registry) override {
    for (auto &feature_extractor : feature_extractors_) {
      feature_extractor->RequestWorkspaces(registry);
    }
  }

  // Must be called on the object one state for each sentence, before any
  // feature extraction (e.g., UpdateMapsForExample, ExtractFeatures).
  void Preprocess(WorkspaceSet *workspaces, OBJ *obj) const {
    for (auto &feature_extractor : feature_extractors_) {
      feature_extractor->Preprocess(workspaces, obj);
    }
  }

  // Extracts features using the extractors. Note that features must already
  // be initialized to the correct number of feature extractors. No predicate
  // mapping is applied.
  void ExtractFeatures(const WorkspaceSet &workspaces, const OBJ &obj,
                       ARGS... args,
                       std::vector<FeatureVector> *features) const {
    TC_DCHECK(features != nullptr);
    TC_DCHECK_EQ(features->size(), feature_extractors_.size());
    for (int i = 0; i < feature_extractors_.size(); ++i) {
      (*features)[i].clear();
      feature_extractors_[i]->ExtractFeatures(workspaces, obj, args...,
                                              &(*features)[i]);
    }
  }

 protected:
  // Provides generic access to the feature extractors.
  const GenericFeatureExtractor *generic_feature_extractor(
      int idx) const override {
    if ((idx < 0) || (idx >= feature_extractors_.size())) {
      TC_LOG(ERROR) << "Out of bounds index " << idx;
      TC_DCHECK(false);  // Crash in debug mode.
      return nullptr;
    }
    return feature_extractors_[idx].get();
  }

 private:
  // Templated feature extractor class.
  std::vector<std::unique_ptr<EXTRACTOR>> feature_extractors_;
};

}  // namespace nlp_core
}  // namespace libtextclassifier

#endif  // LIBTEXTCLASSIFIER_COMMON_EMBEDDING_FEATURE_EXTRACTOR_H_
