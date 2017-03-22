# Copyright (C) 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Useful environment variables that can be set on the mmma command line, as
# <key>=<value> pairs:
#
# LIBTEXTCLASSIFIER_STRIP_OPTS: (optional) value for LOCAL_STRIP_MODULE (for all
#   modules we build).  NOT for prod builds.  Can be set to keep_symbols for
#   debug / treemap purposes.


LOCAL_PATH := $(call my-dir)

# Custom C/C++ compilation flags:
MY_LIBTEXTCLASSIFIER_CFLAGS := \
    -Wno-unused-parameter \
    -Wno-sign-compare \
    -Wno-missing-field-initializers \
    -Wno-ignored-qualifiers \
    -Wno-undefined-var-template \
    -fvisibility=hidden

# ------------------------
# libtextclassifier_protos
# ------------------------

include $(CLEAR_VARS)

LOCAL_MODULE := libtextclassifier_protos

LOCAL_STRIP_MODULE := $(LIBTEXTCLASSIFIER_STRIP_OPTS)

LOCAL_SRC_FILES := $(call all-proto-files-under, .)
LOCAL_SHARED_LIBRARIES := libprotobuf-cpp-lite

include $(BUILD_STATIC_LIBRARY)

# -----------------
# libtextclassifier
# -----------------

include $(CLEAR_VARS)
LOCAL_MODULE := libtextclassifier

proto_sources_dir := $(generated_sources_dir)

LOCAL_CPP_EXTENSION := .cc
LOCAL_CFLAGS += $(MY_LIBTEXTCLASSIFIER_CFLAGS)
LOCAL_STRIP_MODULE := $(LIBTEXTCLASSIFIER_STRIP_OPTS)

LOCAL_SRC_FILES := $(patsubst ./%,%, $(shell cd $(LOCAL_PATH); \
    find . -name "*.cc" -and -not -name ".*" -and -not -path "./tests/*"))
LOCAL_C_INCLUDES += .
LOCAL_C_INCLUDES += $(proto_sources_dir)/proto/external/libtextclassifier

LOCAL_STATIC_LIBRARIES += libtextclassifier_protos
LOCAL_SHARED_LIBRARIES += libprotobuf-cpp-lite
LOCAL_SHARED_LIBRARIES += liblog
LOCAL_REQUIRED_MODULES := textclassifier.langid.model
LOCAL_REQUIRED_MODULES += textclassifier.smartselection.en.model

include $(BUILD_SHARED_LIBRARY)

# -----------------------
# libtextclassifier_tests
# -----------------------

include $(CLEAR_VARS)

LOCAL_MODULE := libtextclassifier_tests
LOCAL_MODULE_TAGS := tests

LOCAL_CPP_EXTENSION := .cc
LOCAL_CFLAGS += $(MY_LIBTEXTCLASSIFIER_CFLAGS)
LOCAL_STRIP_MODULE := $(LIBTEXTCLASSIFIER_STRIP_OPTS)

LOCAL_TEST_DATA := $(call find-test-data-in-subdirs, $(LOCAL_PATH), *, tests/testdata)

LOCAL_CPPFLAGS_32 += -DTEST_DATA_DIR="\"/data/nativetest/libtextclassifier_tests/tests/testdata/\""
LOCAL_CPPFLAGS_64 += -DTEST_DATA_DIR="\"/data/nativetest64/libtextclassifier_tests/tests/testdata/\""

LOCAL_SRC_FILES := $(patsubst ./%,%, $(shell cd $(LOCAL_PATH); \
    find . -name "*.cc" -and -not -name ".*"))
LOCAL_C_INCLUDES += .
LOCAL_C_INCLUDES += $(proto_sources_dir)/proto/external/libtextclassifier

LOCAL_STATIC_LIBRARIES += libtextclassifier_protos libgmock
LOCAL_SHARED_LIBRARIES += libprotobuf-cpp-lite
LOCAL_SHARED_LIBRARIES += liblog

include $(BUILD_NATIVE_TEST)

# ------------
# LangId model
# ------------

include $(CLEAR_VARS)
LOCAL_MODULE        := textclassifier.langid.model
LOCAL_MODULE_CLASS  := ETC
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := google
LOCAL_SRC_FILES     := ./models/textclassifier.langid.model
LOCAL_MODULE_PATH   := $(TARGET_OUT_ETC)/textclassifier
include $(BUILD_PREBUILT)

# ----------------------
# Smart Selection models
# ----------------------

include $(CLEAR_VARS)
LOCAL_MODULE        := textclassifier.smartselection.en.model
LOCAL_MODULE_CLASS  := ETC
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := google
LOCAL_SRC_FILES     := ./models/textclassifier.smartselection.en.model
LOCAL_MODULE_PATH   := $(TARGET_OUT_ETC)/textclassifier
include $(BUILD_PREBUILT)
