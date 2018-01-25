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

// JNI wrapper for the TextClassifier.

#include "textclassifier_jni.h"

#include <jni.h>
#include <vector>

#include "text-classifier.h"
#include "util/java/scoped_local_ref.h"
#include "util/memory/mmap.h"

using libtextclassifier2::AnnotatedSpan;
using libtextclassifier2::Model;
using libtextclassifier2::TextClassifier;

namespace {

bool JStringToUtf8String(JNIEnv* env, const jstring& jstr,
                         std::string* result) {
  if (jstr == nullptr) {
    *result = std::string();
    return false;
  }

  jclass string_class = env->FindClass("java/lang/String");
  if (!string_class) {
    TC_LOG(ERROR) << "Can't find String class";
    return false;
  }

  jmethodID get_bytes_id =
      env->GetMethodID(string_class, "getBytes", "(Ljava/lang/String;)[B");

  jstring encoding = env->NewStringUTF("UTF-8");
  jbyteArray array = reinterpret_cast<jbyteArray>(
      env->CallObjectMethod(jstr, get_bytes_id, encoding));

  jbyte* const array_bytes = env->GetByteArrayElements(array, JNI_FALSE);
  int length = env->GetArrayLength(array);

  *result = std::string(reinterpret_cast<char*>(array_bytes), length);

  // Release the array.
  env->ReleaseByteArrayElements(array, array_bytes, JNI_ABORT);
  env->DeleteLocalRef(array);
  env->DeleteLocalRef(string_class);
  env->DeleteLocalRef(encoding);

  return true;
}

std::string ToStlString(JNIEnv* env, const jstring& str) {
  std::string result;
  JStringToUtf8String(env, str, &result);
  return result;
}

jobjectArray ScoredStringsToJObjectArray(
    JNIEnv* env, const std::string& result_class_name,
    const std::vector<std::pair<std::string, float>>& classification_result) {
  jclass result_class = env->FindClass(result_class_name.c_str());
  if (!result_class) {
    TC_LOG(ERROR) << "Couldn't find result class: " << result_class_name;
    return nullptr;
  }

  jmethodID result_class_constructor =
      env->GetMethodID(result_class, "<init>", "(Ljava/lang/String;F)V");

  jobjectArray results =
      env->NewObjectArray(classification_result.size(), result_class, nullptr);

  for (int i = 0; i < classification_result.size(); i++) {
    jstring row_string =
        env->NewStringUTF(classification_result[i].first.c_str());
    jobject result =
        env->NewObject(result_class, result_class_constructor, row_string,
                       static_cast<jfloat>(classification_result[i].second));
    env->SetObjectArrayElement(results, i, result);
    env->DeleteLocalRef(result);
  }
  env->DeleteLocalRef(result_class);
  return results;
}

}  // namespace

namespace libtextclassifier2 {

using libtextclassifier2::CodepointSpan;

namespace {

CodepointSpan ConvertIndicesBMPUTF8(const std::string& utf8_str,
                                    CodepointSpan orig_indices,
                                    bool from_utf8) {
  const libtextclassifier2::UnicodeText unicode_str =
      libtextclassifier2::UTF8ToUnicodeText(utf8_str, /*do_copy=*/false);

  int unicode_index = 0;
  int bmp_index = 0;

  const int* source_index;
  const int* target_index;
  if (from_utf8) {
    source_index = &unicode_index;
    target_index = &bmp_index;
  } else {
    source_index = &bmp_index;
    target_index = &unicode_index;
  }

  CodepointSpan result{-1, -1};
  std::function<void()> assign_indices_fn = [&result, &orig_indices,
                                             &source_index, &target_index]() {
    if (orig_indices.first == *source_index) {
      result.first = *target_index;
    }

    if (orig_indices.second == *source_index) {
      result.second = *target_index;
    }
  };

  for (auto it = unicode_str.begin(); it != unicode_str.end();
       ++it, ++unicode_index, ++bmp_index) {
    assign_indices_fn();

    // There is 1 extra character in the input for each UTF8 character > 0xFFFF.
    if (*it > 0xFFFF) {
      ++bmp_index;
    }
  }
  assign_indices_fn();

  return result;
}

}  // namespace

CodepointSpan ConvertIndicesBMPToUTF8(const std::string& utf8_str,
                                      CodepointSpan bmp_indices) {
  return ConvertIndicesBMPUTF8(utf8_str, bmp_indices, /*from_utf8=*/false);
}

CodepointSpan ConvertIndicesUTF8ToBMP(const std::string& utf8_str,
                                      CodepointSpan utf8_indices) {
  return ConvertIndicesBMPUTF8(utf8_str, utf8_indices, /*from_utf8=*/true);
}

}  // namespace libtextclassifier2

