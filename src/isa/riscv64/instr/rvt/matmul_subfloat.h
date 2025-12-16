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

#include "matmul.h"
#include <memory/paddr.h>

#ifndef TENSOR_SUBFLOAT_MATMUL_H
#define TENSOR_SUBFLOAT_MATMUL_H

enum {
  MATMUL_INPUT_TYPE_ue1m7 = 0,
  MATMUL_INPUT_TYPE_e1m6,
  MATMUL_INPUT_TYPE_e1m5,
  MATMUL_INPUT_TYPE_e1m4,
  MATMUL_INPUT_TYPE_e1m3,
  MATMUL_INPUT_TYPE_e1m2,
  MATMUL_INPUT_TYPE_e1m1,
  MATMUL_INPUT_TYPE_e1m0,
  MATMUL_INPUT_TYPE_ue1m6,
  MATMUL_INPUT_TYPE_ue2m6,
  MATMUL_INPUT_TYPE_e2m5,
  MATMUL_INPUT_TYPE_e2m4,
  MATMUL_INPUT_TYPE_e2m3,
  MATMUL_INPUT_TYPE_e2m2,
  MATMUL_INPUT_TYPE_e2m1,
  MATMUL_INPUT_TYPE_e2m0,
  MATMUL_INPUT_TYPE_ue1m5,
  MATMUL_INPUT_TYPE_ue2m5,
  MATMUL_INPUT_TYPE_ue3m5,
  MATMUL_INPUT_TYPE_e3m4,
  MATMUL_INPUT_TYPE_e3m3,
  MATMUL_INPUT_TYPE_e3m2,
  MATMUL_INPUT_TYPE_e3m1,
  MATMUL_INPUT_TYPE_e3m0,
  MATMUL_INPUT_TYPE_ue1m4,
  MATMUL_INPUT_TYPE_ue2m4,
  MATMUL_INPUT_TYPE_ue3m4,
  MATMUL_INPUT_TYPE_ue4m4,
  MATMUL_INPUT_TYPE_e4m3,
  MATMUL_INPUT_TYPE_e4m2,
  MATMUL_INPUT_TYPE_e4m1,
  MATMUL_INPUT_TYPE_e4m0,
  MATMUL_INPUT_TYPE_ue1m3,
  MATMUL_INPUT_TYPE_ue2m3,
  MATMUL_INPUT_TYPE_ue3m3,
  MATMUL_INPUT_TYPE_ue4m3,
  MATMUL_INPUT_TYPE_ue5m3,
  MATMUL_INPUT_TYPE_e5m2,
  MATMUL_INPUT_TYPE_e5m1,
  MATMUL_INPUT_TYPE_e5m0,
  MATMUL_INPUT_TYPE_ue1m2,
  MATMUL_INPUT_TYPE_ue2m2,
  MATMUL_INPUT_TYPE_ue3m2,
  MATMUL_INPUT_TYPE_ue4m2,
  MATMUL_INPUT_TYPE_ue5m2,
  MATMUL_INPUT_TYPE_ue6m2,
  MATMUL_INPUT_TYPE_e6m1,
  MATMUL_INPUT_TYPE_e6m0,
  MATMUL_INPUT_TYPE_ue1m1,
  MATMUL_INPUT_TYPE_ue2m1,
  MATMUL_INPUT_TYPE_ue3m1,
  MATMUL_INPUT_TYPE_ue4m1,
  MATMUL_INPUT_TYPE_ue5m1,
  MATMUL_INPUT_TYPE_ue6m1,
  MATMUL_INPUT_TYPE_ue7m1,
  MATMUL_INPUT_TYPE_e7m0,
  MATMUL_INPUT_TYPE_ue1m0,
  MATMUL_INPUT_TYPE_ue2m0,
  MATMUL_INPUT_TYPE_ue3m0,
  MATMUL_INPUT_TYPE_ue4m0,
  MATMUL_INPUT_TYPE_ue5m0,
  MATMUL_INPUT_TYPE_ue6m0,
  MATMUL_INPUT_TYPE_ue7m0,
  MATMUL_INPUT_TYPE_ue8m0,
  MATMUL_SUBFLOAT_INPUT_TYPE_COUNT,
};

enum {
  MATMUL_SUBFLOAT_OUTPUT_TYPE_fp32 = 0,
  MATMUL_SUBFLOAT_OUTPUT_TYPE_fp16 = 1,
  MATMUL_SUBFLOAT_OUTPUT_TYPE_COUNT = 2,
};

// ======================================
// FP32 Output Functions
// ======================================

