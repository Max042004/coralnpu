// Copyright 2026 Google LLC
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

// Face-embedding CNN (MobileFaceNet, int8) on the CoralNPU TFLite Micro runtime.
// The host writes a 112x112x3 int8 image to `inference_input`, runs the
// program, and reads the 192-element int8 embedding from `inference_output`.

#include <stdint.h>
#include <stdio.h>

#include "sw/opt/litert-micro/conv.h"
#include "sw/opt/rvv_opt.h"
#include "tensorflow/lite/core/c/common.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tests/npusim_examples/face_embedding_model.h"

namespace {
constexpr int kInputBytes = 112 * 112 * 3;
constexpr int kEmbeddingSize = 192;

using FaceOpResolver = tflite::MicroMutableOpResolver<12>;
using coralnpu_v2::opt::litert_micro::Register_CONV_2D;

TfLiteStatus RegisterOps(FaceOpResolver& op_resolver) {
  TF_LITE_ENSURE_STATUS(op_resolver.AddConv2D(Register_CONV_2D()));
  // The RVV depthwise kernel faults on this model's shapes (store to an
  // out-of-range address), so use the reference TFLM kernel for depthwise.
  TF_LITE_ENSURE_STATUS(op_resolver.AddDepthwiseConv2D());
  TF_LITE_ENSURE_STATUS(op_resolver.AddAdd());
  TF_LITE_ENSURE_STATUS(op_resolver.AddSub());
  TF_LITE_ENSURE_STATUS(op_resolver.AddMul());
  TF_LITE_ENSURE_STATUS(op_resolver.AddAbs());
  TF_LITE_ENSURE_STATUS(op_resolver.AddRelu());
  TF_LITE_ENSURE_STATUS(op_resolver.AddReshape());
  TF_LITE_ENSURE_STATUS(op_resolver.AddL2Normalization());
  TF_LITE_ENSURE_STATUS(op_resolver.AddQuantize());
  TF_LITE_ENSURE_STATUS(op_resolver.AddDequantize());
  return kTfLiteOk;
}
}  // namespace

extern "C" {
// The model (1.6 MB) and the arena share the 4 MB external data region.
constexpr size_t kTensorArenaSize = 2300 * 1024;
int8_t inference_status = -1;
int8_t inference_input[kInputBytes]
    __attribute__((section(".data"), aligned(16)));
int8_t inference_output[kEmbeddingSize]
    __attribute__((section(".data"), aligned(16)));
uint8_t tensor_arena[kTensorArenaSize]
    __attribute__((section(".extdata"), aligned(16)));
}

int main(int argc, char** argv) {
  const tflite::Model* model =
      tflite::GetModel(g_face_embedding_int8_model_data);
  FaceOpResolver op_resolver;
  if (RegisterOps(op_resolver) != kTfLiteOk) {
    printf("Error registering ops\n");
    return -1;
  }
  tflite::MicroInterpreter interpreter(model, op_resolver, tensor_arena,
                                       kTensorArenaSize);
  if (interpreter.AllocateTensors() != kTfLiteOk) {
    printf("Error during AllocateTensors\n");
    return -1;
  }
  TfLiteTensor* input = interpreter.input(0);
  if (input == nullptr || input->bytes != kInputBytes) {
    printf("Unexpected input tensor\n");
    return -1;
  }
  coralnpu_v2::opt::Memcpy(input->data.data, inference_input, input->bytes);

  if (interpreter.Invoke() != kTfLiteOk) {
    printf("Error during Invoke\n");
    return -1;
  }

  TfLiteTensor* output = interpreter.output(0);
  if (output == nullptr || output->bytes != kEmbeddingSize) {
    printf("Unexpected output tensor\n");
    return -1;
  }
  coralnpu_v2::opt::Memcpy(inference_output, output->data.data, kEmbeddingSize);
  inference_status = 0;
  return 0;
}
