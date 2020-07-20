// Copyright (c) the JPEG XL Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef JXL_NOISE_H_
#define JXL_NOISE_H_

// Noise parameters shared by encoder/decoder.

#include <stddef.h>

#include <algorithm>
#include <utility>

#include "jxl/base/compiler_specific.h"

namespace jxl {

const float kNoisePrecision = 1 << 10;

struct NoiseParams {
  // LUT index is an intensity of pixel / mean intensity of patch
  static constexpr size_t kNumNoisePoints = 8;
  float lut[kNumNoisePoints];

  void Clear() {
    for (float& i : lut) i = 0;
  }
  bool HasAny() const {
    for (float i : lut) {
      if (i != 0.0f) return true;
    }
    return false;
  }
};

static inline std::pair<int, float> IndexAndFrac(float x) {
  // TODO: instead of 1, this should be a proper Y range.
  constexpr float kScale = (NoiseParams::kNumNoisePoints - 2) / 1;
  float scaled_x = std::max(0.f, x * kScale);
  size_t floor_x = static_cast<size_t>(scaled_x);
  if (JXL_UNLIKELY(floor_x > NoiseParams::kNumNoisePoints - 2)) {
    floor_x = NoiseParams::kNumNoisePoints - 2;
  }
  return std::make_pair(floor_x, scaled_x - floor_x);
}

struct NoiseLevel {
  float noise_level;
  float intensity;
};

}  // namespace jxl

#endif  // JXL_NOISE_H_