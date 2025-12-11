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

#ifdef CONFIG_CUSTOM_TENSOR

#ifndef TENSOR_MATMUL_H
#define TENSOR_MATMUL_H
#include <memory/paddr.h>

enum {
  MATMUL_INPUT_TYPE_fp16 = 0,
  MATMUL_INPUT_TYPE_bf16 = 1,
  MATMUL_INPUT_TYPE_fp32 = 2,
  MATMUL_INPUT_TYPE_tf32 = 3,
  MATMUL_INPUT_TYPE_COUNT = 5,
};

enum {
  MATMUL_OUTPUT_TYPE_fp32 = 0,
  MATMUL_OUTPUT_TYPE_fp16 = 1,
  MATMUL_OUTPUT_TYPE_COUNT = 2,
};

/**
 * @brief Matrix multiplication stride configuration structure
 *
 * Encapsulates all stride parameters for input and output tensors.
 * This reduces the number of function parameters and improves code readability.
 */
typedef struct {
  // Input0 tensor strides
  int input0_stride0; /* First input tensor first dimension stride */
  int input0_stride1; /* First input tensor second dimension stride */

  // Input1 tensor strides
  int input1_stride0; /* Second input tensor first dimension stride */
  int input1_stride1; /* Second input tensor second dimension stride */

  // Output tensor strides
  int output_stride0; /* Output tensor first dimension stride */
  int output_stride1; /* Output tensor second dimension stride */
} MatMulStrideConfig;

/**
 * @brief Matrix multiplication configuration structure
 */
typedef struct {
  // Matrix dimensions
  int input0_dim0; /* First input matrix rows */
  int input0_dim1; /* First input matrix columns */
  int input1_dim0; /* Second input matrix rows */
  int input1_dim1; /* Second input matrix columns */
} MatMulConfig;

// ======================================
// Output type: FP32
// ======================================

/**
 * @brief FP16 x FP16 -> FP32 matrix multiplication
 */
void matmul_fp32_fp16_fp16(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief FP16 x BF16 -> FP32 matrix multiplication
 */
void matmul_fp32_fp16_bf16(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief FP16 x TF32 -> FP32 matrix multiplication
 */
void matmul_fp32_fp16_tf32(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief BF16 x FP16 -> FP32 matrix multiplication
 */
void matmul_fp32_bf16_fp16(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief BF16 x BF16 -> FP32 matrix multiplication
 */
void matmul_fp32_bf16_bf16(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief BF16 x TF32 -> FP32 matrix multiplication
 */
void matmul_fp32_bf16_tf32(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief TF32 x FP16 -> FP32 matrix multiplication
 */
void matmul_fp32_tf32_fp16(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief TF32 x BF16 -> FP32 matrix multiplication
 */
void matmul_fp32_tf32_bf16(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief TF32 x TF32 -> FP32 matrix multiplication
 */
void matmul_fp32_tf32_tf32(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

// ======================================
// Output type: FP16
// ======================================

/**
 * @brief FP16 x FP16 -> FP16 matrix multiplication
 */
void matmul_fp16_fp16_fp16(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief FP16 x BF16 -> FP16 matrix multiplication
 */
void matmul_fp16_fp16_bf16(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief FP16 x TF32 -> FP16 matrix multiplication
 */
void matmul_fp16_fp16_tf32(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief BF16 x FP16 -> FP16 matrix multiplication
 */
void matmul_fp16_bf16_fp16(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief BF16 x BF16 -> FP16 matrix multiplication
 */
void matmul_fp16_bf16_bf16(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief BF16 x TF32 -> FP16 matrix multiplication
 */
void matmul_fp16_bf16_tf32(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief TF32 x FP16 -> FP16 matrix multiplication
 */
void matmul_fp16_tf32_fp16(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief TF32 x BF16 -> FP16 matrix multiplication
 */
void matmul_fp16_tf32_bf16(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief TF32 x TF32 -> FP16 matrix multiplication
 */
void matmul_fp16_tf32_tf32(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief FP32 x FP32 -> FP32 matrix multiplication
 */
void matmul_fp32_fp32_fp32(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

// ======================================
// Integer matrix multiplication functions
// ======================================

/**
 * @brief Signed 8-bit x Signed 8-bit -> Signed 16-bit matrix multiplication
 */
int matmul_s16_s8_s8(void *input0, void *input1, void *output,
                     const MatMulConfig *config,
                     const MatMulStrideConfig *strides);

/**
 * @brief Signed 8-bit x Signed 8-bit -> Signed 32-bit matrix multiplication
 */
int matmul_s32_s8_s8(void *input0, void *input1, void *output,
                     const MatMulConfig *config,
                     const MatMulStrideConfig *strides);

/**
 * @brief Unsigned 8-bit x Unsigned 8-bit -> Signed 16-bit matrix multiplication
 */
int matmul_s16_u8_u8(void *input0, void *input1, void *output,
                     const MatMulConfig *config,
                     const MatMulStrideConfig *strides);

/**
 * @brief Unsigned 8-bit x Unsigned 8-bit -> Signed 32-bit matrix multiplication
 */
int matmul_s32_u8_u8(void *input0, void *input1, void *output,
                     const MatMulConfig *config,
                     const MatMulStrideConfig *strides);

#endif /* TENSOR_MATMUL_H */

#endif /* CONFIG_CUSTOM_TENSOR */
