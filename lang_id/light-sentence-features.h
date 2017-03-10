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

#ifndef LIBTEXTCLASSIFIER_LANG_ID_LIGH_SENTENCE_FEATURES_H_
#define LIBTEXTCLASSIFIER_LANG_ID_LIGH_SENTENCE_FEATURES_H_

#include "common/feature-extractor.h"
#include "lang_id/light-sentence.h"

namespace libtextclassifier {
namespace nlp_core {
namespace lang_id {

// Feature function that extracts features from LightSentences.
typedef FeatureFunction<LightSentence> LightSentenceFeature;

// Feature extractor for LightSentences.
typedef FeatureExtractor<LightSentence> LightSentenceExtractor;

// Macro to register features for LightSentences.
#define REGISTER_LIGHT_SENTENCE_FEATURE(name, type) \
  REGISTER_FEATURE_FUNCTION(LightSentenceFeature, name, type)

}  // namespace lang_id
}  // namespace nlp_core
}  // namespace libtextclassifier

#endif  // LIBTEXTCLASSIFIER_LANG_ID_LIGH_SENTENCE_FEATURES_H_
