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

#include "smartselect/feature-processor.h"

#include <iterator>
#include <set>
#include <vector>

#include "smartselect/text-classification-model.pb.h"
#include "util/base/logging.h"
#include "util/strings/utf8.h"
#include "util/utf8/unicodetext.h"

namespace libtextclassifier {

constexpr int kMaxWordLength = 20;  // All words will be trimmed to this length.

namespace internal {

TokenFeatureExtractorOptions BuildTokenFeatureExtractorOptions(
    const FeatureProcessorOptions& options) {
  TokenFeatureExtractorOptions extractor_options;

  extractor_options.num_buckets = options.num_buckets();
  for (int order : options.chargram_orders()) {
    extractor_options.chargram_orders.push_back(order);
  }
  extractor_options.extract_case_feature = options.extract_case_feature();
  extractor_options.extract_selection_mask_feature =
      options.extract_selection_mask_feature();

  return extractor_options;
}

FeatureProcessorOptions ParseSerializedOptions(
    const std::string& serialized_options) {
  FeatureProcessorOptions options;
  options.ParseFromString(serialized_options);
  return options;
}

void SplitTokensOnSelectionBoundaries(CodepointSpan selection,
                                      std::vector<Token>* tokens) {
  for (auto it = tokens->begin(); it != tokens->end(); ++it) {
    const UnicodeText token_word =
        UTF8ToUnicodeText(it->value, /*do_copy=*/false);

    auto last_start = token_word.begin();
    int last_start_index = it->start;
    std::vector<UnicodeText::const_iterator> split_points;

    // Selection start split point.
    if (selection.first > it->start && selection.first < it->end) {
      std::advance(last_start, selection.first - last_start_index);
      split_points.push_back(last_start);
      last_start_index = selection.first;
    }

    // Selection end split point.
    if (selection.second > it->start && selection.second < it->end) {
      std::advance(last_start, selection.second - last_start_index);
      split_points.push_back(last_start);
    }

    if (!split_points.empty()) {
      // Add a final split for the rest of the token unless it's been all
      // consumed already.
      if (split_points.back() != token_word.end()) {
        split_points.push_back(token_word.end());
      }

      std::vector<Token> replacement_tokens;
      last_start = token_word.begin();
      int current_pos = it->start;
      for (const auto& split_point : split_points) {
        Token new_token(token_word.UTF8Substring(last_start, split_point),
                        current_pos,
                        current_pos + std::distance(last_start, split_point),
                        /*is_in_span=*/false);

        last_start = split_point;
        current_pos = new_token.end;

        replacement_tokens.push_back(new_token);
      }

      it = tokens->erase(it);
      it = tokens->insert(it, replacement_tokens.begin(),
                          replacement_tokens.end());
      std::advance(it, replacement_tokens.size() - 1);
    }
  }
}

void FindSubstrings(const UnicodeText& t, const std::set<char32>& codepoints,
                    std::vector<UnicodeTextRange>* ranges) {
  UnicodeText::const_iterator start = t.begin();
  UnicodeText::const_iterator curr = start;
  UnicodeText::const_iterator end = t.end();
  for (; curr != end; ++curr) {
    if (codepoints.find(*curr) != codepoints.end()) {
      if (start != curr) {
        ranges->push_back(std::make_pair(start, curr));
      }
      start = curr;
      ++start;
    }
  }
  if (start != end) {
    ranges->push_back(std::make_pair(start, end));
  }
}

void StripTokensFromOtherLines(const std::string& context, CodepointSpan span,
                               std::vector<Token>* tokens) {
  const UnicodeText context_unicode = UTF8ToUnicodeText(context,
                                                        /*do_copy=*/false);
  std::vector<UnicodeTextRange> lines;
  std::set<char32> codepoints;
  codepoints.insert('\n');
  codepoints.insert('|');
  internal::FindSubstrings(context_unicode, codepoints, &lines);

  auto span_start = context_unicode.begin();
  if (span.first > 0) {
    std::advance(span_start, span.first);
  }
  auto span_end = context_unicode.begin();
  if (span.second > 0) {
    std::advance(span_end, span.second);
  }
  for (const UnicodeTextRange& line : lines) {
    // Find the line that completely contains the span.
    if (line.first <= span_start && line.second >= span_end) {
      const CodepointIndex last_line_begin_index =
          std::distance(context_unicode.begin(), line.first);
      const CodepointIndex last_line_end_index =
          last_line_begin_index + std::distance(line.first, line.second);

      for (auto token = tokens->begin(); token != tokens->end();) {
        if (token->start >= last_line_begin_index &&
            token->end <= last_line_end_index) {
          ++token;
        } else {
          token = tokens->erase(token);
        }
      }
    }
  }
}

}  // namespace internal

const char* const FeatureProcessor::kFeatureTypeName = "chargram_continuous";

std::vector<Token> FeatureProcessor::Tokenize(
    const std::string& utf8_text) const {
  return tokenizer_.Tokenize(utf8_text);
}

bool FeatureProcessor::LabelToSpan(
    const int label, const std::vector<Token>& tokens,
    std::pair<CodepointIndex, CodepointIndex>* span) const {
  if (tokens.size() != GetNumContextTokens()) {
    return false;
  }

  TokenSpan token_span;
  if (!LabelToTokenSpan(label, &token_span)) {
    return false;
  }

  const int result_begin_token = token_span.first;
  const int result_begin_codepoint =
      tokens[options_.context_size() - result_begin_token].start;
  const int result_end_token = token_span.second;
  const int result_end_codepoint =
      tokens[options_.context_size() + result_end_token].end;

  if (result_begin_codepoint == kInvalidIndex ||
      result_end_codepoint == kInvalidIndex) {
    *span = CodepointSpan({kInvalidIndex, kInvalidIndex});
  } else {
    *span = CodepointSpan({result_begin_codepoint, result_end_codepoint});
  }
  return true;
}

bool FeatureProcessor::LabelToTokenSpan(const int label,
                                        TokenSpan* token_span) const {
  if (label >= 0 && label < label_to_selection_.size()) {
    *token_span = label_to_selection_[label];
    return true;
  } else {
    return false;
  }
}

bool FeatureProcessor::SpanToLabel(
    const std::pair<CodepointIndex, CodepointIndex>& span,
    const std::vector<Token>& tokens, int* label) const {
  if (tokens.size() != GetNumContextTokens()) {
    return false;
  }

  const int click_position =
      options_.context_size();  // Click is always in the middle.
  const int padding = options_.context_size() - options_.max_selection_span();

  int span_left = 0;
  for (int i = click_position - 1; i >= padding; i--) {
    if (tokens[i].start != kInvalidIndex && tokens[i].start >= span.first) {
      ++span_left;
    } else {
      break;
    }
  }

  int span_right = 0;
  for (int i = click_position + 1; i < tokens.size() - padding; ++i) {
    if (tokens[i].end != kInvalidIndex && tokens[i].end <= span.second) {
      ++span_right;
    } else {
      break;
    }
  }

  // Check that the spanned tokens cover the whole span.
  if (tokens[click_position - span_left].start == span.first &&
      tokens[click_position + span_right].end == span.second) {
    *label = TokenSpanToLabel({span_left, span_right});
  } else {
    *label = kInvalidLabel;
  }

  return true;
}

int FeatureProcessor::TokenSpanToLabel(const TokenSpan& span) const {
  auto it = selection_to_label_.find(span);
  if (it != selection_to_label_.end()) {
    return it->second;
  } else {
    return kInvalidLabel;
  }
}

// Converts a codepoint span to a token span in the given list of tokens.
TokenSpan CodepointSpanToTokenSpan(const std::vector<Token>& selectable_tokens,
                                   CodepointSpan codepoint_span) {
  const int codepoint_start = std::get<0>(codepoint_span);
  const int codepoint_end = std::get<1>(codepoint_span);

  TokenIndex start_token = kInvalidIndex;
  TokenIndex end_token = kInvalidIndex;
  for (int i = 0; i < selectable_tokens.size(); ++i) {
    if (codepoint_start <= selectable_tokens[i].start &&
        codepoint_end >= selectable_tokens[i].end) {
      if (start_token == kInvalidIndex) {
        start_token = i;
      }
      end_token = i + 1;
    }
  }
  return {start_token, end_token};
}

namespace {

// Finds a single token that completely contains the given span.
int FindTokenThatContainsSpan(const std::vector<Token>& selectable_tokens,
                              CodepointSpan codepoint_span) {
  const int codepoint_start = std::get<0>(codepoint_span);
  const int codepoint_end = std::get<1>(codepoint_span);

  for (int i = 0; i < selectable_tokens.size(); ++i) {
    if (codepoint_start >= selectable_tokens[i].start &&
        codepoint_end <= selectable_tokens[i].end) {
      return i;
    }
  }
  return kInvalidIndex;
}

}  // namespace

namespace internal {

int CenterTokenFromClick(CodepointSpan span,
                         const std::vector<Token>& selectable_tokens) {
  int range_begin;
  int range_end;
  std::tie(range_begin, range_end) =
      CodepointSpanToTokenSpan(selectable_tokens, span);

  // If no exact match was found, try finding a token that completely contains
  // the click span. This is useful e.g. when Android builds the selection
  // using ICU tokenization, and ends up with only a portion of our space-
  // separated token. E.g. for "(857)" Android would select "857".
  if (range_begin == kInvalidIndex || range_end == kInvalidIndex) {
    int token_index = FindTokenThatContainsSpan(selectable_tokens, span);
    if (token_index != kInvalidIndex) {
      range_begin = token_index;
      range_end = token_index + 1;
    }
  }

  // We only allow clicks that are exactly 1 selectable token.
  if (range_end - range_begin == 1) {
    return range_begin;
  } else {
    return kInvalidIndex;
  }
}

int CenterTokenFromMiddleOfSelection(
    CodepointSpan span, const std::vector<Token>& selectable_tokens) {
  int range_begin;
  int range_end;
  std::tie(range_begin, range_end) =
      CodepointSpanToTokenSpan(selectable_tokens, span);

  // Center the clicked token in the selection range.
  if (range_begin != kInvalidIndex && range_end != kInvalidIndex) {
    return (range_begin + range_end - 1) / 2;
  } else {
    return kInvalidIndex;
  }
}

}  // namespace internal

int FeatureProcessor::FindCenterToken(CodepointSpan span,
                                      const std::vector<Token>& tokens) const {
  if (options_.center_token_selection_method() ==
      FeatureProcessorOptions::CENTER_TOKEN_FROM_CLICK) {
    return internal::CenterTokenFromClick(span, tokens);
  } else if (options_.center_token_selection_method() ==
             FeatureProcessorOptions::CENTER_TOKEN_MIDDLE_OF_SELECTION) {
    return internal::CenterTokenFromMiddleOfSelection(span, tokens);
  } else if (options_.center_token_selection_method() ==
             FeatureProcessorOptions::DEFAULT_CENTER_TOKEN_METHOD) {
    // TODO(zilka): This is a HACK not to break the current models. Remove once
    // we have new models on the device.
    // It uses the fact that sharing model use
    // split_tokens_on_selection_boundaries and selection not. So depending on
    // this we select the right way of finding the click location.
    if (!options_.split_tokens_on_selection_boundaries()) {
      // SmartSelection model.
      return internal::CenterTokenFromClick(span, tokens);
    } else {
      // SmartSharing model.
      return internal::CenterTokenFromMiddleOfSelection(span, tokens);
    }
  } else {
    TC_LOG(ERROR) << "Invalid center token selection method.";
    return kInvalidIndex;
  }
}

std::vector<Token> FeatureProcessor::FindTokensInSelection(
    const std::vector<Token>& selectable_tokens,
    const SelectionWithContext& selection_with_context) const {
  std::vector<Token> tokens_in_selection;
  for (const Token& token : selectable_tokens) {
    const bool selection_start_in_token =
        token.start <= selection_with_context.selection_start &&
        token.end > selection_with_context.selection_start;

    const bool token_contained_in_selection =
        token.start >= selection_with_context.selection_start &&
        token.end < selection_with_context.selection_end;

    const bool selection_end_in_token =
        token.start < selection_with_context.selection_end &&
        token.end >= selection_with_context.selection_end;

    if (selection_start_in_token || token_contained_in_selection ||
        selection_end_in_token) {
      tokens_in_selection.push_back(token);
    }
  }
  return tokens_in_selection;
}

CodepointSpan FeatureProcessor::ClickRandomTokenInSelection(
    const SelectionWithContext& selection_with_context) const {
  const std::vector<Token> tokens = Tokenize(selection_with_context.context);
  const std::vector<Token> tokens_in_selection =
      FindTokensInSelection(tokens, selection_with_context);

  if (!tokens_in_selection.empty()) {
    std::uniform_int_distribution<> selection_token_draw(
        0, tokens_in_selection.size() - 1);
    const int token_id = selection_token_draw(*random_);
    return {tokens_in_selection[token_id].start,
            tokens_in_selection[token_id].end};
  } else {
    return {kInvalidIndex, kInvalidIndex};
  }
}

bool FeatureProcessor::GetFeatures(
    const std::string& context, CodepointSpan input_span,
    std::vector<nlp_core::FeatureVector>* features,
    std::vector<float>* extra_features,
    std::vector<CodepointSpan>* selection_label_spans) const {
  return FeatureProcessor::GetFeaturesAndLabels(
      context, input_span, {kInvalidIndex, kInvalidIndex}, "", features,
      extra_features, selection_label_spans, /*selection_label=*/nullptr,
      /*selection_codepoint_label=*/nullptr, /*classification_label=*/nullptr);
}

bool FeatureProcessor::GetFeaturesAndLabels(
    const std::string& context, CodepointSpan input_span,
    CodepointSpan label_span, const std::string& label_collection,
    std::vector<nlp_core::FeatureVector>* features,
    std::vector<float>* extra_features,
    std::vector<CodepointSpan>* selection_label_spans, int* selection_label,
    CodepointSpan* selection_codepoint_label, int* classification_label) const {
  if (features == nullptr) {
    return false;
  }
  *features =
      std::vector<nlp_core::FeatureVector>(options_.context_size() * 2 + 1);

  std::vector<Token> input_tokens = Tokenize(context);

  if (options_.split_tokens_on_selection_boundaries()) {
    internal::SplitTokensOnSelectionBoundaries(input_span, &input_tokens);
  }

  if (options_.only_use_line_with_click()) {
    internal::StripTokensFromOtherLines(context, input_span, &input_tokens);
  }

  const int click_pos = FindCenterToken(input_span, input_tokens);
  if (click_pos == kInvalidIndex) {
    TC_LOG(ERROR) << "Could not extract click position.";
    return false;
  }

  std::vector<Token> output_tokens;
  bool status = ComputeFeatures(click_pos, input_tokens, input_span, features,
                                extra_features, &output_tokens);
  if (!status) {
    TC_LOG(ERROR) << "Feature computation failed.";
    return false;
  }

  if (selection_label != nullptr) {
    status = SpanToLabel(label_span, output_tokens, selection_label);
    if (!status) {
      TC_LOG(ERROR) << "Could not convert selection span to label.";
      return false;
    }
  }

  if (selection_codepoint_label != nullptr) {
    *selection_codepoint_label = label_span;
  }

  if (selection_label_spans != nullptr) {
    for (int i = 0; i < label_to_selection_.size(); ++i) {
      CodepointSpan span;
      status = LabelToSpan(i, output_tokens, &span);
      if (!status) {
        TC_LOG(ERROR) << "Could not convert label to span: " << i;
        return false;
      }
      selection_label_spans->push_back(span);
    }
  }

  if (classification_label != nullptr) {
    *classification_label = CollectionToLabel(label_collection);
  }

  return true;
}

bool FeatureProcessor::GetFeaturesAndLabels(
    const std::string& context, CodepointSpan input_span,
    CodepointSpan label_span, const std::string& label_collection,
    std::vector<std::vector<std::pair<int, float>>>* features,
    std::vector<float>* extra_features,
    std::vector<CodepointSpan>* selection_label_spans, int* selection_label,
    CodepointSpan* selection_codepoint_label, int* classification_label) const {
  if (features == nullptr) {
    return false;
  }
  if (extra_features == nullptr) {
    return false;
  }

  std::vector<nlp_core::FeatureVector> feature_vectors;
  bool result = GetFeaturesAndLabels(
      context, input_span, label_span, label_collection, &feature_vectors,
      extra_features, selection_label_spans, selection_label,
      selection_codepoint_label, classification_label);

  if (!result) {
    return false;
  }

  features->clear();
  for (int i = 0; i < feature_vectors.size(); ++i) {
    features->emplace_back();
    for (int j = 0; j < feature_vectors[i].size(); ++j) {
      nlp_core::FloatFeatureValue feature_value(feature_vectors[i].value(j));
      (*features)[i].push_back({feature_value.id, feature_value.weight});
    }
  }

  return true;
}

bool FeatureProcessor::ComputeFeatures(
    int click_pos, const std::vector<Token>& tokens,
    CodepointSpan selected_span, std::vector<nlp_core::FeatureVector>* features,
    std::vector<float>* extra_features,
    std::vector<Token>* output_tokens) const {
  int dropout_left = 0;
  int dropout_right = 0;
  if (options_.context_dropout_probability() > 0.0) {
    // Determine how much context to drop.
    bool status = GetContextDropoutRange(&dropout_left, &dropout_right);
    if (!status) {
      return false;
    }
  }

  int feature_index = 0;
  output_tokens->reserve(options_.context_size() * 2 + 1);
  const int num_extra_features =
      static_cast<int>(options_.extract_case_feature()) +
      static_cast<int>(options_.extract_selection_mask_feature());
  extra_features->reserve((options_.context_size() * 2 + 1) *
                          num_extra_features);
  for (int i = click_pos - options_.context_size();
       i <= click_pos + options_.context_size(); ++i, ++feature_index) {
    std::vector<int> sparse_features;
    std::vector<float> dense_features;

    const bool is_valid_token = i >= 0 && i < tokens.size();

    bool is_dropped = false;
    if (options_.context_dropout_probability() > 0.0) {
      if (i < click_pos - options_.context_size() + dropout_left) {
        is_dropped = true;
      } else if (i > click_pos + options_.context_size() - dropout_right) {
        is_dropped = true;
      }
    }

    if (is_valid_token && !is_dropped) {
      Token token(tokens[i]);
      token.is_in_span = token.start >= selected_span.first &&
                         token.end <= selected_span.second;
      feature_extractor_.Extract(token, &sparse_features, &dense_features);
      output_tokens->push_back(tokens[i]);
    } else {
      feature_extractor_.Extract(Token(), &sparse_features, &dense_features);
      // This adds an empty string for each missing context token to exactly
      // match the input tokens to the network.
      output_tokens->emplace_back();
    }

    for (int feature_id : sparse_features) {
      const int64 feature_value =
          nlp_core::FloatFeatureValue(feature_id, 1.0 / sparse_features.size())
              .discrete_value;
      (*features)[feature_index].add(
          const_cast<nlp_core::NumericFeatureType*>(&feature_type_),
          feature_value);
    }

    for (float value : dense_features) {
      extra_features->push_back(value);
    }
  }
  return true;
}

int FeatureProcessor::CollectionToLabel(const std::string& collection) const {
  const auto it = collection_to_label_.find(collection);
  if (it == collection_to_label_.end()) {
    return options_.default_collection();
  } else {
    return it->second;
  }
}

std::string FeatureProcessor::LabelToCollection(int label) const {
  if (label >= 0 && label < collection_to_label_.size()) {
    return options_.collections(label);
  } else {
    return GetDefaultCollection();
  }
}

void FeatureProcessor::MakeLabelMaps() {
  for (int i = 0; i < options_.collections().size(); ++i) {
    collection_to_label_[options_.collections(i)] = i;
  }

  int selection_label_id = 0;
  for (int l = 0; l < (options_.max_selection_span() + 1); ++l) {
    for (int r = 0; r < (options_.max_selection_span() + 1); ++r) {
      if (!options_.selection_reduced_output_space() ||
          r + l <= options_.max_selection_span()) {
        TokenSpan token_span{l, r};
        selection_to_label_[token_span] = selection_label_id;
        label_to_selection_.push_back(token_span);
        ++selection_label_id;
      }
    }
  }
}

bool FeatureProcessor::GetContextDropoutRange(int* dropout_left,
                                              int* dropout_right) const {
  std::uniform_real_distribution<> uniform01_draw(0, 1);
  if (uniform01_draw(*random_) < options_.context_dropout_probability()) {
    if (options_.use_variable_context_dropout()) {
      std::uniform_int_distribution<> uniform_context_draw(
          0, options_.context_size());
      // Select how much to drop in the range: [zero; context size]
      *dropout_left = uniform_context_draw(*random_);
      *dropout_right = uniform_context_draw(*random_);
    } else {
      // Drop all context.
      return false;
    }
  } else {
    *dropout_left = 0;
    *dropout_right = 0;
  }
  return true;
}

}  // namespace libtextclassifier
