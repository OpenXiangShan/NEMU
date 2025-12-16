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

#ifndef RISCV_CUSTOM_TENSOR_SFU_H
#define RISCV_CUSTOM_TENSOR_SFU_H

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/**
 * @brief Special Function Unit (SFU) operation types
 * 
 * SFUs are specialized hardware units that accelerate common mathematical
 * functions that are expensive to compute in general-purpose ALUs.
 * These operations are frequently used in neural network activation functions
 * and normalization layers.
 */
typedef enum {
  SFU_RECIP,   /**< Reciprocal: f(x) = 1/x. Used in division, normalization */
  SFU_RSQRT,   /**< Reciprocal square root: f(x) = 1/√x. Used in normalization (RMSNorm, LayerNorm) */
  SFU_SIN,     /**< Sine function: f(x) = sin(x). Used in positional encodings (Transformers) */
  SFU_COS,     /**< Cosine function: f(x) = cos(x). Used in positional encodings */
  SFU_EXP,     /**< Exponential: f(x) = e^x. Used in softmax, sigmoid, GELU */
  SFU_RELU     /**< Rectified Linear Unit: f(x) = max(0, x). Most common activation function */
} SpecialFunctionUnitType;

/**
 * @brief Apply special function operation to a 4D FP32 tensor
 * 
 * This function applies element-wise special mathematical functions to a
 * 4-dimensional tensor using optimized SFU hardware when available.
 * The operation is applied independently to each element.
 * 
 * @param input Pointer to input tensor data (FP32 format)
 * @param output Pointer to output tensor buffer (must be pre-allocated, FP32 format)
 * @param input_dims Array of 4 dimensions [D0, D1, D2, D3] specifying tensor shape
 *                  Typical usage:
 *                  - D0: Batch size (1-256)
 *                  - D1: Channels/features (1-4096)
 *                  - D2: Height (1-1024)
 *                  - D3: Width (1-1024)
 * @param input_strides Array of 4 stride values for input tensor (in elements, not bytes)
 *                     Allows processing of tensors with non-contiguous memory layouts
 * @param output_strides Array of 4 stride values for output tensor (in elements)
 * @param op Special function operation to apply (see SpecialFunctionUnitType)
 * @param mode Operation mode flags
 * @return true if operation succeeded
 * @return false if operation failed (invalid parameters, unsupported operation)
 */
bool tensor_sfu_fp32(float *input, float *output, int32_t input_dims[4],
                     int32_t input_strides[4], int32_t output_strides[4],
                     SpecialFunctionUnitType op, uint8_t mode);

#endif /* RISCV_CUSTOM_TENSOR_SFU_H */

#endif /* CONFIG_CUSTOM_TENSOR */