using libtextclassifier2::CodepointSpan;
using libtextclassifier2::ConvertIndicesBMPToUTF8;
using libtextclassifier2::ConvertIndicesUTF8ToBMP;
using libtextclassifier2::ScopedLocalRef;

JNI_METHOD(jlong, TC_CLASS_NAME, nativeNew)
(JNIEnv* env, jobject thiz, jint fd) {
  return reinterpret_cast<jlong>(
      TextClassifier::FromFileDescriptor(fd).release());
}

JNI_METHOD(jlong, TC_CLASS_NAME, nativeNewFromPath)
(JNIEnv* env, jobject thiz, jstring path) {
  const std::string path_str = ToStlString(env, path);
  return reinterpret_cast<jlong>(TextClassifier::FromPath(path_str).release());
}

JNI_METHOD(jlong, TC_CLASS_NAME, nativeNewFromAssetFileDescriptor)
(JNIEnv* env, jobject thiz, jobject afd, jlong offset, jlong size) {
  // Get system-level file descriptor from AssetFileDescriptor.
  ScopedLocalRef<jclass> afd_class(
      env->FindClass("android/content/res/AssetFileDescriptor"), env);
  if (afd_class == nullptr) {
    TC_LOG(ERROR) << "Couldn't find AssetFileDescriptor.";
    return reinterpret_cast<jlong>(nullptr);
  }
  jmethodID afd_class_getFileDescriptor = env->GetMethodID(
      afd_class.get(), "getFileDescriptor", "()Ljava/io/FileDescriptor;");
  if (afd_class_getFileDescriptor == nullptr) {
    TC_LOG(ERROR) << "Couldn't find getFileDescriptor.";
    return reinterpret_cast<jlong>(nullptr);
  }

  ScopedLocalRef<jclass> fd_class(env->FindClass("java/io/FileDescriptor"),
                                  env);
  if (fd_class == nullptr) {
    TC_LOG(ERROR) << "Couldn't find FileDescriptor.";
    return reinterpret_cast<jlong>(nullptr);
  }
  jfieldID fd_class_descriptor =
      env->GetFieldID(fd_class.get(), "descriptor", "I");
  if (fd_class_descriptor == nullptr) {
    TC_LOG(ERROR) << "Couldn't find descriptor.";
    return reinterpret_cast<jlong>(nullptr);
  }

  jobject bundle_jfd = env->CallObjectMethod(afd, afd_class_getFileDescriptor);
  jint bundle_cfd = env->GetIntField(bundle_jfd, fd_class_descriptor);

  return reinterpret_cast<jlong>(
      TextClassifier::FromFileDescriptor(bundle_cfd, offset, size).release());
}

JNI_METHOD(jintArray, TC_CLASS_NAME, nativeSuggest)
(JNIEnv* env, jobject thiz, jlong ptr, jstring context, jint selection_begin,
 jint selection_end) {
  if (!ptr) {
    return nullptr;
  }

  TextClassifier* model = reinterpret_cast<TextClassifier*>(ptr);

  const std::string context_utf8 = ToStlString(env, context);
  CodepointSpan input_indices =
      ConvertIndicesBMPToUTF8(context_utf8, {selection_begin, selection_end});
  CodepointSpan selection =
      model->SuggestSelection(context_utf8, input_indices);
  selection = ConvertIndicesUTF8ToBMP(context_utf8, selection);

  jintArray result = env->NewIntArray(2);
  env->SetIntArrayRegion(result, 0, 1, &(std::get<0>(selection)));
  env->SetIntArrayRegion(result, 1, 1, &(std::get<1>(selection)));
  return result;
}

