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

#include "regex-base.h"

namespace libtextclassifier2 {

Rules::Rules(const std::string &locale) : locale_(locale) {}

FlatBufferRules::FlatBufferRules(const std::string &locale, const Model *model)
    : Rules(locale), model_(model), rule_cache_() {
  for (int i = 0; i < model_->regex_model()->patterns()->Length(); i++) {
    auto regex_model = model_->regex_model()->patterns()->Get(i);
    for (int j = 0; j < regex_model->node_names()->Length(); j++) {
      std::string name = regex_model->node_names()->Get(j)->str();
      rule_cache_[name].push_back(regex_model->pattern());
    }
  }
}

bool FlatBufferRules::RuleForName(const std::string &name,
                                  std::string *out) const {
  const auto match = rule_cache_.find(name);
  if (match != rule_cache_.end()) {
    if (match->second.size() != 1) {
      TC_LOG(ERROR) << "Found " << match->second.size()
                    << " rule where only 1 was expected.";
      return false;
    }
    *out = match->second[0]->str();
    return true;
  }
  return false;
}

const std::vector<std::string> FlatBufferRules::RulesForName(
    const std::string &name) const {
  std::vector<std::string> results;
  const auto match = rule_cache_.find(name);
  if (match != rule_cache_.end()) {
    for (auto &s : match->second) {
      results.push_back(s->str());
    }
  }
  return results;
}

}  // namespace libtextclassifier2
