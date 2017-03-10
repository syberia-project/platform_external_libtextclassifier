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

#include "smartselect/token-feature-extractor.h"

#include "util/hash/farmhash.h"

namespace libtextclassifier {

constexpr int kMaxWordLength = 20;  // All words will be trimmed to this length.

int TokenFeatureExtractor::HashToken(const std::string& token) const {
  return tcfarmhash::Fingerprint64(token) % options_.num_buckets;
}

std::vector<int> TokenFeatureExtractor::ExtractCharactergramFeatures(
    const Token& token) const {
  std::vector<int> result;
  if (token.is_padding) {
    result.push_back(HashToken("<PAD>"));
  } else {
    const std::string& word = token.value;
    std::string feature_word;

    // Trim words that are over kMaxWordLength characters.
    if (word.size() > kMaxWordLength) {
      feature_word =
          "^" + word.substr(0, kMaxWordLength / 2) + "\1" +
          word.substr(word.size() - kMaxWordLength / 2, kMaxWordLength / 2) +
          "$";
    } else {
      // Add a prefix and suffix to the word.
      feature_word = "^" + word + "$";
    }

    // Upper-bound the number of charactergram extracted to avoid resizing.
    result.reserve(options_.chargram_orders.size() * feature_word.size());

    // Generate the character-grams.
    for (int chargram_order : options_.chargram_orders) {
      if (chargram_order == 1) {
        for (int i = 1; i < feature_word.size() - 1; ++i) {
          result.push_back(HashToken(feature_word.substr(i, 1)));
        }
      } else {
        for (int i = 0;
             i < static_cast<int>(feature_word.size()) - chargram_order + 1;
             ++i) {
          result.push_back(HashToken(feature_word.substr(i, chargram_order)));
        }
      }
    }
  }
  return result;
}

bool TokenFeatureExtractor::Extract(const Token& token,
                                    std::vector<int>* sparse_features,
                                    std::vector<float>* dense_features) const {
  if (sparse_features == nullptr || dense_features == nullptr) {
    return false;
  }

  *sparse_features = ExtractCharactergramFeatures(token);

  if (options_.extract_case_feature) {
    // TODO(zilka): Make isupper Unicode-aware.
    if (!token.value.empty() && isupper(*token.value.begin())) {
      dense_features->push_back(1.0);
    } else {
      dense_features->push_back(-1.0);
    }
  }

  if (options_.extract_selection_mask_feature) {
    if (token.is_in_span) {
      dense_features->push_back(1.0);
    } else {
      // TODO(zilka): Switch to -1. Doing this now would break current models.
      dense_features->push_back(0.0);
    }
  }

  return true;
}

bool TokenFeatureExtractor::Extract(
    const std::vector<Token>& tokens,
    std::vector<std::vector<int>>* sparse_features,
    std::vector<std::vector<float>>* dense_features) const {
  if (sparse_features == nullptr || dense_features == nullptr) {
    return false;
  }

  sparse_features->resize(tokens.size());
  dense_features->resize(tokens.size());
  for (size_t i = 0; i < tokens.size(); i++) {
    if (!Extract(tokens[i], &((*sparse_features)[i]),
                 &((*dense_features)[i]))) {
      return false;
    }
  }
  return true;
}

}  // namespace libtextclassifier
