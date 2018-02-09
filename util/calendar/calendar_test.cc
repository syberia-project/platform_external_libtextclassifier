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

// This test serves the purpose of making sure all the different implementations
// of the unspoken CalendarLib interface support the same methods.

#include "util/calendar/calendar.h"
#include "util/base/logging.h"

#include "gtest/gtest.h"

namespace libtextclassifier2 {
namespace {

TEST(CalendarTest, Interface) {
  CalendarLib calendar;
  int64 time;
  std::string timezone;
  bool result = calendar.InterpretParseData(
      DateParseData{0l, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    static_cast<DateParseData::Relation>(0),
                    static_cast<DateParseData::RelationType>(0), 0},
      0L, "Zurich", &time);
  TC_LOG(INFO) << result;
}

}  // namespace
}  // namespace libtextclassifier2
