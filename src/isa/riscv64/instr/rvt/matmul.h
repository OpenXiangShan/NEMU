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

#ifdef CONFIG_RVT

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

// ======================================
// Output type: FP32
// ======================================

/**
 * @brief FP16 x FP16 -> FP32 matrix multiplication
 */
void matmul_fp32_fp16_fp16(void *input0, void *input1, void *output,
                           int input0_dim0, int input0_dim1, int input1_dim0,
                           int input1_dim1, int input0_stride0,
                           int input0_stride1, int input1_stride0,
                           int input1_stride1, int output_stride0,
                           int output_stride1);

/**
 * @brief FP16 x BF16 -> FP32 matrix multiplication
 */
void matmul_fp32_fp16_bf16(void *input0, void *input1, void *output,
                           int input0_dim0, int input0_dim1, int input1_dim0,
                           int input1_dim1, int input0_stride0,
                           int input0_stride1, int input1_stride0,
                           int input1_stride1, int output_stride0,
                           int output_stride1);

/**
 * @brief FP16 x TF32 -> FP32 matrix multiplication
 */
void matmul_fp32_fp16_tf32(void *input0, void *input1, void *output,
                           int input0_dim0, int input0_dim1, int input1_dim0,
                           int input1_dim1, int input0_stride0,
                           int input0_stride1, int input1_stride0,
                           int input1_stride1, int output_stride0,
                           int output_stride1);

/**
 * @brief BF16 x FP16 -> FP32 matrix multiplication
 */
void matmul_fp32_bf16_fp16(void *input0, void *input1, void *output,
                           int input0_dim0, int input0_dim1, int input1_dim0,
                           int input1_dim1, int input0_stride0,
                           int input0_stride1, int input1_stride0,
                           int input1_stride1, int output_stride0,
                           int output_stride1);

/**
 * @brief BF16 x BF16 -> FP32 matrix multiplication
 */
void matmul_fp32_bf16_bf16(void *input0, void *input1, void *output,
                           int input0_dim0, int input0_dim1, int input1_dim0,
                           int input1_dim1, int input0_stride0,
                           int input0_stride1, int input1_stride0,
                           int input1_stride1, int output_stride0,
                           int output_stride1);

/**
 * @brief BF16 x TF32 -> FP32 matrix multiplication
 */
void matmul_fp32_bf16_tf32(void *input0, void *input1, void *output,
                           int input0_dim0, int input0_dim1, int input1_dim0,
                           int input1_dim1, int input0_stride0,
                           int input0_stride1, int input1_stride0,
                           int input1_stride1, int output_stride0,
                           int output_stride1);

/**
 * @brief TF32 x FP16 -> FP32 matrix multiplication
 */
void matmul_fp32_tf32_fp16(void *input0, void *input1, void *output,
                           int input0_dim0, int input0_dim1, int input1_dim0,
                           int input1_dim1, int input0_stride0,
                           int input0_stride1, int input1_stride0,
                           int input1_stride1, int output_stride0,
                           int output_stride1);

/**
 * @brief TF32 x BF16 -> FP32 matrix multiplication
 */
void matmul_fp32_tf32_bf16(void *input0, void *input1, void *output,
                           int input0_dim0, int input0_dim1, int input1_dim0,
                           int input1_dim1, int input0_stride0,
                           int input0_stride1, int input1_stride0,
                           int input1_stride1, int output_stride0,
                           int output_stride1);

/**
 * @brief TF32 x TF32 -> FP32 matrix multiplication
 */
void matmul_fp32_tf32_tf32(void *input0, void *input1, void *output,
                           int input0_dim0, int input0_dim1, int input1_dim0,
                           int input1_dim1, int input0_stride0,
                           int input0_stride1, int input1_stride0,
                           int input1_stride1, int output_stride0,
                           int output_stride1);

// ======================================
// Output type: FP16
// ======================================

/**
 * @brief FP16 x FP16 -> FP16 matrix multiplication
 */
void matmul_fp16_fp16_fp16(void *input0, void *input1, void *output,
                           int input0_dim0, int input0_dim1, int input1_dim0,
                           int input1_dim1, int input0_stride0,
                           int input0_stride1, int input1_stride0,
                           int input1_stride1, int output_stride0,
                           int output_stride1);

/**
 * @brief FP16 x BF16 -> FP16 matrix multiplication
 */
void matmul_fp16_fp16_bf16(void *input0, void *input1, void *output,
                           int input0_dim0, int input0_dim1, int input1_dim0,
                           int input1_dim1, int input0_stride0,
                           int input0_stride1, int input1_stride0,
                           int input1_stride1, int output_stride0,
                           int output_stride1);