/**
 * @brief ue1m7 x ue1m7 -> fp32 matrix multiplication
 */
void matmul_fp32_ue1m7_ue1m7(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief e1m6 x e1m6 -> fp32 matrix multiplication
 */
void matmul_fp32_e1m6_e1m6(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e1m5 x e1m5 -> fp32 matrix multiplication
 */
void matmul_fp32_e1m5_e1m5(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e1m4 x e1m4 -> fp32 matrix multiplication
 */
void matmul_fp32_e1m4_e1m4(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e1m3 x e1m3 -> fp32 matrix multiplication
 */
void matmul_fp32_e1m3_e1m3(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e1m2 x e1m2 -> fp32 matrix multiplication
 */
void matmul_fp32_e1m2_e1m2(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e1m1 x e1m1 -> fp32 matrix multiplication
 */
void matmul_fp32_e1m1_e1m1(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e1m0 x e1m0 -> fp32 matrix multiplication
 */
void matmul_fp32_e1m0_e1m0(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief ue1m6 x ue1m6 -> fp32 matrix multiplication
 */
void matmul_fp32_ue1m6_ue1m6(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue2m6 x ue2m6 -> fp32 matrix multiplication
 */
void matmul_fp32_ue2m6_ue2m6(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief e2m5 x e2m5 -> fp32 matrix multiplication
 */
void matmul_fp32_e2m5_e2m5(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e2m4 x e2m4 -> fp32 matrix multiplication
 */
void matmul_fp32_e2m4_e2m4(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e2m3 x e2m3 -> fp32 matrix multiplication
 */
void matmul_fp32_e2m3_e2m3(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e2m2 x e2m2 -> fp32 matrix multiplication
 */
void matmul_fp32_e2m2_e2m2(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e2m1 x e2m1 -> fp32 matrix multiplication
 */
void matmul_fp32_e2m1_e2m1(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e2m0 x e2m0 -> fp32 matrix multiplication
 */
void matmul_fp32_e2m0_e2m0(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief ue1m5 x ue1m5 -> fp32 matrix multiplication
 */
void matmul_fp32_ue1m5_ue1m5(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue2m5 x ue2m5 -> fp32 matrix multiplication
 */
void matmul_fp32_ue2m5_ue2m5(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue3m5 x ue3m5 -> fp32 matrix multiplication
 */
void matmul_fp32_ue3m5_ue3m5(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief e3m4 x e3m4 -> fp32 matrix multiplication
 */
void matmul_fp32_e3m4_e3m4(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e3m3 x e3m3 -> fp32 matrix multiplication
 */
void matmul_fp32_e3m3_e3m3(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e3m2 x e3m2 -> fp32 matrix multiplication
 */
void matmul_fp32_e3m2_e3m2(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e3m1 x e3m1 -> fp32 matrix multiplication
 */
void matmul_fp32_e3m1_e3m1(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e3m0 x e3m0 -> fp32 matrix multiplication
 */
void matmul_fp32_e3m0_e3m0(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief ue1m4 x ue1m4 -> fp32 matrix multiplication
 */
void matmul_fp32_ue1m4_ue1m4(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue2m4 x ue2m4 -> fp32 matrix multiplication
 */
void matmul_fp32_ue2m4_ue2m4(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue3m4 x ue3m4 -> fp32 matrix multiplication
 */
void matmul_fp32_ue3m4_ue3m4(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue4m4 x ue4m4 -> fp32 matrix multiplication
 */
void matmul_fp32_ue4m4_ue4m4(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief e4m3 x e4m3 -> fp32 matrix multiplication
 */
void matmul_fp32_e4m3_e4m3(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e4m2 x e4m2 -> fp32 matrix multiplication
 */
void matmul_fp32_e4m2_e4m2(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e4m1 x e4m1 -> fp32 matrix multiplication
 */
void matmul_fp32_e4m1_e4m1(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e4m0 x e4m0 -> fp32 matrix multiplication
 */
void matmul_fp32_e4m0_e4m0(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief ue1m3 x ue1m3 -> fp32 matrix multiplication
 */
void matmul_fp32_ue1m3_ue1m3(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue2m3 x ue2m3 -> fp32 matrix multiplication
 */
void matmul_fp32_ue2m3_ue2m3(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue3m3 x ue3m3 -> fp32 matrix multiplication
 */
void matmul_fp32_ue3m3_ue3m3(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue4m3 x ue4m3 -> fp32 matrix multiplication
 */
void matmul_fp32_ue4m3_ue4m3(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue5m3 x ue5m3 -> fp32 matrix multiplication
 */
void matmul_fp32_ue5m3_ue5m3(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief e5m2 x e5m2 -> fp32 matrix multiplication
 */
void matmul_fp32_e5m2_e5m2(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e5m1 x e5m1 -> fp32 matrix multiplication
 */
void matmul_fp32_e5m1_e5m1(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e5m0 x e5m0 -> fp32 matrix multiplication
 */
void matmul_fp32_e5m0_e5m0(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief ue1m2 x ue1m2 -> fp32 matrix multiplication
 */
void matmul_fp32_ue1m2_ue1m2(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue2m2 x ue2m2 -> fp32 matrix multiplication
 */
void matmul_fp32_ue2m2_ue2m2(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue3m2 x ue3m2 -> fp32 matrix multiplication
 */
void matmul_fp32_ue3m2_ue3m2(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue4m2 x ue4m2 -> fp32 matrix multiplication
 */
void matmul_fp32_ue4m2_ue4m2(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue5m2 x ue5m2 -> fp32 matrix multiplication
 */
void matmul_fp32_ue5m2_ue5m2(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue6m2 x ue6m2 -> fp32 matrix multiplication
 */
void matmul_fp32_ue6m2_ue6m2(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief e6m1 x e6m1 -> fp32 matrix multiplication
 */
void matmul_fp32_e6m1_e6m1(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e6m0 x e6m0 -> fp32 matrix multiplication
 */
void matmul_fp32_e6m0_e6m0(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief ue1m1 x ue1m1 -> fp32 matrix multiplication
 */
void matmul_fp32_ue1m1_ue1m1(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue2m1 x ue2m1 -> fp32 matrix multiplication
 */
void matmul_fp32_ue2m1_ue2m1(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue3m1 x ue3m1 -> fp32 matrix multiplication
 */
void matmul_fp32_ue3m1_ue3m1(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue4m1 x ue4m1 -> fp32 matrix multiplication
 */
void matmul_fp32_ue4m1_ue4m1(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue5m1 x ue5m1 -> fp32 matrix multiplication
 */
void matmul_fp32_ue5m1_ue5m1(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue6m1 x ue6m1 -> fp32 matrix multiplication
 */
void matmul_fp32_ue6m1_ue6m1(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue7m1 x ue7m1 -> fp32 matrix multiplication
 */
void matmul_fp32_ue7m1_ue7m1(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief e7m0 x e7m0 -> fp32 matrix multiplication
 */
void matmul_fp32_e7m0_e7m0(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief ue1m0 x ue1m0 -> fp32 matrix multiplication
 */
void matmul_fp32_ue1m0_ue1m0(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue2m0 x ue2m0 -> fp32 matrix multiplication
 */
void matmul_fp32_ue2m0_ue2m0(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue3m0 x ue3m0 -> fp32 matrix multiplication
 */
void matmul_fp32_ue3m0_ue3m0(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue4m0 x ue4m0 -> fp32 matrix multiplication
 */
void matmul_fp32_ue4m0_ue4m0(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue5m0 x ue5m0 -> fp32 matrix multiplication
 */
void matmul_fp32_ue5m0_ue5m0(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue6m0 x ue6m0 -> fp32 matrix multiplication
 */
void matmul_fp32_ue6m0_ue6m0(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue7m0 x ue7m0 -> fp32 matrix multiplication
 */
void matmul_fp32_ue7m0_ue7m0(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue8m0 x ue8m0 -> fp32 matrix multiplication
 */
void matmul_fp32_ue8m0_ue8m0(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

// ======================================
// FP16 Output Functions
// ======================================

/**
 * @brief ue1m7 x ue1m7 -> fp16 matrix multiplication
 */
void matmul_fp16_ue1m7_ue1m7(void *input0, void *input1, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief e1m6 x e1m6 -> fp16 matrix multiplication
 */
void matmul_fp16_e1m6_e1m6(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e1m5 x e1m5 -> fp16 matrix multiplication
 */
void matmul_fp16_e1m5_e1m5(void *input1, void *input2, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e1m4 x e1m4 -> fp16 matrix multiplication
 */
void matmul_fp16_e1m4_e1m4(void *input1, void *input2, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e1m3 x e1m3 -> fp16 matrix multiplication
 */
void matmul_fp16_e1m3_e1m3(void *input1, void *input2, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e1m2 x e1m2 -> fp16 matrix multiplication
 */
void matmul_fp16_e1m2_e1m2(void *input1, void *input2, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e1m1 x e1m1 -> fp16 matrix multiplication
 */
void matmul_fp16_e1m1_e1m1(void *input1, void *input2, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e1m0 x e1m0 -> fp16 matrix multiplication
 */
void matmul_fp16_e1m0_e1m0(void *input1, void *input2, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief ue1m6 x ue1m6 -> fp16 matrix multiplication
 */
void matmul_fp16_ue1m6_ue1m6(void *input1, void *input2, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue2m6 x ue2m6 -> fp16 matrix multiplication
 */
void matmul_fp16_ue2m6_ue2m6(void *input1, void *input2, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief e2m5 x e2m5 -> fp16 matrix multiplication
 */
void matmul_fp16_e2m5_e2m5(void *input1, void *input2, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e2m4 x e2m4 -> fp16 matrix multiplication
 */
void matmul_fp16_e2m4_e2m4(void *input1, void *input2, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);
/**
 * @brief e2m3 x e2m3 -> fp16 matrix multiplication
 */
void matmul_fp16_e2m3_e2m3(void *input1, void *input2, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e2m2 x e2m2 -> fp16 matrix multiplication
 */
void matmul_fp16_e2m2_e2m2(void *input1, void *input2, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e2m1 x e2m1 -> fp16 matrix multiplication
 */
void matmul_fp16_e2m1_e2m1(void *input1, void *input2, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);
/**
 * @brief e2m0 x e2m0 -> fp16 matrix multiplication
 */
void matmul_fp16_e2m0_e2m0(void *input1, void *input2, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief ue1m5 x ue1m5 -> fp16 matrix multiplication
 */
void matmul_fp16_ue1m5_ue1m5(void *input1, void *input2, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue2m5 x ue2m5 -> fp16 matrix multiplication
 */
void matmul_fp16_ue2m5_ue2m5(void *input1, void *input2, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue3m5 x ue3m5 -> fp16 matrix multiplication
 */
void matmul_fp16_ue3m5_ue3m5(void *input1, void *input2, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief e3m4 x e3m4 -> fp16 matrix multiplication
 */
void matmul_fp16_e3m4_e3m4(void *input1, void *input2, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e3m3 x e3m3 -> fp16 matrix multiplication
 */
void matmul_fp16_e3m3_e3m3(void *input1, void *input2, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e3m2 x e3m2 -> fp16 matrix multiplication
 */
void matmul_fp16_e3m2_e3m2(void *input1, void *input2, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e3m1 x e3m1 -> fp16 matrix multiplication
 */
void matmul_fp16_e3m1_e3m1(void *input1, void *input2, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e3m0 x e3m0 -> fp16 matrix multiplication
 */
void matmul_fp16_e3m0_e3m0(void *input1, void *input2, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief ue1m4 x ue1m4 -> fp16 matrix multiplication
 */
void matmul_fp16_ue1m4_ue1m4(void *input1, void *input2, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue2m4 x ue2m4 -> fp16 matrix multiplication
 */
void matmul_fp16_ue2m4_ue2m4(void *input1, void *input2, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue3m4 x ue3m4 -> fp16 matrix multiplication
 */
void matmul_fp16_ue3m4_ue3m4(void *input1, void *input2, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue4m4 x ue4m4 -> fp16 matrix multiplication
 */
void matmul_fp16_ue4m4_ue4m4(void *input1, void *input2, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief e4m3 x e4m3 -> fp16 matrix multiplication
 */
void matmul_fp16_e4m3_e4m3(void *input1, void *input2, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e4m2 x e4m2 -> fp16 matrix multiplication
 */
void matmul_fp16_e4m2_e4m2(void *input1, void *input2, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e4m1 x e4m1 -> fp16 matrix multiplication
 */
void matmul_fp16_e4m1_e4m1(void *input1, void *input2, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e4m0 x e4m0 -> fp16 matrix multiplication
 */
void matmul_fp16_e4m0_e4m0(void *input1, void *input2, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);
/**
 * @brief ue1m3 x ue1m3 -> fp16 matrix multiplication
 */
void matmul_fp16_ue1m3_ue1m3(void *input1, void *input2, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue2m3 x ue2m3 -> fp16 matrix multiplication
 */
void matmul_fp16_ue2m3_ue2m3(void *input1, void *input2, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue3m3 x ue3m3 -> fp16 matrix multiplication
 */
void matmul_fp16_ue3m3_ue3m3(void *input1, void *input2, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue4m3 x ue4m3 -> fp16 matrix multiplication
 */
void matmul_fp16_ue4m3_ue4m3(void *input1, void *input2, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue5m3 x ue5m3 -> fp16 matrix multiplication
 */
void matmul_fp16_ue5m3_ue5m3(void *input1, void *input2, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief e5m2 x e5m2 -> fp16 matrix multiplication
 */
void matmul_fp16_e5m2_e5m2(void *input1, void *input2, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);
/**
 * @brief e5m1 x e5m1 -> fp16 matrix multiplication
 */
void matmul_fp16_e5m1_e5m1(void *input1, void *input2, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e5m0 x e5m0 -> fp16 matrix multiplication
 */
void matmul_fp16_e5m0_e5m0(void *input1, void *input2, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief ue1m2 x ue1m2 -> fp16 matrix multiplication
 */
void matmul_fp16_ue1m2_ue1m2(void *input1, void *input2, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue2m2 x ue2m2 -> fp16 matrix multiplication
 */
void matmul_fp16_ue2m2_ue2m2(void *input1, void *input2, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue3m2 x ue3m2 -> fp16 matrix multiplication
 */
void matmul_fp16_ue3m2_ue3m2(void *input1, void *input2, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue4m2 x ue4m2 -> fp16 matrix multiplication
 */
void matmul_fp16_ue4m2_ue4m2(void *input1, void *input2, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue5m2 x ue5m2 -> fp16 matrix multiplication
 */
void matmul_fp16_ue5m2_ue5m2(void *input1, void *input2, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue6m2 x ue6m2 -> fp16 matrix multiplication
 */
void matmul_fp16_ue6m2_ue6m2(void *input1, void *input2, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief e6m1 x e6m1 -> fp16 matrix multiplication
 */
void matmul_fp16_e6m1_e6m1(void *input1, void *input2, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief e6m0 x e6m0 -> fp16 matrix multiplication
 */
void matmul_fp16_e6m0_e6m0(void *input1, void *input2, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief ue1m1 x ue1m1 -> fp16 matrix multiplication
 */
void matmul_fp16_ue1m1_ue1m1(void *input1, void *input2, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);
/**
 * @brief ue2m1 x ue2m1 -> fp16 matrix multiplication
 */
void matmul_fp16_ue2m1_ue2m1(void *input1, void *input2, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue3m1 x ue3m1 -> fp16 matrix multiplication
 */
void matmul_fp16_ue3m1_ue3m1(void *input1, void *input2, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue4m1 x ue4m1 -> fp16 matrix multiplication
 */
void matmul_fp16_ue4m1_ue4m1(void *input1, void *input2, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue5m1 x ue5m1 -> fp16 matrix multiplication
 */
void matmul_fp16_ue5m1_ue5m1(void *input1, void *input2, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue6m1 x ue6m1 -> fp16 matrix multiplication
 */
void matmul_fp16_ue6m1_ue6m1(void *input1, void *input2, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue7m1 x ue7m1 -> fp16 matrix multiplication
 */
void matmul_fp16_ue7m1_ue7m1(void *input1, void *input2, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief e7m0 x e7m0 -> fp16 matrix multiplication
 */
void matmul_fp16_e7m0_e7m0(void *input1, void *input2, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief ue1m0 x ue1m0 -> fp16 matrix multiplication
 */
void matmul_fp16_ue1m0_ue1m0(void *input1, void *input2, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue2m0 x ue2m0 -> fp16 matrix multiplication
 */
void matmul_fp16_ue2m0_ue2m0(void *input1, void *input2, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue3m0 x ue3m0 -> fp16 matrix multiplication
 */
void matmul_fp16_ue3m0_ue3m0(void *input1, void *input2, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);
/**
 * @brief ue4m0 x ue4m0 -> fp16 matrix multiplication
 */
void matmul_fp16_ue4m0_ue4m0(void *input1, void *input2, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue5m0 x ue5m0 -> fp16 matrix multiplication
 */
void matmul_fp16_ue5m0_ue5m0(void *input1, void *input2, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue6m0 x ue6m0 -> fp16 matrix multiplication
 */
void matmul_fp16_ue6m0_ue6m0(void *input1, void *input2, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue7m0 x ue7m0 -> fp16 matrix multiplication
 */
void matmul_fp16_ue7m0_ue7m0(void *input1, void *input2, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

/**
 * @brief ue8m0 x ue8m0 -> fp16 matrix multiplication
 */
void matmul_fp16_ue8m0_ue8m0(void *input1, void *input2, void *output,
                             const MatMulConfig *config,
                             const MatMulStrideConfig *strides);

#endif

#endif /* CONFIG_CUSTOM_TENSOR */
