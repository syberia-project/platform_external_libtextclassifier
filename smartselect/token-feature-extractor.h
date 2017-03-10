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

#ifndef LIBTEXTCLASSIFIER_SMARTSELECT_TOKEN_FEATURE_EXTRACTOR_H_
#define LIBTEXTCLASSIFIER_SMARTSELECT_TOKEN_FEATURE_EXTRACTOR_H_

#include <vector>

#include "base.h"
#include "smartselect/types.h"

namespace libtextclassifier {

struct TokenFeatureExtractorOptions {
  // Number of buckets used for hashing charactergrams.
  int num_buckets;

  // Orders of charactergrams to extract. E.g., 2 means character bigrams, 3
  // character trigrams etc.
  std::vector<int> chargram_orders;

  // Whether to extract the token case feature.
  bool extract_case_feature;

  // Whether to extract the selection mask feature.
  bool extract_selection_mask_feature;
};

class TokenFeatureExtractor {
 public:
  explicit TokenFeatureExtractor(const TokenFeatureExtractorOptions& options)
      : options_(options) {}

  // Extracts features from a token.
  //  - sparse_features are indices into a sparse feature vector of size
  //    options.num_buckets which are set to 1.0 (others are implicitly 0.0).
  //  - dense_features are values of a dense feature vector of size 0-2
  //    (depending on the options) for the token
  bool Extract(const Token& token, std::vector<int>* sparse_features,
               std::vector<float>* dense_features) const;

  // Convenience method that sequentially applies Extract to each Token.
  bool Extract(const std::vector<Token>& tokens,
               std::vector<std::vector<int>>* sparse_features,
               std::vector<std::vector<float>>* dense_features) const;

 protected:
  // Hashes given token to given number of buckets.
  int HashToken(const std::string& token) const;

  // Extracts the charactergram features from the token.
  std::vector<int> ExtractCharactergramFeatures(const Token& token) const;

 private:
  TokenFeatureExtractorOptions options_;
};

}  // namespace libtextclassifier

#endif  // LIBTEXTCLASSIFIER_SMARTSELECT_TOKEN_FEATURE_EXTRACTOR_H_
