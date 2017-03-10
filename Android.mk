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

# ------------------------
# libtextclassifier_protos
# ------------------------

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := libtextclassifier_protos

LOCAL_SRC_FILES := $(call all-proto-files-under, .)
LOCAL_SHARED_LIBRARIES := libprotobuf-cpp-lite

include $(BUILD_STATIC_LIBRARY)

# -----------------
# libtextclassifier
# -----------------

include $(CLEAR_VARS)
LOCAL_MODULE := libtextclassifier

LOCAL_CPP_EXTENSION := .cc
LOCAL_CFLAGS += -Wno-unused-parameter \
    -Wno-sign-compare \
    -Wno-missing-field-initializers \
    -Wno-ignored-qualifiers \
    -Wno-undefined-var-template

LOCAL_SRC_FILES := $(patsubst ./%,%, $(shell cd $(LOCAL_PATH); \
    find . -name "*.cc" -and -not -name ".*"))
LOCAL_C_INCLUDES += .
LOCAL_C_INCLUDES += $(generated_sources_dir)/proto/external/libtextclassifier

LOCAL_SHARED_LIBRARIES := libprotobuf-cpp-lite
LOCAL_STATIC_LIBRARIES := libtextclassifier_protos

include $(BUILD_SHARED_LIBRARY)
