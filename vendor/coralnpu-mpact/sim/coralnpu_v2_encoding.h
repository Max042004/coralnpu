// Copyright 2025 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SIM_CORALNPU_V2_ENCODING_H_
#define SIM_CORALNPU_V2_ENCODING_H_

#include <cstdint>

#include "sim/coralnpu_v2_bin_decoder.h"
#include "sim/coralnpu_v2_decoder.h"
#include "sim/coralnpu_v2_encoding_template.h"
#include "sim/coralnpu_v2_enums.h"
#include "sim/coralnpu_v2_state.h"

namespace coralnpu::sim {

class CoralNPUV2Encoding
    : public CoralNPUV2EncodingTemplate<
          ::coralnpu::sim::isa32_v2::CoralNPUV2EncodingBase,
          ::coralnpu::sim::isa32_v2::OpcodeEnum,
          ::coralnpu::sim::isa32_v2::SlotEnum,
          ::coralnpu::sim::isa32_v2::SourceOpEnum,
          ::coralnpu::sim::isa32_v2::DestOpEnum,
          ::coralnpu::sim::encoding::Extractors> {
 public:
  using Base = CoralNPUV2EncodingTemplate<
      ::coralnpu::sim::isa32_v2::CoralNPUV2EncodingBase,
      ::coralnpu::sim::isa32_v2::OpcodeEnum,
      ::coralnpu::sim::isa32_v2::SlotEnum,
      ::coralnpu::sim::isa32_v2::SourceOpEnum,
      ::coralnpu::sim::isa32_v2::DestOpEnum,
      ::coralnpu::sim::encoding::Extractors>;

  explicit CoralNPUV2Encoding(CoralNPUV2State* state) : Base(state) {}

  // Parses an instruction and determines the opcode.
  void ParseInstruction(uint32_t inst_word);
};

}  // namespace coralnpu::sim

#endif  // SIM_CORALNPU_V2_ENCODING_H_
