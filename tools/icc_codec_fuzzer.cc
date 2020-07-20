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

#include "jxl/icc_codec.h"

namespace jxl {

int TestOneInput(const uint8_t* data, size_t size) {
#if defined(JXL_ICC_FUZZER_ONLY_WRITE)
  bool read = false;
#elif defined(JXL_ICC_FUZZER_ONLY_READ)
  bool read = true;
#else
  // Decide whether to test the reader or the writer (both use parsing)
  if (!size) return 0;
  bool read = data[0] == 0;
  data++;
  size--;
#endif

#ifdef JXL_ICC_FUZZER_SLOW_TEST
  // Including Brotli compression. Brotli is already fuzzed separately, so it is
  // better to disable JXL_ICC_FUZZER_SLOW_TEST to focus on the ICC parsing.
  if (read) {
    // Reading parses the compressed format.
    BitReader br(Span<const uint8_t>(data, size));
    PaddedBytes result;
    (void)ReadICC(&br, &result);
    (void)br.Close();
  } else {
    // Writing parses the original ICC profile.
    PaddedBytes icc;
    icc.assign(data, data + size);
    BitWriter writer;
    AuxOut aux;
    (void)WriteICC(icc, &writer, 0, &aux);
  }
#else  // JXL_ICC_FUZZER_SLOW_TEST
  if (read) {
    // Reading (unpredicting) parses the compressed format.
    PaddedBytes result;
    (void)UnpredictICC(data, size, &result);
  } else {
    // Writing (predicting) parses the original ICC profile.
    PaddedBytes result;
    (void)PredictICC(data, size, &result);
  }
#endif  // JXL_ICC_FUZZER_SLOW_TEST
  return 0;
}

}  // namespace jxl

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  return jxl::TestOneInput(data, size);
}