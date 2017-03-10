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

#ifndef LIBTEXTCLASSIFIER_COMMON_UTILS_H_
#define LIBTEXTCLASSIFIER_COMMON_UTILS_H_

#include <string>

#include "util/base/logging.h"

namespace libtextclassifier {
namespace nlp_core {
namespace utils {

// Returns a string which is like the parameter "text", but without leading and
// trailing whitespaces.
std::string TrimString(const std::string &text);

// Returns lower-cased version of s.
std::string Lowercase(const std::string &s);

void NormalizeDigits(std::string *form);

}  // namespace utils
}  // namespace nlp_core
}  // namespace libtextclassifier

#endif  // LIBTEXTCLASSIFIER_COMMON_UTILS_H_