/**
 * @brief FP16 x TF32 -> FP16 matrix multiplication
 */
void matmul_fp16_fp16_tf32(void *input0, void *input1, void *output,
                           int input0_dim0, int input0_dim1, int input1_dim0,
                           int input1_dim1, int input0_stride0,
                           int input0_stride1, int input1_stride0,
                           int input1_stride1, int output_stride0,
                           int output_stride1);

/**
 * @brief BF16 x FP16 -> FP16 matrix multiplication
 */
void matmul_fp16_bf16_fp16(void *input0, void *input1, void *output,
                           int input0_dim0, int input0_dim1, int input1_dim0,
                           int input1_dim1, int input0_stride0,
                           int input0_stride1, int input1_stride0,
                           int input1_stride1, int output_stride0,
                           int output_stride1);

/**
 * @brief BF16 x BF16 -> FP16 matrix multiplication
 */
void matmul_fp16_bf16_bf16(void *input0, void *input1, void *output,
                           int input0_dim0, int input0_dim1, int input1_dim0,
                           int input1_dim1, int input0_stride0,
                           int input0_stride1, int input1_stride0,
                           int input1_stride1, int output_stride0,
                           int output_stride1);

/**
 * @brief BF16 x TF32 -> FP16 matrix multiplication
 */
void matmul_fp16_bf16_tf32(void *input0, void *input1, void *output,
                           int input0_dim0, int input0_dim1, int input1_dim0,
                           int input1_dim1, int input0_stride0,
                           int input0_stride1, int input1_stride0,
                           int input1_stride1, int output_stride0,
                           int output_stride1);

/**
 * @brief TF32 x FP16 -> FP16 matrix multiplication
 */
void matmul_fp16_tf32_fp16(void *input0, void *input1, void *output,
                           int input0_dim0, int input0_dim1, int input1_dim0,
                           int input1_dim1, int input0_stride0,
                           int input0_stride1, int input1_stride0,
                           int input1_stride1, int output_stride0,
                           int output_stride1);

/**
 * @brief TF32 x BF16 -> FP16 matrix multiplication
 */
void matmul_fp16_tf32_bf16(void *input0, void *input1, void *output,
                           int input0_dim0, int input0_dim1, int input1_dim0,
                           int input1_dim1, int input0_stride0,
                           int input0_stride1, int input1_stride0,
                           int input1_stride1, int output_stride0,
                           int output_stride1);

/**
 * @brief TF32 x TF32 -> FP16 matrix multiplication
 */
void matmul_fp16_tf32_tf32(void *input0, void *input1, void *output,
                           int input0_dim0, int input0_dim1, int input1_dim0,
                           int input1_dim1, int input0_stride0,
                           int input0_stride1, int input1_stride0,
                           int input1_stride1, int output_stride0,
                           int output_stride1);

/**
 * @brief FP32 x FP32 -> FP32 matrix multiplication
 */
void matmul_fp32_fp32_fp32(void *input0, void *input1, void *output,
                           int input0_dim0, int input0_dim1, int input1_dim0,
                           int input1_dim1, int input0_stride0,
                           int input0_stride1, int input1_stride0,
                           int input1_stride1, int output_stride0,
                           int output_stride1);

int matmul_s16_s8_s8(void *input0, void *input1, void *output, int input0_dim0,
                     int input0_dim1, int input1_dim0, int input1_dim1,
                     int input0_stride0, int input0_stride1, int input1_stride0,
                     int input1_stride1, int output_stride0,
                     int output_stride1);
int matmul_s32_s8_s8(void *input0, void *input1, void *output, int input0_dim0,
                     int input0_dim1, int input1_dim0, int input1_dim1,
                     int input0_stride0, int input0_stride1, int input1_stride0,
                     int input1_stride1, int output_stride0,
                     int output_stride1);
int matmul_s16_u8_u8(void *input0, void *input1, void *output, int input0_dim0,
                     int input0_dim1, int input1_dim0, int input1_dim1,
                     int input0_stride0, int input0_stride1, int input1_stride0,
                     int input1_stride1, int output_stride0,
                     int output_stride1);
int matmul_s32_u8_u8(void *input0, void *input1, void *output, int input0_dim0,
                     int input0_dim1, int input1_dim0, int input1_dim1,
                     int input0_stride0, int input0_stride1, int input1_stride0,
                     int input1_stride1, int output_stride0,
                     int output_stride1);

#endif /* TENSOR_MATMUL_H */

#endif /* CONFIG_RVT */
