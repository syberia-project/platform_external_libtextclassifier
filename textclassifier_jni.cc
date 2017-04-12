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

// Simple JNI wrapper for the SmartSelection library.

#include <jni.h>
#include <vector>

#include "lang_id/lang-id.h"
#include "smartselect/text-classification-model.h"

#ifdef __cplusplus
extern "C" {
#endif

// SmartSelection.
JNIEXPORT jlong JNICALL
Java_android_view_textclassifier_SmartSelection_nativeNew(JNIEnv* env,
                                                          jobject thiz,
                                                          jint fd);

JNIEXPORT jintArray JNICALL
Java_android_view_textclassifier_SmartSelection_nativeSuggest(
    JNIEnv* env, jobject thiz, jlong ptr, jstring context, jint selection_begin,
    jint selection_end);

JNIEXPORT jobjectArray JNICALL
Java_android_view_textclassifier_SmartSelection_nativeClassifyText(
    JNIEnv* env, jobject thiz, jlong ptr, jstring context, jint selection_begin,
    jint selection_end, jint input_flags);

JNIEXPORT void JNICALL
Java_android_view_textclassifier_SmartSelection_nativeClose(JNIEnv* env,
                                                            jobject thiz,
                                                            jlong ptr);

JNIEXPORT jstring JNICALL
Java_android_view_textclassifier_SmartSelection_nativeGetLanguage(JNIEnv* env,
                                                                  jobject thiz,
                                                                  jint fd);

JNIEXPORT jint JNICALL
Java_android_view_textclassifier_SmartSelection_nativeGetVersion(JNIEnv* env,
                                                                 jobject thiz,
                                                                 jint fd);

// LangId.
JNIEXPORT jlong JNICALL Java_android_view_textclassifier_LangId_nativeNew(
    JNIEnv* env, jobject thiz, jint fd);

JNIEXPORT jobjectArray JNICALL
Java_android_view_textclassifier_LangId_nativeFindLanguages(JNIEnv* env,
                                                            jobject thiz,
                                                            jlong ptr,
                                                            jstring text);

JNIEXPORT void JNICALL Java_android_view_textclassifier_LangId_nativeClose(
    JNIEnv* env, jobject thiz, jlong ptr);

#ifdef __cplusplus
}
#endif

using libtextclassifier::TextClassificationModel;
using libtextclassifier::ModelOptions;
using libtextclassifier::nlp_core::lang_id::LangId;

namespace {

bool JStringToUtf8String(JNIEnv* env, const jstring& jstr,
                         std::string* result) {
  if (jstr == nullptr) {
    *result = std::string();
    return false;
  }

  jclass string_class = env->FindClass("java/lang/String");
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

JNIEXPORT jlong JNICALL
Java_android_view_textclassifier_SmartSelection_nativeNew(JNIEnv* env,
                                                          jobject thiz,
                                                          jint fd) {
  TextClassificationModel* model = new TextClassificationModel(fd);
  return reinterpret_cast<jlong>(model);
}

JNIEXPORT jintArray JNICALL
Java_android_view_textclassifier_SmartSelection_nativeSuggest(
    JNIEnv* env, jobject thiz, jlong ptr, jstring context, jint selection_begin,
    jint selection_end) {
  TextClassificationModel* model =
      reinterpret_cast<TextClassificationModel*>(ptr);

  const libtextclassifier::CodepointSpan selection = model->SuggestSelection(
      ToStlString(env, context), {selection_begin, selection_end});

  jintArray result = env->NewIntArray(2);
  env->SetIntArrayRegion(result, 0, 1, &(std::get<0>(selection)));
  env->SetIntArrayRegion(result, 1, 1, &(std::get<1>(selection)));
  return result;
}

JNIEXPORT jobjectArray JNICALL
Java_android_view_textclassifier_SmartSelection_nativeClassifyText(
    JNIEnv* env, jobject thiz, jlong ptr, jstring context, jint selection_begin,
    jint selection_end, jint input_flags) {
  TextClassificationModel* ff_model =
      reinterpret_cast<TextClassificationModel*>(ptr);
  const std::vector<std::pair<std::string, float>> classification_result =
      ff_model->ClassifyText(ToStlString(env, context),
                             {selection_begin, selection_end}, input_flags);

  return ScoredStringsToJObjectArray(
      env, "android/view/textclassifier/SmartSelection$ClassificationResult",
      classification_result);
}

JNIEXPORT void JNICALL
Java_android_view_textclassifier_SmartSelection_nativeClose(JNIEnv* env,
                                                            jobject thiz,
                                                            jlong ptr) {
  TextClassificationModel* model =
      reinterpret_cast<TextClassificationModel*>(ptr);
  delete model;
}

JNIEXPORT jlong JNICALL Java_android_view_textclassifier_LangId_nativeNew(
    JNIEnv* env, jobject thiz, jint fd) {
  return reinterpret_cast<jlong>(new LangId(fd));
}

JNIEXPORT jstring JNICALL
Java_android_view_textclassifier_SmartSelection_nativeGetLanguage(JNIEnv* env,
                                                                  jobject clazz,
                                                                  jint fd) {
  ModelOptions model_options;
  if (ReadSelectionModelOptions(fd, &model_options)) {
    return env->NewStringUTF(model_options.language().c_str());
  } else {
    return env->NewStringUTF("UNK");
  }
}

JNIEXPORT jint JNICALL
Java_android_view_textclassifier_SmartSelection_nativeGetVersion(JNIEnv* env,
                                                                 jobject clazz,
                                                                 jint fd) {
  ModelOptions model_options;
  if (ReadSelectionModelOptions(fd, &model_options)) {
    return model_options.version();
  } else {
    return -1;
  }
}

JNIEXPORT jobjectArray JNICALL
Java_android_view_textclassifier_LangId_nativeFindLanguages(JNIEnv* env,
                                                            jobject thiz,
                                                            jlong ptr,
                                                            jstring text) {
  LangId* lang_id = reinterpret_cast<LangId*>(ptr);
  const std::vector<std::pair<std::string, float>> scored_languages =
      lang_id->FindLanguages(ToStlString(env, text));

  return ScoredStringsToJObjectArray(
      env, "android/view/textclassifier/LangId$ClassificationResult",
      scored_languages);
}

JNIEXPORT void JNICALL Java_android_view_textclassifier_LangId_nativeClose(
    JNIEnv* env, jobject thiz, jlong ptr) {
  LangId* lang_id = reinterpret_cast<LangId*>(ptr);
  delete lang_id;
}
