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

#ifndef LIBTEXTCLASSIFIER_COMMON_CONFIG_H_
#define LIBTEXTCLASSIFIER_COMMON_CONFIG_H_

// Logic to define PORTABLE_SAFT_MOBILE.  This macro is not zero if and only if
// we build for something else than the google3 Linux desktop.  We need to
// identify that case in order to e.g., use the different base type for the
// portable protos (different than the base type of the google3 protos).
//
// This file is inspired by google3/base/base_config.h
//
// For sample use of this header and of PORTABLE_SAFT_MOBILE, see
// nlp/saft/components/common/mobile/base-proto.h

#ifndef PORTABLE_SAFT_MOBILE
#if defined(__ANDROID__) || defined(__APPLE__)
#define PORTABLE_SAFT_MOBILE 1
#else
#define PORTABLE_SAFT_MOBILE 0
#endif

#endif

#endif  // LIBTEXTCLASSIFIER_COMMON_CONFIG_H_
