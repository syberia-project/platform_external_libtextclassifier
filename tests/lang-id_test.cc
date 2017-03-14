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

#include "lang_id/lang-id.h"

#include <memory>
#include <string>

#include "base.h"
#include "util/base/logging.h"
#include "gtest/gtest.h"

namespace libtextclassifier {
namespace nlp_core {
namespace lang_id {

namespace {

std::string GetModelPath() {
  return "tests/testdata/langid.model";
}

// Creates a LangId with default model.  Passes ownership to
// the caller.
LangId *CreateLanguageDetector() { return new LangId(GetModelPath()); }

}  // namespace

TEST(LangIdTest, Normal) {
  std::unique_ptr<LangId> lang_id(CreateLanguageDetector());

  EXPECT_EQ("en", lang_id->FindLanguage("Text written in English."));
  EXPECT_EQ("en", lang_id->FindLanguage("Text  written in   English.  "));
  EXPECT_EQ("en", lang_id->FindLanguage("  Text written in English.  "));
  EXPECT_EQ("fr", lang_id->FindLanguage("Vive la France!"));
  EXPECT_EQ("ro", lang_id->FindLanguage("Sunt foarte fericit!"));
}

namespace {
void CheckPredictionForGibberishStrings(const std::string &default_language) {
  static const char *const kGibberish[] = {
    "",
    " ",
    "       ",
    "  ___  ",
    "123 456 789",
    "><> (-_-) <><",
    nullptr,
  };

  std::unique_ptr<LangId> lang_id(CreateLanguageDetector());
  TC_LOG(INFO) << "Default language: " << default_language;
  lang_id->SetDefaultLanguage(default_language);
  for (int i = 0; true; ++i) {
    const char *gibberish = kGibberish[i];
    if (gibberish == nullptr) {
      break;
    }
    const std::string predicted_language = lang_id->FindLanguage(gibberish);
    TC_LOG(INFO) << "Predicted " << predicted_language << " for \"" << gibberish
                 << "\"";
    EXPECT_EQ(default_language, predicted_language);
  }
}
}  // namespace

TEST(LangIdTest, CornerCases) {
  CheckPredictionForGibberishStrings("en");
  CheckPredictionForGibberishStrings("ro");
  CheckPredictionForGibberishStrings("fr");
}

}  // namespace lang_id
}  // namespace nlp_core
}  // namespace libtextclassifier
