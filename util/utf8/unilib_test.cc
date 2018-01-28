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

#include "util/utf8/unilib.h"

#include "util/base/logging.h"

#include "gtest/gtest.h"

namespace libtextclassifier2 {
namespace {

TEST(UniLibTest, Interface) {
  UniLib unilib;
  TC_LOG(INFO) << unilib.IsOpeningBracket('(');
  TC_LOG(INFO) << unilib.IsClosingBracket(')');
  TC_LOG(INFO) << unilib.IsWhitespace(')');
  TC_LOG(INFO) << unilib.IsDigit(')');
  TC_LOG(INFO) << unilib.IsUpper(')');
  TC_LOG(INFO) << unilib.ToLower(')');
  TC_LOG(INFO) << unilib.GetPairedBracket(')');
  std::unique_ptr<UniLib::RegexPattern> pattern =
      unilib.CreateRegexPattern("[0-9]");
  TC_LOG(INFO) << pattern->Matches("Hello");
  std::unique_ptr<UniLib::BreakIterator> iterator =
      unilib.CreateBreakIterator("some text");
  TC_LOG(INFO) << iterator->Next();
  TC_LOG(INFO) << UniLib::BreakIterator::kDone;
}

}  // namespace
}  // namespace libtextclassifier2
