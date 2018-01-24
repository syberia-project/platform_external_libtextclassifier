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

#ifndef KNOWLEDGE_CEREBRA_SENSE_TEXT_CLASSIFIER_LIB2_REGEX_BASE_H_
#define KNOWLEDGE_CEREBRA_SENSE_TEXT_CLASSIFIER_LIB2_REGEX_BASE_H_

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "model_generated.h"
#include "util/base/logging.h"
#include "util/memory/mmap.h"
#include "unicode/regex.h"
#include "unicode/uchar.h"

namespace libtextclassifier2 {

// Encapsulates the start and end of a region of a string of a entity that
// has been mapping to an element of type T
template <class T>
class SpanResult {
 private:
  const T data_;
  const int start_;
  const int end_;

 public:
  SpanResult(int start, int end, T data)
      : data_(data), start_(start), end_(end) {}

  const T &Data() const { return data_; }

  int Start() const { return start_; }

  int End() const { return end_; }
};

// Interface supplying class that provides a protocol for encapsulating a unit
// of text processing that can match against strings and also extract values of
// SpanResults of type T.  Implemenations are expected to be thread-safe.
template <class T>
class Node {
 public:
  typedef SpanResult<T> Result;
  typedef std::vector<Result> Results;

  // Returns a boolean value if Node can find a region of the string which
  // matches it's logic.
  virtual bool Matches(const std::string &input) const = 0;

  // Populates the supplied Results vector with the values obtained by
  // extracted on any matching elements of the string.
  // Returns true if the processing yielded no error.
  // Returns false if their was an error processing the string.
  virtual bool Extract(const std::string &input, Results *result) const = 0;

  virtual ~Node() = default;
};

class Rules {
 public:
  explicit Rules(const std::string &locale);
  virtual ~Rules() = default;
  virtual const std::vector<std::string> RulesForName(
      const std::string &name) const = 0;
  virtual bool RuleForName(const std::string &name, std::string *out) const = 0;

 protected:
  const std::string locale_;
};

class FlatBufferRules : public Rules {
 public:
  FlatBufferRules(const std::string &locale, const Model *model);
  const std::vector<std::string> RulesForName(
      const std::string &name) const override;
  bool RuleForName(const std::string &name, std::string *out) const override;

 protected:
  const Model *model_;
  std::unordered_map<std::string, std::vector<const flatbuffers::String *>>
      rule_cache_;
};

// Abstract supplying class that provides a protocol for encapsulating a unit
// of regular expression processing that can match against strings and also
// extract values of SpanResults of type T.  Implemenations are expected to be
// thread-safe.
// Implementors of this class are expected to call Init() prior to calling
// Extract() or Match().
template <class T>
class RegexNode : public Node<T> {
 public:
  typedef typename Node<T>::Result Result;
  typedef typename Node<T>::Results Results;

  RegexNode() : pattern_() {}

  // A string representation of the regular expression used in this processing.
  const std::string pattern() const {
    std::string regex;
    pattern_->pattern().toUTF8String(regex);
    return regex;
  }

  bool Matches(const std::string &input) const override {
    UErrorCode status = U_ZERO_ERROR;
    const icu::UnicodeString unicode_context(input.c_str(), input.size(),
                                             "utf-8");
    std::unique_ptr<icu::RegexMatcher> matcher(
        pattern_->matcher(unicode_context, status));

    if (U_FAILURE(status)) {
      TC_LOG(ERROR) << "failed to load regex '" << pattern()
                    << "': " << u_errorName(status);
      return false;
    }
    const bool res = matcher->find(status);
    if (U_FAILURE(status)) {
      TC_LOG(ERROR) << "failed to find with regex '" << pattern()
                    << "': " << u_errorName(status);
      return false;
    }
    return res;
  }

  bool Extract(const std::string &input, Results *result) const override = 0;
  ~RegexNode() override {}

 protected:
  bool Init(const std::string &regex) {
    UErrorCode status = U_ZERO_ERROR;
    pattern_ = std::unique_ptr<icu::RegexPattern>(
        icu::RegexPattern::compile(regex.c_str(), UREGEX_MULTILINE, status));
    if (U_FAILURE(status)) {
      TC_LOG(ERROR) << "failed to compile regex '" << pattern()
                    << "': " << u_errorName(status);
      return false;
    }
    return true;
  }

  std::unique_ptr<icu::RegexPattern> pattern_;
};

// Class that encapsulates complex object matching and extract values of
// from multiple sub-patterns, each of which is mapped to a string describing
// the value.
// Thread-safe.
template <class T>
class CompoundNode : public RegexNode<std::unordered_map<std::string, T>> {
 public:
  typedef std::unordered_map<std::string, std::unique_ptr<const Node<T>>>
      Extractors;
  typedef typename RegexNode<std::unordered_map<std::string, T>>::Result Result;
  typedef
      typename RegexNode<std::unordered_map<std::string, T>>::Results Results;

