/***************************************************************************************
 * Copyright (c) 2025 Institute of Computing Technology, Chinese Academy of
 *Sciences
 *
 * NEMU is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan
 *PSL v2. You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 *KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 *NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/

#include <common.h>
#ifdef CONFIG_CUSTOM_TENSOR

#include "matmul.h"
#include "type.h"

// ======================================
// Type-specific load/store functions
// ======================================

/* FP32 */
static float load_fp32(void *data, int idx) { return ((float *)data)[idx]; }

static void store_fp32(void *data, int idx, float value) {
  ((float *)data)[idx] = value;
}

/* FP16 */
static float load_fp16(void *data, int idx) {
  return fp16_to_fp32(((uint16_t *)data)[idx]);
}

static void store_fp16(void *data, int idx, float value) {
  ((uint16_t *)data)[idx] = fp32_to_fp16(value);
}

/* BF16 */
static float load_bf16(void *data, int idx) {
  return bf16_to_fp32(((uint16_t *)data)[idx]);
}

/* TF32 */
static float load_tf32(void *data, int idx) {
  return tf32_to_fp32(((uint32_t *)data)[idx]);
}

// ======================================
// Core matmul kernel with accumulation
// ======================================

static inline void matmul_accum_kernel(
    void *input0, void *input1, void *output, const MatMulConfig *config,
    const MatMulStrideConfig *strides, float (*convert0)(void *, int),
    float (*convert1)(void *, int), float (*load_output)(void *, int),
    void (*store_output)(void *, int, float)) {

  // Unpack configuration for readability
  int input0_dim0 = config->input0_dim0;
  int input0_dim1 = config->input0_dim1;
  int input1_dim0 = config->input1_dim0;
  int input1_dim1 = config->input1_dim1;

  // Unpack strides for readability
  int input0_stride0 = strides->input0_stride0;
  int input0_stride1 = strides->input0_stride1;
  int input1_stride0 = strides->input1_stride0;
  int input1_stride1 = strides->input1_stride1;
  int output_stride0 = strides->output_stride0;
  int output_stride1 = strides->output_stride1;

  // Dimension compatibility check
  if (input0_dim1 != input1_dim0) {
    return;
  }

  for (int i = 0; i < input0_dim0; i++) {
    for (int j = 0; j < input1_dim1; j++) {
      int out_idx = i * output_stride0 + j * output_stride1;
      float sum = load_output(output, out_idx);

      for (int k = 0; k < input0_dim1; k++) {
        int idx0 = i * input0_stride0 + k * input0_stride1;
        int idx1 = k * input1_stride0 + j * input1_stride1;

        float a = convert0(input0, idx0);
        float b = convert1(input1, idx1);
        sum += a * b;
      }
      store_output(output, out_idx, sum);
    }
  }
}

// ======================================
// FP32 Output Kernels
// ======================================

void matmul_fp32_fp16_fp16(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides) {
  matmul_accum_kernel(input0, input1, output, config, strides, load_fp16,
                      load_fp16, load_fp32, store_fp32);
}

void matmul_fp32_fp16_bf16(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides) {
  matmul_accum_kernel(input0, input1, output, config, strides, load_fp16,
                      load_bf16, load_fp32, store_fp32);
}

void matmul_fp32_fp16_tf32(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides) {
  matmul_accum_kernel(input0, input1, output, config, strides, load_fp16,
                      load_tf32, load_fp32, store_fp32);
}

void matmul_fp32_bf16_fp16(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides) {
  matmul_accum_kernel(input0, input1, output, config, strides, load_bf16,
                      load_fp16, load_fp32, store_fp32);
}

void matmul_fp32_bf16_bf16(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides) {
  matmul_accum_kernel(input0, input1, output, config, strides, load_bf16,
                      load_bf16, load_fp32, store_fp32);
}

void matmul_fp32_bf16_tf32(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides) {
  matmul_accum_kernel(input0, input1, output, config, strides, load_bf16,
                      load_tf32, load_fp32, store_fp32);
}

