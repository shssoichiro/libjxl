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
#include "jxl/quant_weights.h"

#include <stdlib.h>

#include <algorithm>
#include <cmath>
#include <hwy/base.h>  // HWY_ALIGN_MAX
#include <hwy/tests/test_util-inl.h>
#include <numeric>
#include <random>

#include "jxl/dct_for_test.h"
#include "jxl/dec_transforms.h"
#include "jxl/enc_modular.h"
#include "jxl/enc_transforms.h"

namespace jxl {
namespace {

template <typename T>
void CheckSimilar(T a, T b) {
  EXPECT_EQ(a, b);
}
// minimum exponent = -15.
template <>
void CheckSimilar(float a, float b) {
  float m = std::max(std::abs(a), std::abs(b));
  // 10 bits of precision are used in the format. Relative error should be
  // below 2^-10.
  EXPECT_LE(std::abs(a - b), m / 1024.0f) << "a: " << a << " b: " << b;
}

TEST(QuantWeightsTest, DC) {
  DequantMatrices mat;
  float dc_quant[3] = {1e+5, 1e+3, 1e+1};
  mat.SetCustomDC(dc_quant);
  for (size_t c = 0; c < 3; c++) {
    CheckSimilar(mat.InvDCQuant(c), dc_quant[c]);
  }
}

void RoundtripMatrices(const std::vector<QuantEncoding>& encodings) {
  ASSERT_TRUE(encodings.size() == DequantMatrices::kNum);
  DequantMatrices mat;
  ModularFrameEncoder encoder(FrameDimensions{}, FrameHeader{},
                              CompressParams{});
  mat.SetCustom(encodings, &encoder);
  const std::vector<QuantEncoding>& encodings_dec = mat.encodings();
  for (size_t i = 0; i < encodings.size(); i++) {
    const QuantEncoding& e = encodings[i];
    const QuantEncoding& d = encodings_dec[i];
    // Check values roundtripped correctly.
    EXPECT_EQ(e.mode, d.mode);
    EXPECT_EQ(e.predefined, d.predefined);
    EXPECT_EQ(e.source, d.source);

    EXPECT_EQ(static_cast<uint64_t>(e.dct_params.num_distance_bands),
              static_cast<uint64_t>(d.dct_params.num_distance_bands));
    for (size_t c = 0; c < 3; c++) {
      for (size_t j = 0; j < DctQuantWeightParams::kMaxDistanceBands; j++) {
        CheckSimilar(e.dct_params.distance_bands[c][j],
                     d.dct_params.distance_bands[c][j]);
      }
    }

    if (e.mode == QuantEncoding::kQuantModeRAW) {
      EXPECT_FALSE(!e.qraw.qtable);
      EXPECT_FALSE(!d.qraw.qtable);
      EXPECT_EQ(e.qraw.qtable->size(), d.qraw.qtable->size());
      for (size_t j = 0; j < e.qraw.qtable->size(); j++) {
        EXPECT_EQ((*e.qraw.qtable)[j], (*d.qraw.qtable)[j]);
      }
      EXPECT_EQ(e.qraw.qtable_den_shift, d.qraw.qtable_den_shift);
    } else {
      // modes different than kQuantModeRAW use one of the other fields used
      // here, which all happen to be arrays of floats.
      for (size_t c = 0; c < 3; c++) {
        for (size_t j = 0; j < 3; j++) {
          CheckSimilar(e.idweights[c][j], d.idweights[c][j]);
        }
        for (size_t j = 0; j < 6; j++) {
          CheckSimilar(e.dct2weights[c][j], d.dct2weights[c][j]);
        }
        for (size_t j = 0; j < 2; j++) {
          CheckSimilar(e.dct4multipliers[c][j], d.dct4multipliers[c][j]);
        }
        CheckSimilar(e.dct4x8multipliers[c], d.dct4x8multipliers[c]);
        for (size_t j = 0; j < 9; j++) {
          CheckSimilar(e.afv_weights[c][j], d.afv_weights[c][j]);
        }
        for (size_t j = 0; j < DctQuantWeightParams::kMaxDistanceBands; j++) {
          CheckSimilar(e.dct_params_afv_4x4.distance_bands[c][j],
                       d.dct_params_afv_4x4.distance_bands[c][j]);
        }
      }
    }
  }
}

TEST(QuantWeightsTest, AllDefault) {
  std::vector<QuantEncoding> encodings(DequantMatrices::kNum,
                                       QuantEncoding::Library(0));
  RoundtripMatrices(encodings);
}

void TestSingleQuantMatrix(DequantMatrices::QuantTable kind) {
  std::vector<QuantEncoding> encodings(DequantMatrices::kNum,
                                       QuantEncoding::Library(0));
  encodings[kind] = DequantMatrices::Library()[kind];
  RoundtripMatrices(encodings);
}

// Ensure we can reasonably represent default quant tables.
TEST(QuantWeightsTest, DCT) { TestSingleQuantMatrix(DequantMatrices::DCT); }
TEST(QuantWeightsTest, IDENTITY) {
  TestSingleQuantMatrix(DequantMatrices::IDENTITY);
}
TEST(QuantWeightsTest, DCT2X2) {
  TestSingleQuantMatrix(DequantMatrices::DCT2X2);
}
TEST(QuantWeightsTest, DCT4X4) {
  TestSingleQuantMatrix(DequantMatrices::DCT4X4);
}
TEST(QuantWeightsTest, DCT16X16) {
  TestSingleQuantMatrix(DequantMatrices::DCT16X16);
}
TEST(QuantWeightsTest, DCT32X32) {
  TestSingleQuantMatrix(DequantMatrices::DCT32X32);
}
TEST(QuantWeightsTest, DCT8X16) {
  TestSingleQuantMatrix(DequantMatrices::DCT8X16);
}
TEST(QuantWeightsTest, DCT8X32) {
  TestSingleQuantMatrix(DequantMatrices::DCT8X32);
}
TEST(QuantWeightsTest, DCT16X32) {
  TestSingleQuantMatrix(DequantMatrices::DCT16X32);
}
TEST(QuantWeightsTest, DCT4X8) {
  TestSingleQuantMatrix(DequantMatrices::DCT4X8);
}
TEST(QuantWeightsTest, AFV0) { TestSingleQuantMatrix(DequantMatrices::AFV0); }
TEST(QuantWeightsTest, RAW) {
  std::vector<QuantEncoding> encodings(DequantMatrices::kNum,
                                       QuantEncoding::Library(0));
  std::vector<int> matrix(3 * 32 * 32);
  std::mt19937 rng;
  std::uniform_int_distribution<size_t> dist(1, 255);
  for (size_t i = 0; i < matrix.size(); i++) matrix[i] = dist(rng);
  encodings[DequantMatrices::kQuantTable[AcStrategy::DCT32X32]] =
      QuantEncoding::RAW(matrix, 2);
  RoundtripMatrices(encodings);
}

class QuantWeightsTargetTest : public hwy::TestWithParamTarget {};
HWY_TARGET_INSTANTIATE_TEST_SUITE_P(QuantWeightsTargetTest);

TEST_P(QuantWeightsTargetTest, DCTUniform) {
  constexpr float kUniformQuant = 4;
  float weights[3][2] = {{1.0f / kUniformQuant, 0},
                         {1.0f / kUniformQuant, 0},
                         {1.0f / kUniformQuant, 0}};
  DctQuantWeightParams dct_params(weights);
  std::vector<QuantEncoding> encodings(DequantMatrices::kNum,
                                       QuantEncoding::DCT(dct_params));
  DequantMatrices dequant_matrices;
  ModularFrameEncoder encoder(FrameDimensions{}, FrameHeader{},
                              CompressParams{});
  dequant_matrices.SetCustom(encodings, &encoder);

  const float dc_quant[3] = {1.0f / kUniformQuant, 1.0f / kUniformQuant,
                             1.0f / kUniformQuant};
  dequant_matrices.SetCustomDC(dc_quant);

  // DCT8
  {
    HWY_ALIGN_MAX float pixels[64];
    std::iota(std::begin(pixels), std::end(pixels), 0);
    HWY_ALIGN_MAX float coeffs[64];
    const AcStrategy::Type dct = AcStrategy::DCT;
    TransformFromPixels(dct, pixels, 8, coeffs);
    HWY_ALIGN_MAX double slow_coeffs[64];
    for (size_t i = 0; i < 64; i++) slow_coeffs[i] = pixels[i];
    DCTSlow<8>(slow_coeffs);

    for (size_t i = 0; i < 64; i++) {
      // DCTSlow doesn't multiply/divide by 1/N, so we do it manually.
      slow_coeffs[i] =
          std::round(slow_coeffs[i] / 8 / kUniformQuant) * 8 * kUniformQuant;
      coeffs[i] =
          std::round(coeffs[i] * dequant_matrices.InvMatrix(dct, 0)[i]) *
          dequant_matrices.Matrix(dct, 0)[i];
    }
    IDCTSlow<8>(slow_coeffs);
    TransformToPixels(dct, coeffs, pixels, 8);
    for (size_t i = 0; i < 64; i++) {
      EXPECT_NEAR(pixels[i], slow_coeffs[i], 1e-4);
    }
  }

  // DCT16
  {
    HWY_ALIGN_MAX float pixels[64 * 4];
    std::iota(std::begin(pixels), std::end(pixels), 0);
    HWY_ALIGN_MAX float coeffs[64 * 4];
    const AcStrategy::Type dct = AcStrategy::DCT16X16;
    TransformFromPixels(dct, pixels, 16, coeffs);
    HWY_ALIGN_MAX double slow_coeffs[64 * 4];
    for (size_t i = 0; i < 64 * 4; i++) slow_coeffs[i] = pixels[i];
    DCTSlow<16>(slow_coeffs);

    for (size_t i = 0; i < 64 * 4; i++) {
      slow_coeffs[i] =
          std::round(slow_coeffs[i] / 16 / kUniformQuant) * 16 * kUniformQuant;
      coeffs[i] =
          std::round(coeffs[i] * dequant_matrices.InvMatrix(dct, 0)[i]) *
          dequant_matrices.Matrix(dct, 0)[i];
    }

    IDCTSlow<16>(slow_coeffs);
    TransformToPixels(dct, coeffs, pixels, 16);
    for (size_t i = 0; i < 64 * 4; i++) {
      EXPECT_NEAR(pixels[i], slow_coeffs[i], 1e-4);
    }
  }

  // Check that all matrices have the same DC quantization, i.e. that they all
  // have the same scaling.
  for (size_t i = 0; i < AcStrategy::kNumValidStrategies; i++) {
    EXPECT_NEAR(dequant_matrices.Matrix(i, 0)[0], kUniformQuant, 1e-6);
  }
}

}  // namespace
}  // namespace jxl