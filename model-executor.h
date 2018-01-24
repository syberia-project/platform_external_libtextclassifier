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

// Contains classes that can execute different models/parts of a model.

#ifndef KNOWLEDGE_CEREBRA_SENSE_TEXT_CLASSIFIER_LIB2_MODEL_EXECUTOR_H_
#define KNOWLEDGE_CEREBRA_SENSE_TEXT_CLASSIFIER_LIB2_MODEL_EXECUTOR_H_

#include <memory>

#include "tensor-view.h"
#include "types.h"
#include "util/base/logging.h"
#include "tensorflow/contrib/lite/interpreter.h"
#include "tensorflow/contrib/lite/kernels/register.h"
#include "tensorflow/contrib/lite/model.h"

namespace libtextclassifier2 {

namespace internal {
bool FromModelSpec(const tflite::Model* model_spec,
                   std::unique_ptr<tflite::FlatBufferModel>* model,
                   std::unique_ptr<tflite::Interpreter>* interpreter);
}  // namespace internal

// A helper function that given indices of feature and logits tensor, feature
// values computes the logits using given interpreter.
TensorView<float> ComputeLogitsHelper(const int input_index_features,
                                      const int output_index_logits,
                                      const TensorView<float>& features,
                                      tflite::Interpreter* interpreter);

// Executor for the text selection prediction and classification models.
// NOTE: This class is not thread-safe.
class ModelExecutor {
 public:
  explicit ModelExecutor(const tflite::Model* model_spec) {
    internal::FromModelSpec(model_spec, &model_, &interpreter_);
  }

  TensorView<float> ComputeLogits(const TensorView<float>& features) {
    return ComputeLogitsHelper(kInputIndexFeatures, kOutputIndexLogits,
                               features, interpreter_.get());
  }

 protected:
  static const int kInputIndexFeatures = 0;
  static const int kOutputIndexLogits = 0;

  std::unique_ptr<tflite::FlatBufferModel> model_ = nullptr;
  std::unique_ptr<tflite::Interpreter> interpreter_ = nullptr;
};

// Executor for embedding sparse features into a dense vector.
class EmbeddingExecutor {
 public:
  virtual ~EmbeddingExecutor() {}

  // Embeds the sparse_features into a dense embedding and adds (+) it
  // element-wise to the dest vector.
  virtual bool AddEmbedding(const TensorView<int>& sparse_features, float* dest,
                            int dest_size) = 0;

  // Returns true when the model is ready to be used, false otherwise.
  virtual bool IsReady() { return true; }
};

// NOTE: This class is not thread-safe.
class TFLiteEmbeddingExecutor : public EmbeddingExecutor {
 public:
  explicit TFLiteEmbeddingExecutor(const tflite::Model* model_spec);
  bool AddEmbedding(const TensorView<int>& sparse_features, float* dest,
                    int dest_size) override;

  bool IsReady() override { return initialized_; }

 protected:
  static const int kQuantBias = 128;
  bool initialized_ = false;
  int num_buckets_ = -1;
  int embedding_size_ = -1;
  const TfLiteTensor* scales_ = nullptr;
  const TfLiteTensor* embeddings_ = nullptr;

  std::unique_ptr<tflite::FlatBufferModel> model_ = nullptr;
  std::unique_ptr<tflite::Interpreter> interpreter_ = nullptr;
};

}  // namespace libtextclassifier2

#endif  // KNOWLEDGE_CEREBRA_SENSE_TEXT_CLASSIFIER_LIB2_MODEL_EXECUTOR_H_