void matmul_fp32_tf32_fp16(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides) {
  matmul_accum_kernel(input0, input1, output, config, strides, load_tf32,
                      load_fp16, load_fp32, store_fp32);
}

void matmul_fp32_tf32_bf16(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides) {
  matmul_accum_kernel(input0, input1, output, config, strides, load_tf32,
                      load_bf16, load_fp32, store_fp32);
}

void matmul_fp32_tf32_tf32(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides) {
  matmul_accum_kernel(input0, input1, output, config, strides, load_tf32,
                      load_tf32, load_fp32, store_fp32);
}

void matmul_fp32_fp32_fp32(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides) {
  matmul_accum_kernel(input0, input1, output, config, strides, load_fp32,
                      load_fp32, load_fp32, store_fp32);
}

// ======================================
// FP16 Output Kernels
// ======================================

void matmul_fp16_fp16_fp16(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides) {
  matmul_accum_kernel(input0, input1, output, config, strides, load_fp16,
                      load_fp16, load_fp16, store_fp16);
}

void matmul_fp16_fp16_bf16(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides) {
  matmul_accum_kernel(input0, input1, output, config, strides, load_fp16,
                      load_bf16, load_fp16, store_fp16);
}

void matmul_fp16_fp16_tf32(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides) {
  matmul_accum_kernel(input0, input1, output, config, strides, load_fp16,
                      load_tf32, load_fp16, store_fp16);
}

void matmul_fp16_bf16_fp16(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides) {
  matmul_accum_kernel(input0, input1, output, config, strides, load_bf16,
                      load_fp16, load_fp16, store_fp16);
}

void matmul_fp16_bf16_bf16(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides) {
  matmul_accum_kernel(input0, input1, output, config, strides, load_bf16,
                      load_bf16, load_fp16, store_fp16);
}

void matmul_fp16_bf16_tf32(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides) {
  matmul_accum_kernel(input0, input1, output, config, strides, load_bf16,
                      load_tf32, load_fp16, store_fp16);
}

void matmul_fp16_tf32_fp16(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides) {
  matmul_accum_kernel(input0, input1, output, config, strides, load_tf32,
                      load_fp16, load_fp16, store_fp16);
}

void matmul_fp16_tf32_bf16(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides) {
  matmul_accum_kernel(input0, input1, output, config, strides, load_tf32,
                      load_bf16, load_fp16, store_fp16);
}

void matmul_fp16_tf32_tf32(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides) {
  matmul_accum_kernel(input0, input1, output, config, strides, load_tf32,
                      load_tf32, load_fp16, store_fp16);
}

// ======================================
// Integer matrix multiplication functions
// ======================================

int matmul_s16_s8_s8(void *input0, void *input1, void *output,
                     const MatMulConfig *config,
                     const MatMulStrideConfig *strides) {

  // Unpack configuration
  int input0_dim0 = config->input0_dim0;
  int input0_dim1 = config->input0_dim1;
  int input1_dim0 = config->input1_dim0;
  int input1_dim1 = config->input1_dim1;

  // Unpack strides
  int input0_stride0 = strides->input0_stride0;
  int input0_stride1 = strides->input0_stride1;
  int input1_stride0 = strides->input1_stride0;
  int input1_stride1 = strides->input1_stride1;
  int output_stride0 = strides->output_stride0;
  int output_stride1 = strides->output_stride1;

  // Check if matrix dimensions are compatible for multiplication
  if (input0_dim1 != input1_dim0) {
    return -1; // Dimension mismatch error
  }

  for (int i = 0; i < input0_dim0; i++) {
    for (int j = 0; j < input1_dim1; j++) {
      int16_t sum = 0;
      for (int k = 0; k < input0_dim1; k++) {
        sum += (int16_t)((
                   (int8_t *)input0)[i * input0_stride0 + k * input0_stride1]) *
               (int16_t)(
                   ((int8_t *)input1)[k * input1_stride0 + j * input1_stride1]);
      }
      ((int16_t *)output)[i * output_stride0 + j * output_stride1] += sum;
    }
  }
  return 0;
}

