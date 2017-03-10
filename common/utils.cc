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

#include "common/utils.h"

#include <ctype.h>
#include <stdlib.h>

namespace libtextclassifier {
namespace nlp_core {
namespace utils {

std::string TrimString(const std::string &text) {
  int start = 0;
  while ((start < text.size()) && isspace(text[start])) {
    start++;
  }
  int end = text.size() - 1;
  while ((end > start) && isspace(text[end])) {
    end--;
  }
  return text.substr(start, end - start + 1);
}

std::string Lowercase(const std::string &s) {
  std::string result(s);
  for (char &c : result) {
    c = tolower(c);
  }
  return result;
}

void NormalizeDigits(std::string *form) {
  for (size_t i = 0; i < form->size(); ++i) {
    if ((*form)[i] >= '0' && (*form)[i] <= '9') {
      (*form)[i] = '9';
    }
  }
}

}  // namespace utils
}  // namespace nlp_core
}  // namespace libtextclassifier
