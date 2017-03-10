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

JNIEXPORT jstring JNICALL
Java_android_view_textclassifier_SmartSelection_nativeClassifyText(
    JNIEnv* env, jobject thiz, jlong ptr, jstring context, jint selection_begin,
    jint selection_end);

JNIEXPORT void JNICALL
Java_android_view_textclassifier_SmartSelection_nativeClose(JNIEnv* env,
                                                            jobject thiz,
                                                            jlong ptr);

// LangId.
JNIEXPORT jlong JNICALL Java_android_view_textclassifier_LangId_nativeNew(
    JNIEnv* env, jobject thiz, jint fd);

JNIEXPORT jstring JNICALL
Java_android_view_textclassifier_LangId_nativeFindLanguage(JNIEnv* env,
                                                           jobject thiz,
                                                           jlong ptr,
                                                           jstring text);

JNIEXPORT void JNICALL Java_android_view_textclassifier_LangId_nativeClose(
    JNIEnv* env, jobject thiz, jlong ptr);

#ifdef __cplusplus
}
#endif

using libtextclassifier::TextClassificationModel;
using libtextclassifier::nlp_core::lang_id::LangId;

namespace {

std::string ToStlString(JNIEnv* env, jstring str) {
  const char* bytes = env->GetStringUTFChars(str, 0);
  const std::string s = bytes;
  env->ReleaseStringUTFChars(str, bytes);
  return s;
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

JNIEXPORT jstring JNICALL
Java_android_view_textclassifier_SmartSelection_nativeClassifyText(
    JNIEnv* env, jobject thiz, jlong ptr, jstring context, jint selection_begin,
    jint selection_end) {
  TextClassificationModel* model =
      reinterpret_cast<TextClassificationModel*>(ptr);
  const std::string classification = model->ClassifyText(
      ToStlString(env, context), {selection_begin, selection_end});
  return env->NewStringUTF(classification.c_str());
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
Java_android_view_textclassifier_LangId_nativeFindLanguage(JNIEnv* env,
                                                           jobject thiz,
                                                           jlong ptr,
                                                           jstring text) {
  LangId* lang_id = reinterpret_cast<LangId*>(ptr);
  const std::string detected_language =
      lang_id->FindLanguage(ToStlString(env, text));
  jstring result = env->NewStringUTF(detected_language.c_str());
  return result;
}

JNIEXPORT void JNICALL Java_android_view_textclassifier_LangId_nativeClose(
    JNIEnv* env, jobject thiz, jlong ptr) {
  LangId* lang_id = reinterpret_cast<LangId*>(ptr);
  delete lang_id;
}
