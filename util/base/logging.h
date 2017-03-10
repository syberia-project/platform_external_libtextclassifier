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
#ifndef LIBTEXTCLASSIFIER_UTIL_BASE_LOGGING_H_
#define LIBTEXTCLASSIFIER_UTIL_BASE_LOGGING_H_

#include <cassert>
#include <iostream>
#include <string>

namespace libtextclassifier {
namespace logging {

class LogMessage {
 public:
  explicit LogMessage(const std::string &type, const char *file_name,
                      int line_number)
      : fatal_(type == "FATAL") {
    std::cerr << type << " " << file_name << ":" << line_number << ": ";
  }

  ~LogMessage() {
    std::cerr << std::endl;
    if (fatal_) {
      exit(1);
    }
  }

  std::ostream &stream() { return std::cerr; }

 private:
  const bool fatal_;
};

#define TC_LOG(type) \
  ::libtextclassifier::logging::LogMessage(#type, __FILE__, __LINE__).stream()

// If condition x is true, does nothing.  Otherwise, crashes the program (liek
// LOG(FATAL)) with an informative message.  Can be continued with extra
// messages, via <<, like any logging macro, e.g.,
//
// TC_CHECK(my_cond) << "I think we hit a problem";
#define TC_CHECK(x)                                                           \
  (x) || TC_LOG(FATAL) << __FILE__ << ":" << __LINE__ << ": check failed: \"" \
                       << #x

#define TC_CHECK_EQ(x, y) TC_CHECK((x) == (y))
#define TC_CHECK_LT(x, y) TC_CHECK((x) < (y))
#define TC_CHECK_GT(x, y) TC_CHECK((x) > (y))
#define TC_CHECK_LE(x, y) TC_CHECK((x) <= (y))
#define TC_CHECK_GE(x, y) TC_CHECK((x) >= (y))
#define TC_CHECK_NE(x, y) TC_CHECK((x) != (y))

// Debug checks: a TC_DCHECK<suffix> macro should behave like TC_CHECK<suffix>
// in debug mode an don't check / don't print anything in non-debug mode.
#ifdef NDEBUG

// Pseudo-stream that "eats" the tokens <<-pumped into it, without printing
// anything.
class NullStream {
 public:
  NullStream() {}
  NullStream &stream() { return *this; }
};
template <typename T>
inline NullStream &operator<<(NullStream &str, const T &) {
  return str;
}

#define TC_NULLSTREAM ::libtextclassifier::logging::NullStream().stream()
#define TC_DCHECK(x) TC_NULLSTREAM
#define TC_DCHECK_EQ(x, y) TC_NULLSTREAM
#define TC_DCHECK_LT(x, y) TC_NULLSTREAM
#define TC_DCHECK_GT(x, y) TC_NULLSTREAM
#define TC_DCHECK_LE(x, y) TC_NULLSTREAM
#define TC_DCHECK_GE(x, y) TC_NULLSTREAM
#define TC_DCHECK_NE(x, y) TC_NULLSTREAM

#else  // NDEBUG

// In debug mode, each TC_DCHECK<suffix> is equivalent to TC_CHECK<suffix>,
// i.e., a real check that crashes when the condition is not true.
#define TC_DCHECK(x) TC_CHECK(x)
#define TC_DCHECK_EQ(x, y) TC_CHECK_EQ(x, y)
#define TC_DCHECK_LT(x, y) TC_CHECK_LT(x, y)
#define TC_DCHECK_GT(x, y) TC_CHECK_GT(x, y)
#define TC_DCHECK_LE(x, y) TC_CHECK_LE(x, y)
#define TC_DCHECK_GE(x, y) TC_CHECK_GE(x, y)
#define TC_DCHECK_NE(x, y) TC_CHECK_NE(x, y)

#endif  // NDEBUG
}  // namespace logging
}  // namespace libtextclassifier

#endif  // LIBTEXTCLASSIFIER_UTIL_BASE_LOGGING_H_
