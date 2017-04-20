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

#ifndef LIBTEXTCLASSIFIER_COMMON_FLOAT16_H_
#define LIBTEXTCLASSIFIER_COMMON_FLOAT16_H_

#include "util/base/casts.h"
#include "util/base/integral_types.h"

namespace libtextclassifier {
namespace nlp_core {

// 16 bit encoding of a float.  NOTE: can't be used directly for computation:
// one first needs to convert it to a normal float, using Float16To32.
//
// Documentation copied from original file:
//
// Compact 16-bit encoding of floating point numbers. This
// representation uses 1 bit for the sign, 8 bits for the exponent and
// 7 bits for the mantissa.  It is assumed that floats are in IEEE 754
// format so a float16 is just bits 16-31 of a single precision float.
//
// NOTE: The IEEE floating point standard defines a float16 format that
// is different than this format (it has fewer bits of exponent and more
// bits of mantissa).  We don't use that format here because conversion
// to/from 32-bit floats is more complex for that format, and the
// conversion for this format is very simple.
//
// <---------float16------------>
// s e e e e e e e e f f f f f f f f f f f f f f f f f f f f f f f
// <------------------------------float-------------------------->
// 3 3             2 2             1 1                           0
// 1 0             3 2             5 4                           0

typedef uint16 float16;

static inline float16 Float32To16(float f) {
  // Note that we just truncate the mantissa bits: we make no effort to
  // do any smarter rounding.
  return (bit_cast<uint32>(f) >> 16) & 0xffff;
}

static inline float Float16To32(float16 f) {
  // We fill in the new mantissa bits with 0, and don't do anything smarter.
  return bit_cast<float>(f << 16);
}

}  // namespace nlp_core
}  // namespace libtextclassifier

#endif  // LIBTEXTCLASSIFIER_COMMON_FLOAT16_H_
