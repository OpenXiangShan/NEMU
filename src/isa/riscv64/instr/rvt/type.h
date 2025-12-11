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

#ifndef RISCV_CUSTOM_TENSOR_TYPE_H
#define RISCV_CUSTOM_TENSOR_TYPE_H

#include <math.h>
#include <stdint.h>
#include <stdio.h>

/**
 * @brief 8-bit floating point type (E4M3 format)
 * 
 * 8-bit float format:
 * - 1 sign bit
 * - 4 exponent bits (bias = 7)
 * - 3 mantissa bits
 */
typedef uint8_t fp8;

/**
 * @brief 16-bit half-precision floating point type (IEEE 754)
 * 
 * Standard FP16 format:
 * - 1 sign bit
 * - 5 exponent bits (bias = 15)
 * - 10 mantissa bits
 */
typedef uint16_t fp16;

/**
 * @brief 16-bit Brain floating point type
 * 
 * BF16 format:
 * - 1 sign bit
 * - 8 exponent bits (bias = 127, same as FP32)
 * - 7 mantissa bits
 */
typedef uint16_t bf16;

/**
 * @brief Rounding mode for type conversion operations
 * 
 * Defines the rounding behavior when converting between different
 * floating point precision formats.
 */
typedef enum __TX_CONVERT_MODE {
  TX_CONVERT_ROUND = 0,  /**< Round to nearest, ties to even (IEEE 754 default) */
  TX_CONVERT_FLOOR = 1,  /**< Round toward negative infinity */
  TX_CONVERT_CEIL = 2    /**< Round toward positive infinity */
} ConvertMode;

/**
 * @brief Convert FP8 (E4M3) to FP32
 * 
 * @param x FP8 value in E4M3 format
 * @return float FP32 equivalent value
 */
float fp8_e4m3_to_fp32(fp8 x);

/**
 * @brief Convert FP32 to FP8 (E4M3) with default rounding
 * 
 * @param f FP32 input value
 * @return fp8 FP8 value in E4M3 format
 */
fp8 fp32_to_fp8_e4m3(float f);

/**
 * @brief Convert FP32 to FP8 (E4M3) with specified rounding mode
 * 
 * @param f FP32 input value
 * @param mode Rounding mode (ROUND, FLOOR, CEIL)
 * @return fp8 FP8 value in E4M3 format
 */
fp8 fp32_to_fp8_e4m3_with_mode(float f, ConvertMode mode);

/**
 * @brief Convert FP16 to FP32
 * 
 * @param h FP16 input value
 * @return float FP32 equivalent value
 */
float fp16_to_fp32(fp16 h);

/**
 * @brief Convert FP32 to FP16 with default rounding
 * 
 * @param f FP32 input value
 * @return fp16 FP16 value
 */
fp16 fp32_to_fp16(float f);

/**
 * @brief Convert FP32 to FP16 with specified rounding mode
 * 
 * @param f FP32 input value
 * @param m Rounding mode (ROUND, FLOOR, CEIL)
 * @return fp16 FP16 value
 */
fp16 fp32_to_fp16_with_mode(float f, ConvertMode m);

/**
 * @brief Convert BF16 to FP32
 * 
 * @param h BF16 input value
 * @return float FP32 equivalent value
 */
float bf16_to_fp32(bf16 h);

/**
 * @brief Convert FP32 to BF16 with default rounding
 * 
 * @param f FP32 input value
 * @return bf16 BF16 value
 */
bf16 fp32_to_bf16(float f);

/**
 * @brief Convert FP32 to BF16 with specified rounding mode
 * 
 * @param f FP32 input value
 * @param m Rounding mode (ROUND, FLOOR, CEIL)
 * @return fp16 BF16 value (return type may need correction)
 */
fp16 fp32_to_bf16_with_mode(float f, ConvertMode m);

/**
 * @brief Convert TF32 to FP32
 * 
 * TensorFloat-32 (TF32) is a 19-bit format stored in 32 bits:
 * - 1 sign bit
 * - 8 exponent bits (bias = 127, same as FP32)
 * - 10 mantissa bits (stored in bits 22-13)
 * 
 * @param t TF32 value stored in uint32_t (lower 19 bits used)
 * @return float FP32 equivalent value
 */
float tf32_to_fp32(uint32_t t);

/**
 * @brief Convert FP32 to TF32
 * 
 * @param f FP32 input value
 * @return uint32_t TF32 representation (19-bit in 32-bit container)
 */
uint32_t fp32_to_tf32(float f);

#endif /* RISCV_CUSTOM_TENSOR_TYPE_H */

#endif /* CONFIG_CUSTOM_TENSOR */
