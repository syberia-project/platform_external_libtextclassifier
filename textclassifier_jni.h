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

#ifndef KNOWLEDGE_CEREBRA_SENSE_TEXT_CLASSIFIER_LIB2_TEXTCLASSIFIER_JNI_H_
#define KNOWLEDGE_CEREBRA_SENSE_TEXT_CLASSIFIER_LIB2_TEXTCLASSIFIER_JNI_H_

#include <jni.h>
#include <string>

#include "types.h"

// When we use a macro as an argument for a macro, an additional level of
// indirection is needed, if the macro argument is used with # or ##.
#define ADD_QUOTES_HELPER(TOKEN) #TOKEN
#define ADD_QUOTES(TOKEN) ADD_QUOTES_HELPER(TOKEN)

#ifndef TC_PACKAGE_NAME
#define TC_PACKAGE_NAME android_view_textclassifier
#endif

#ifndef TC_CLASS_NAME
#define TC_CLASS_NAME SmartSelection
#endif
#define TC_CLASS_NAME_STR ADD_QUOTES(TC_CLASS_NAME)

#ifndef TC_PACKAGE_PATH
#define TC_PACKAGE_PATH "android/view/textclassifier/"
#endif

#define JNI_METHOD_PRIMITIVE(return_type, package_name, class_name, \
                             method_name)                           \
  JNIEXPORT return_type JNICALL                                     \
      Java_##package_name##_##class_name##_##method_name

// The indirection is needed to correctly expand the TC_PACKAGE_NAME macro.
// See the explanation near ADD_QUOTES macro.
#define JNI_METHOD2(return_type, package_name, class_name, method_name) \
  JNI_METHOD_PRIMITIVE(return_type, package_name, class_name, method_name)

#define JNI_METHOD(return_type, class_name, method_name) \
  JNI_METHOD2(return_type, TC_PACKAGE_NAME, class_name, method_name)

#ifdef __cplusplus
extern "C" {
#endif

// SmartSelection.
JNI_METHOD(jlong, TC_CLASS_NAME, nativeNew)
(JNIEnv* env, jobject thiz, jint fd);

JNI_METHOD(jlong, TC_CLASS_NAME, nativeNewFromPath)
(JNIEnv* env, jobject thiz, jstring path);

JNI_METHOD(jlong, TC_CLASS_NAME, nativeNewFromAssetFileDescriptor)
(JNIEnv* env, jobject thiz, jobject afd, jlong offset, jlong size);

JNI_METHOD(jintArray, TC_CLASS_NAME, nativeSuggest)
(JNIEnv* env, jobject thiz, jlong ptr, jstring context, jint selection_begin,
 jint selection_end);

JNI_METHOD(jobjectArray, TC_CLASS_NAME, nativeClassifyText)
(JNIEnv* env, jobject thiz, jlong ptr, jstring context, jint selection_begin,
 jint selection_end);

JNI_METHOD(jobjectArray, TC_CLASS_NAME, nativeAnnotate)
(JNIEnv* env, jobject thiz, jlong ptr, jstring context);

JNI_METHOD(void, TC_CLASS_NAME, nativeClose)
(JNIEnv* env, jobject thiz, jlong ptr);

JNI_METHOD(jstring, TC_CLASS_NAME, nativeGetLanguage)
(JNIEnv* env, jobject clazz, jint fd);

JNI_METHOD(jint, TC_CLASS_NAME, nativeGetVersion)
(JNIEnv* env, jobject clazz, jint fd);

#ifdef __cplusplus
}
#endif

namespace libtextclassifier2 {

// Given a utf8 string and a span expressed in Java BMP (basic multilingual
// plane) codepoints, converts it to a span expressed in utf8 codepoints.
libtextclassifier2::CodepointSpan ConvertIndicesBMPToUTF8(
    const std::string& utf8_str, libtextclassifier2::CodepointSpan bmp_indices);

// Given a utf8 string and a span expressed in utf8 codepoints, converts it to a
// span expressed in Java BMP (basic multilingual plane) codepoints.
libtextclassifier2::CodepointSpan ConvertIndicesUTF8ToBMP(
    const std::string& utf8_str,
    libtextclassifier2::CodepointSpan utf8_indices);

}  // namespace libtextclassifier2

#endif  // KNOWLEDGE_CEREBRA_SENSE_TEXT_CLASSIFIER_LIB2_TEXTCLASSIFIER_JNI_H_