JNI_METHOD(jobjectArray, TC_CLASS_NAME, nativeClassifyText)
(JNIEnv* env, jobject thiz, jlong ptr, jstring context, jint selection_begin,
 jint selection_end) {
  if (!ptr) {
    return nullptr;
  }
  TextClassifier* ff_model = reinterpret_cast<TextClassifier*>(ptr);
  const std::vector<std::pair<std::string, float>> classification_result =
      ff_model->ClassifyText(ToStlString(env, context),
                             {selection_begin, selection_end});

  return ScoredStringsToJObjectArray(
      env, TC_PACKAGE_PATH TC_CLASS_NAME_STR "$ClassificationResult",
      classification_result);
}

JNI_METHOD(jobjectArray, TC_CLASS_NAME, nativeAnnotate)
(JNIEnv* env, jobject thiz, jlong ptr, jstring context) {
  if (!ptr) {
    return nullptr;
  }
  TextClassifier* model = reinterpret_cast<TextClassifier*>(ptr);
  std::string context_utf8 = ToStlString(env, context);
  std::vector<AnnotatedSpan> annotations = model->Annotate(context_utf8);

  jclass result_class =
      env->FindClass(TC_PACKAGE_PATH TC_CLASS_NAME_STR "$AnnotatedSpan");
  if (!result_class) {
    TC_LOG(ERROR) << "Couldn't find result class: "
                  << TC_PACKAGE_PATH TC_CLASS_NAME_STR "$AnnotatedSpan";
    return nullptr;
  }

  jmethodID result_class_constructor = env->GetMethodID(
      result_class, "<init>",
      "(II[L" TC_PACKAGE_PATH TC_CLASS_NAME_STR "$ClassificationResult;)V");

  jobjectArray results =
      env->NewObjectArray(annotations.size(), result_class, nullptr);

  for (int i = 0; i < annotations.size(); ++i) {
    CodepointSpan span_bmp =
        ConvertIndicesUTF8ToBMP(context_utf8, annotations[i].span);
    jobject result = env->NewObject(
        result_class, result_class_constructor,
        static_cast<jint>(span_bmp.first), static_cast<jint>(span_bmp.second),
        ScoredStringsToJObjectArray(
            env, TC_PACKAGE_PATH TC_CLASS_NAME_STR "$ClassificationResult",
            annotations[i].classification));
    env->SetObjectArrayElement(results, i, result);
    env->DeleteLocalRef(result);
  }
  env->DeleteLocalRef(result_class);
  return results;
}

JNI_METHOD(void, TC_CLASS_NAME, nativeClose)
(JNIEnv* env, jobject thiz, jlong ptr) {
  TextClassifier* model = reinterpret_cast<TextClassifier*>(ptr);
  delete model;
}

JNI_METHOD(jstring, TC_CLASS_NAME, nativeGetLanguage)
(JNIEnv* env, jobject clazz, jint fd) {
  std::unique_ptr<libtextclassifier2::ScopedMmap> mmap(
      new libtextclassifier2::ScopedMmap(fd));
  if (!mmap->handle().ok()) {
    return env->NewStringUTF("");
  }
  const Model* model = libtextclassifier2::ViewModel(
      mmap->handle().start(), mmap->handle().num_bytes());
  if (!model || !model->language()) {
    return env->NewStringUTF("");
  }
  return env->NewStringUTF(model->language()->c_str());
}

JNI_METHOD(jint, TC_CLASS_NAME, nativeGetVersion)
(JNIEnv* env, jobject clazz, jint fd) {
  std::unique_ptr<libtextclassifier2::ScopedMmap> mmap(
      new libtextclassifier2::ScopedMmap(fd));
  if (!mmap->handle().ok()) {
    return 0;
  }
  const Model* model = libtextclassifier2::ViewModel(
      mmap->handle().start(), mmap->handle().num_bytes());
  if (!model) {
    return 0;
  }
  return model->version();
}