  static std::unique_ptr<CompoundNode<T>> Instance(const std::string &rule,
                                                   const Extractors &extractors,
                                                   const Rules &rules) {
    std::unique_ptr<CompoundNode<T>> node(new CompoundNode(extractors));
    if (!node->Init(rule, rules)) {
      return nullptr;
    }
    return node;
  }

  bool Extract(const std::string &input, Results *result) const override {
    UErrorCode status = U_ZERO_ERROR;
    const icu::UnicodeString unicode_context(input.c_str(), input.size(),
                                             "utf-8");
    std::unique_ptr<icu::RegexMatcher> matcher(
        RegexNode<std::unordered_map<std::string, T>>::pattern_->matcher(
            unicode_context, status));
    if (U_FAILURE(status)) {
      TC_LOG(ERROR) << "error loading regex '"
                    << RegexNode<std::unordered_map<std::string, T>>::pattern()
                    << "'" << u_errorName(status);
      return false;
    }
    while (matcher->find() && U_SUCCESS(status)) {
      const int start = matcher->start(status);
      if (U_FAILURE(status)) {
        TC_LOG(ERROR)
            << "failed to demarshall start '"
            << RegexNode<std::unordered_map<std::string, T>>::pattern() << "'"
            << u_errorName(status);
        return false;
      }

      const int end = matcher->end(status);
      if (U_FAILURE(status)) {
        TC_LOG(ERROR)
            << "failed to demarshall end '"
            << RegexNode<std::unordered_map<std::string, T>>::pattern() << "'"
            << u_errorName(status);
        return false;
      }

      std::unordered_map<std::string, T> extraction;
      for (auto name : groupnames_) {
        const int group_number = matcher->pattern().groupNumberFromName(
            icu::UnicodeString(name.c_str(), name.size(), "utf-8"), status);
        if (U_FAILURE(status)) {
          // We expect this to happen for optional named groups.
          continue;
        }

        std::string capture;
        matcher->group(group_number, status).toUTF8String(capture);
        std::vector<SpanResult<T>> sub_result;

        if (!extractors_->find(name)->second->Extract(capture, &sub_result)) {
          return false;
        }
        if (!sub_result.empty()) {
          extraction[name] = sub_result[0].Data();
        }
      }
      result->push_back(Result(start, end, extraction));
    }
    if (U_FAILURE(status)) {
      TC_LOG(ERROR) << "failed to extract '"
                    << RegexNode<std::unordered_map<std::string, T>>::pattern()
                    << "':" << u_errorName(status);
      return false;
    }
    return true;
  }

 private:
  const Extractors *extractors_;
  std::vector<std::string> groupnames_;

  explicit CompoundNode(const Extractors &extractors)
      : extractors_(&extractors), groupnames_() {}

  bool Init(const std::string &rule, const Rules &rules) {
    static const icu::RegexPattern *pattern = []() {
      UErrorCode status = U_ZERO_ERROR;
      return icu::RegexPattern::compile("[?]<([A-Z_]+)>", UREGEX_MULTILINE,
                                        status);
    }();
    if (!pattern) {
      return false;
    }
    std::string source = rule;
    UErrorCode status = U_ZERO_ERROR;
    const icu::UnicodeString unicode_context(source.c_str(), source.size(),
                                             "utf-8");
    std::unique_ptr<icu::RegexMatcher> matcher(
        pattern->matcher(unicode_context, status));

    std::unordered_map<std::string, std::string> swaps;
    while (matcher->find(status)) {
      std::string name;
      matcher->group(1, status).toUTF8String(name);
      if (U_FAILURE(status)) {
        TC_LOG(ERROR) << "failed to demarshall name " << u_errorName(status);
        return false;
      }
      groupnames_.push_back(name);
    }
    if (U_FAILURE(status)) {
      TC_LOG(ERROR) << "failed to execute the regex properly"
                    << u_errorName(status);
      return false;
    }
    for (auto swap : swaps) {
      std::string::size_type n = 0;
      while ((n = source.find(swap.first, n)) != std::string::npos) {
        source.replace(n, swap.first.size(), swap.second);
        n += swap.second.size();
      }
    }
    return RegexNode<std::unordered_map<std::string, T>>::Init(source);
  }
};

// Class that managed multiple alternate Nodes that all yield the same result
// and can be used interchangeably. It returns reults only from the first
// successfully matching child node - this Nodes can be given to in in
// order of precedence.
// Thread-safe.
template <class T>
class OrNode : public Node<T> {
 public:
  typedef typename Node<T>::Result Result;
  typedef typename Node<T>::Results Results;

  explicit OrNode(std::vector<std::unique_ptr<const Node<T>>> alternatives)
      : alternatives_(std::move(alternatives)) {}

