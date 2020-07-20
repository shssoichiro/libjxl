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

#ifndef JXL_EXTRAS_CODEC_EXR_H_
#define JXL_EXTRAS_CODEC_EXR_H_

// Encodes OpenEXR images in memory.

#include "jxl/base/data_parallel.h"
#include "jxl/base/padded_bytes.h"
#include "jxl/base/span.h"
#include "jxl/base/status.h"
#include "jxl/codec_in_out.h"
#include "jxl/color_encoding.h"

namespace jxl {

// Decodes `bytes` into `io`. io->dec_hints are ignored.
Status DecodeImageEXR(Span<const uint8_t> bytes, ThreadPool* pool,
                      CodecInOut* io);

// Transforms from io->c_current to `c_desired` (with the transfer function set
// to linear as that is the OpenEXR convention) and encodes into `bytes`.
Status EncodeImageEXR(const CodecInOut* io, const ColorEncoding& c_desired,
                      ThreadPool* pool, PaddedBytes* bytes);

}  // namespace jxl

#endif  // JXL_EXTRAS_CODEC_EXR_H_