int matmul_s32_s8_s8(void *input0, void *input1, void *output,
                     const MatMulConfig *config,
                     const MatMulStrideConfig *strides) {

  // Unpack configuration
  int input0_dim0 = config->input0_dim0;
  int input0_dim1 = config->input0_dim1;
  int input1_dim0 = config->input1_dim0;
  int input1_dim1 = config->input1_dim1;

  // Unpack strides
  int input0_stride0 = strides->input0_stride0;
  int input0_stride1 = strides->input0_stride1;
  int input1_stride0 = strides->input1_stride0;
  int input1_stride1 = strides->input1_stride1;
  int output_stride0 = strides->output_stride0;
  int output_stride1 = strides->output_stride1;

  if (input0_dim1 != input1_dim0) {
    return -1;
  }

  for (int i = 0; i < input0_dim0; i++) {
    for (int j = 0; j < input1_dim1; j++) {
      int32_t sum = 0;
      for (int k = 0; k < input0_dim1; k++) {
        sum += (int32_t)((
                   (int8_t *)input0)[i * input0_stride0 + k * input0_stride1]) *
               (int32_t)(
                   ((int8_t *)input1)[k * input1_stride0 + j * input1_stride1]);
      }
      ((int32_t *)output)[i * output_stride0 + j * output_stride1] += sum;
    }
  }
  return 0;
}

int matmul_s32_u8_u8(void *input0, void *input1, void *output,
                     const MatMulConfig *config,
                     const MatMulStrideConfig *strides) {

  // Unpack configuration
  int input0_dim0 = config->input0_dim0;
  int input0_dim1 = config->input0_dim1;
  int input1_dim0 = config->input1_dim0;
  int input1_dim1 = config->input1_dim1;

  // Unpack strides
  int input0_stride0 = strides->input0_stride0;
  int input0_stride1 = strides->input0_stride1;
  int input1_stride0 = strides->input1_stride0;
  int input1_stride1 = strides->input1_stride1;
  int output_stride0 = strides->output_stride0;
  int output_stride1 = strides->output_stride1;

  if (input0_dim1 != input1_dim0) {
    return -1;
  }

  for (int i = 0; i < input0_dim0; i++) {
    for (int j = 0; j < input1_dim1; j++) {
      int32_t sum = 0;
      for (int k = 0; k < input0_dim1; k++) {
        sum +=
            (int32_t)(
                ((uint8_t *)input0)[i * input0_stride0 + k * input0_stride1]) *
            (int32_t)(
                ((uint8_t *)input1)[k * input1_stride0 + j * input1_stride1]);
      }
      ((int32_t *)output)[i * output_stride0 + j * output_stride1] += sum;
    }
  }
  return 0;
}

int matmul_s16_u8_u8(void *input0, void *input1, void *output,
                     const MatMulConfig *config,
                     const MatMulStrideConfig *strides) {

  // Unpack configuration
  int input0_dim0 = config->input0_dim0;
  int input0_dim1 = config->input0_dim1;
  int input1_dim0 = config->input1_dim0;
  int input1_dim1 = config->input1_dim1;

  // Unpack strides
  int input0_stride0 = strides->input0_stride0;
  int input0_stride1 = strides->input0_stride1;
  int input1_stride0 = strides->input1_stride0;
  int input1_stride1 = strides->input1_stride1;
  int output_stride0 = strides->output_stride0;
  int output_stride1 = strides->output_stride1;

  if (input0_dim1 != input1_dim0) {
    return -1;
  }

  for (int i = 0; i < input0_dim0; i++) {
    for (int j = 0; j < input1_dim1; j++) {
      int16_t sum = 0;
      for (int k = 0; k < input0_dim1; k++) {
        sum +=
            (int16_t)(
                ((uint8_t *)input0)[i * input0_stride0 + k * input0_stride1]) *
            (int16_t)(
                ((uint8_t *)input1)[k * input1_stride0 + j * input1_stride1]);
      }
      ((int16_t *)output)[i * output_stride0 + j * output_stride1] += sum;
    }
  }
  return 0;
}

#endif /* CONFIG_CUSTOM_TENSOR */