  bool Extract(const std::string &input, Results *result) const override {
    for (auto &alternative : alternatives_) {
      typename Node<T>::Results alternative_result;
      // NOTE: We are explicitly choosing to fall through errors on these
      // alternatives to try a lesser match instead of bailing on the user.
      if (alternative->Extract(input, &alternative_result)) {
        if (!alternative_result.empty()) {
          for (auto &s : alternative_result) {
            result->push_back(s);
          }
          return true;
        }
      }
    }
    return true;
  }

  bool Matches(const std::string &input) const override {
    for (auto &alternative : alternatives_) {
      if (alternative->Matches(input)) {
        return true;
      }
    }
    return false;
  }

 private:
  std::vector<std::unique_ptr<const Node<T>>> alternatives_;
};

// Class that managed multiple alternate RegexNodes that all yield the same
// result and can be used interchangeably. It returns reults only from the first
// successfully matching child node - this Nodes can be given to in in
// order of precedence.
// Thread-safe.
template <class T>
class OrRegexNode : public RegexNode<T> {
 public:
  typedef typename RegexNode<T>::Result Result;
  typedef typename RegexNode<T>::Results Results;

  bool Extract(const std::string &input, Results *result) const override {
    for (auto &alternative : alternatives_) {
      typename RegexNode<T>::Results alternative_result;
      // NOTE: we are explicitly choosing to fall through errors on these
      // alternatives to try a lesser match instead of bailing on the user
      if (alternative->Extract(input, &alternative_result)) {
        if (!alternative_result.empty()) {
          for (typename RegexNode<T>::Result &s : alternative_result) {
            result->push_back(s);
          }
          return true;
        }
      }
    }
    return true;
  }

 protected:
  std::vector<std::unique_ptr<const RegexNode<T>>> alternatives_;

  explicit OrRegexNode(
      std::vector<std::unique_ptr<const RegexNode<T>>> alternatives)
      : alternatives_(std::move(alternatives)) {}

  bool Init() {
    std::string pattern;
    for (int i = 0; i < alternatives_.size(); i++) {
      if (i == 0) {
        pattern = alternatives_[i]->pattern();
      } else {
        pattern += "|";
        pattern += alternatives_[i]->pattern();
      }
    }
    return RegexNode<T>::Init(pattern);
  }
};

// Class that yields a constant value for any string that matches the input
// Thread-safe.
template <class T>
class MappingNode : public RegexNode<T> {
 public:
  typedef RegexNode<T> Parent;
  typedef typename Parent::Result Result;
  typedef typename Parent::Results Results;

  static std::unique_ptr<MappingNode<T>> Instance(const std::string &name,
                                                  const T &value,
                                                  const Rules &rules) {
    std::unique_ptr<MappingNode<T>> node(new MappingNode<T>(value));
    if (!node->Init(name, rules)) {
      return nullptr;
    }
    return node;
  }

  bool Extract(const std::string &input, Results *result) const override {
    UErrorCode status = U_ZERO_ERROR;

    const icu::UnicodeString unicode_context(input.c_str(), input.size(),
                                             "utf-8");
    std::unique_ptr<icu::RegexMatcher> matcher(
        Parent::pattern_->matcher(unicode_context, status));

    if (U_FAILURE(status)) {
      TC_LOG(ERROR) << "error loading regex ";
      return false;
    }

    while (matcher->find() && U_SUCCESS(status)) {
      const int start = matcher->start(status);
      if (U_FAILURE(status)) {
        TC_LOG(ERROR) << "failed to demarshall start " << u_errorName(status);
        return false;
      }
      const int end = matcher->end(status);
      if (U_FAILURE(status)) {
        TC_LOG(ERROR) << "failed to demarshall end " << u_errorName(status);
        return false;
      }
      result->push_back(Result(start, end, value_));
    }
    if (U_FAILURE(status)) {
      TC_LOG(ERROR) << "failed to demarshall end " << u_errorName(status);
      return false;
    }
    return true;
  }

 private:
  explicit MappingNode(const T value) : value_(value) {}

  const T value_;

  bool Init(const std::string &name, const Rules &rules) {
    std::string pattern;
    if (!rules.RuleForName(name, &pattern)) {
      TC_LOG(ERROR) << "failed to load rule for name '" << name << "'";
      return false;
    }
    return RegexNode<T>::Init(pattern);
  }
};

template <class T>
bool BuildMappings(const Rules &rules,
                   const std::vector<std::pair<std::string, T>> &pairs,
                   std::vector<std::unique_ptr<const RegexNode<T>>> *mappings) {
  for (auto &p : pairs) {
    if (std::unique_ptr<RegexNode<T>> node =
            MappingNode<T>::Instance(p.first, p.second, rules)) {
      mappings->emplace_back(std::move(node));
    } else {
      return false;
    }
  }
  return true;
}

}  // namespace libtextclassifier2
#endif  // KNOWLEDGE_CEREBRA_SENSE_TEXT_CLASSIFIER_LIB2_REGEX_BASE_H_
