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

#include "tensor_init.h"
#include <stdio.h>
#include <string.h>

// ====================================================
// Global Dispatch Table Definitions
// ====================================================

MatMulFunc matmul_float_table[MATMUL_INPUT_TYPE_COUNT][MATMUL_INPUT_TYPE_COUNT]
                             [MATMUL_OUTPUT_TYPE_COUNT] = {{{0}}};

Conv2DFunc conv2d_float_table[CONV2D_INPUT_TYPE_COUNT][CONV2D_INPUT_TYPE_COUNT]
                             [CONV2D_OUTPUT_TYPE_COUNT] = {{{0}}};

PoolFunc pooling_float_table[POOL_OP_COUNT][POOLING_INPUT_TYPE_COUNT]
                            [POOLING_OUTPUT_TYPE_COUNT] = {{{0}}};

MatMulSubfloatFunc matmul_subfloat_table[MATMUL_SUBFLOAT_INPUT_TYPE_COUNT]
                                        [MATMUL_SUBFLOAT_INPUT_TYPE_COUNT]
                                        [MATMUL_SUBFLOAT_OUTPUT_TYPE_COUNT] = {
                                            {{0}}};

// ====================================================
// Initialization Status Flags Definitions
// ====================================================

bool has_init_matmul_float_table = 0;
bool has_init_conv2d_float_table = 0;
bool has_init_pooling_float_table = 0;
bool has_init_matmul_subfloat_table = 0;

// ====================================================
// Macro Definitions for Table Initialization
// ====================================================

/**
 * @brief Initialize a matrix multiplication table entry
 * Maps: (input_type1, input_type2, output_type) -> function
 */
#define INIT_MATMUL_FLOAT_TABLE_ENTRY(rs1type, rs2type, rs3type)               \
  matmul_float_table[MATMUL_INPUT_TYPE_##rs1type][MATMUL_INPUT_TYPE_##rs2type] \
                    [MATMUL_OUTPUT_TYPE_##rs3type] =                           \
                        matmul_##rs3type##_##rs1type##_##rs2type

/**
 * @brief Initialize a convolution table entry
 * Maps: (input_type, filter_type, output_type) -> function
 */
#define INIT_CONV2D_FLOAT_TABLE_ENTRY(rs1type, rs2type, rs3type)               \
  conv2d_float_table[CONV2D_INPUT_TYPE_##rs1type][CONV2D_INPUT_TYPE_##rs2type] \
                    [CONV2D_OUTPUT_TYPE_##rs3type] =                           \
                        conv2d_##rs3type##_##rs1type##_##rs2type

/**
 * @brief Initialize a pooling table entry
 * Maps: (pool_op, input_type, output_type) -> function
 */
#define INIT_POOLING_FLOAT_TABLE_ENTRY(optype, rs1type, rs3type)               \
  pooling_float_table[POOL_OP_##optype][POOLING_INPUT_TYPE_##rs1type]          \
                     [POOLING_OUTPUT_TYPE_##rs3type] =                         \
                         optype##_##rs3type##_##rs1type

/**
 * @brief Initialize a subfloat matrix multiplication table entry
 * Maps: (input_type1, input_type2, output_type) -> function
 */
#define INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(type1, type2, out_type)               \
  matmul_subfloat_table[MATMUL_INPUT_TYPE_##type1][MATMUL_INPUT_TYPE_##type2]  \
                       [MATMUL_SUBFLOAT_OUTPUT_TYPE_##out_type] =              \
                           matmul_##out_type##_##type1##_##type2

// ====================================================
// Table Initialization Functions
// ====================================================

/**
 * @brief Initialize matrix multiplication function dispatch table
 * This table maps (input1_type, input2_type, output_type) to the appropriate
 * matrix multiplication function for floating-point types.
 */
void init_matmul_float_table(void) {
  if (has_init_matmul_float_table)
    return;

  // FP32 output types
  INIT_MATMUL_FLOAT_TABLE_ENTRY(fp16, fp16, fp32);
  INIT_MATMUL_FLOAT_TABLE_ENTRY(fp16, bf16, fp32);
  INIT_MATMUL_FLOAT_TABLE_ENTRY(fp16, tf32, fp32);
  INIT_MATMUL_FLOAT_TABLE_ENTRY(bf16, fp16, fp32);
  INIT_MATMUL_FLOAT_TABLE_ENTRY(bf16, bf16, fp32);
  INIT_MATMUL_FLOAT_TABLE_ENTRY(bf16, tf32, fp32);
  INIT_MATMUL_FLOAT_TABLE_ENTRY(tf32, fp16, fp32);
  INIT_MATMUL_FLOAT_TABLE_ENTRY(tf32, bf16, fp32);
  INIT_MATMUL_FLOAT_TABLE_ENTRY(tf32, tf32, fp32);
  INIT_MATMUL_FLOAT_TABLE_ENTRY(fp32, fp32, fp32);

  // FP16 output types
  INIT_MATMUL_FLOAT_TABLE_ENTRY(fp16, fp16, fp16);
  INIT_MATMUL_FLOAT_TABLE_ENTRY(fp16, bf16, fp16);
  INIT_MATMUL_FLOAT_TABLE_ENTRY(fp16, tf32, fp16);
  INIT_MATMUL_FLOAT_TABLE_ENTRY(bf16, fp16, fp16);
  INIT_MATMUL_FLOAT_TABLE_ENTRY(bf16, bf16, fp16);
  INIT_MATMUL_FLOAT_TABLE_ENTRY(bf16, tf32, fp16);
  INIT_MATMUL_FLOAT_TABLE_ENTRY(tf32, fp16, fp16);
  INIT_MATMUL_FLOAT_TABLE_ENTRY(tf32, bf16, fp16);
  INIT_MATMUL_FLOAT_TABLE_ENTRY(tf32, tf32, fp16);

  has_init_matmul_float_table = 1;
}

/**
 * @brief Initialize convolution function dispatch table
 * This table maps (input_type, filter_type, output_type) to the appropriate
 * convolution function.
 */
void init_conv2d_float_table(void) {
  if (has_init_conv2d_float_table)
    return;

  INIT_CONV2D_FLOAT_TABLE_ENTRY(fp16, fp16, fp32);
  INIT_CONV2D_FLOAT_TABLE_ENTRY(fp16, fp16, fp16);
  INIT_CONV2D_FLOAT_TABLE_ENTRY(fp32, fp32, fp32);

  has_init_conv2d_float_table = 1;
}

/**
 * @brief Initialize pooling function dispatch table
 * This table maps (pool_operation, input_type, output_type) to the appropriate
 * pooling function.
 */
void init_pooling_float_table(void) {
  if (has_init_pooling_float_table)
    return;

  // Max pooling operations
  INIT_POOLING_FLOAT_TABLE_ENTRY(maxpool, fp16, fp16);
  // INIT_POOLING_FLOAT_TABLE_ENTRY(maxpool, fp32, fp32);

  // Average pooling operations
  INIT_POOLING_FLOAT_TABLE_ENTRY(avgpool, fp16, fp16);
  // INIT_POOLING_FLOAT_TABLE_ENTRY(avgpool, fp32, fp32);

  has_init_pooling_float_table = 1;
}

/**
 * @brief Initialize subfloat matrix multiplication function dispatch table
 * This table maps (input1_type, input2_type, output_type) to the appropriate
 * subfloat matrix multiplication function.
 */
void init_matmul_subfloat_table(void) {
  if (has_init_matmul_subfloat_table)
    return;
  /* ue1m7 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue1m7, ue1m7, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue1m7, ue1m7, fp16);

  /* e1m6 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e1m6, e1m6, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e1m6, e1m6, fp16);

  /* e1m5 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e1m5, e1m5, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e1m5, e1m5, fp16);

  /* e1m4 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e1m4, e1m4, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e1m4, e1m4, fp16);

  /* e1m3 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e1m3, e1m3, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e1m3, e1m3, fp16);

  /* e1m2 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e1m2, e1m2, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e1m2, e1m2, fp16);

  /* e1m1 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e1m1, e1m1, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e1m1, e1m1, fp16);

  /* e1m0 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e1m0, e1m0, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e1m0, e1m0, fp16);

  /* ue1m6 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue1m6, ue1m6, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue1m6, ue1m6, fp16);

  /* ue2m6 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue2m6, ue2m6, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue2m6, ue2m6, fp16);

  /* e2m5 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e2m5, e2m5, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e2m5, e2m5, fp16);

  /* e2m4 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e2m4, e2m4, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e2m4, e2m4, fp16);

  /* e2m3 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e2m3, e2m3, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e2m3, e2m3, fp16);

  /* e2m2 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e2m2, e2m2, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e2m2, e2m2, fp16);

  /* e2m1 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e2m1, e2m1, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e2m1, e2m1, fp16);

  /* e2m0 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e2m0, e2m0, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e2m0, e2m0, fp16);

  /* ue1m5 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue1m5, ue1m5, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue1m5, ue1m5, fp16);

  /* ue2m5 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue2m5, ue2m5, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue2m5, ue2m5, fp16);

  /* ue3m5 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue3m5, ue3m5, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue3m5, ue3m5, fp16);

  /* e3m4 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e3m4, e3m4, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e3m4, e3m4, fp16);

  /* e3m3 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e3m3, e3m3, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e3m3, e3m3, fp16);

  /* e3m2 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e3m2, e3m2, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e3m2, e3m2, fp16);

  /* e3m1 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e3m1, e3m1, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e3m1, e3m1, fp16);

  /* e3m0 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e3m0, e3m0, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e3m0, e3m0, fp16);

  /* ue1m4 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue1m4, ue1m4, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue1m4, ue1m4, fp16);

  /* ue2m4 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue2m4, ue2m4, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue2m4, ue2m4, fp16);

  /* ue3m4 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue3m4, ue3m4, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue3m4, ue3m4, fp16);

  /* ue4m4 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue4m4, ue4m4, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue4m4, ue4m4, fp16);

  /* e4m3 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e4m3, e4m3, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e4m3, e4m3, fp16);

  /* e4m2 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e4m2, e4m2, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e4m2, e4m2, fp16);

  /* e4m1 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e4m1, e4m1, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e4m1, e4m1, fp16);

  /* e4m0 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e4m0, e4m0, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e4m0, e4m0, fp16);

  /* ue1m3 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue1m3, ue1m3, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue1m3, ue1m3, fp16);

  /* ue2m3 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue2m3, ue2m3, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue2m3, ue2m3, fp16);

  /* ue3m3 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue3m3, ue3m3, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue3m3, ue3m3, fp16);

  /* ue4m3 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue4m3, ue4m3, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue4m3, ue4m3, fp16);

  /* ue5m3 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue5m3, ue5m3, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue5m3, ue5m3, fp16);

  /* e5m2 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e5m2, e5m2, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e5m2, e5m2, fp16);

  /* e5m1 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e5m1, e5m1, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e5m1, e5m1, fp16);

  /* e5m0 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e5m0, e5m0, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e5m0, e5m0, fp16);

  /* ue1m2 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue1m2, ue1m2, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue1m2, ue1m2, fp16);

  /* ue2m2 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue2m2, ue2m2, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue2m2, ue2m2, fp16);

  /* ue3m2 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue3m2, ue3m2, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue3m2, ue3m2, fp16);

  /* ue4m2 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue4m2, ue4m2, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue4m2, ue4m2, fp16);

  /* ue5m2 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue5m2, ue5m2, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue5m2, ue5m2, fp16);

  /* ue6m2 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue6m2, ue6m2, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue6m2, ue6m2, fp16);

  /* e6m1 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e6m1, e6m1, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e6m1, e6m1, fp16);

  /* e6m0 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e6m0, e6m0, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e6m0, e6m0, fp16);

  /* ue1m1 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue1m1, ue1m1, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue1m1, ue1m1, fp16);

  /* ue2m1 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue2m1, ue2m1, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue2m1, ue2m1, fp16);

  /* ue3m1 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue3m1, ue3m1, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue3m1, ue3m1, fp16);

  /* ue4m1 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue4m1, ue4m1, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue4m1, ue4m1, fp16);

  /* ue5m1 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue5m1, ue5m1, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue5m1, ue5m1, fp16);

  /* ue6m1 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue6m1, ue6m1, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue6m1, ue6m1, fp16);

  /* ue7m1 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue7m1, ue7m1, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue7m1, ue7m1, fp16);

  /* e7m0 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e7m0, e7m0, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e7m0, e7m0, fp16);

  /* ue1m0 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue1m0, ue1m0, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue1m0, ue1m0, fp16);

  /* ue2m0 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue2m0, ue2m0, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue2m0, ue2m0, fp16);

  /* ue3m0 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue3m0, ue3m0, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue3m0, ue3m0, fp16);

  /* ue4m0 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue4m0, ue4m0, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue4m0, ue4m0, fp16);

  /* ue5m0 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue5m0, ue5m0, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue5m0, ue5m0, fp16);

  /* ue6m0 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue6m0, ue6m0, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue6m0, ue6m0, fp16);

  /* ue7m0 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue7m0, ue7m0, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue7m0, ue7m0, fp16);

  /* ue8m0 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue8m0, ue8m0, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue8m0, ue8m0, fp16);

  has_init_matmul_subfloat_table = 1;
}

// ====================================================
// Configuration Creation Helper Functions
// ====================================================

/**
 * @brief Create matrix multiplication configuration structure
 * @param config Pointer to configuration structure to fill
 * @param input0_dim0 First dimension of first input matrix
 * @param input0_dim1 Second dimension of first input matrix
 * @param input1_dim0 First dimension of second input matrix
 * @param input1_dim1 Second dimension of second input matrix
 */
void create_matmul_config(MatMulConfig *config, int input0_dim0,
                          int input0_dim1, int input1_dim0, int input1_dim1) {
  config->input0_dim0 = input0_dim0;
  config->input0_dim1 = input0_dim1;
  config->input1_dim0 = input1_dim0;
  config->input1_dim1 = input1_dim1;
}

/**
 * @brief Create matrix multiplication stride configuration structure
 * @param strides Pointer to stride configuration structure to fill
 * @param input0_stride0 First input matrix first dimension stride
 * @param input0_stride1 First input matrix second dimension stride
 * @param input1_stride0 Second input matrix first dimension stride
 * @param input1_stride1 Second input matrix second dimension stride
 * @param output_stride0 Output matrix first dimension stride
 * @param output_stride1 Output matrix second dimension stride
 */
void create_matmul_stride_config(MatMulStrideConfig *strides,
                                 int input0_stride0, int input0_stride1,
                                 int input1_stride0, int input1_stride1,
                                 int output_stride0, int output_stride1) {
  strides->input0_stride0 = input0_stride0;
  strides->input0_stride1 = input0_stride1;
  strides->input1_stride0 = input1_stride0;
  strides->input1_stride1 = input1_stride1;
  strides->output_stride0 = output_stride0;
  strides->output_stride1 = output_stride1;
}

/**
 * @brief Create convolution configuration structure
 * @param config Pointer to configuration structure to fill
 * @param batch Batch size
 * @param in_height Input tensor height
 * @param in_width Input tensor width
 * @param in_channel Input tensor channels
 * @param filter_height Filter height
 * @param filter_width Filter width
 * @param out_channel Output channels/filters
 * @param pad_height Height padding
 * @param pad_width Width padding
 * @param stride_height Height stride
 * @param stride_width Width stride
 */
void create_conv2d_config(Conv2DConfig *config, int batch, int in_height,
                          int in_width, int in_channel, int filter_height,
                          int filter_width, int out_channel, int pad_height,
                          int pad_width, int stride_height, int stride_width) {
  config->batch = batch;
  config->in_height = in_height;
  config->in_width = in_width;
  config->in_channel = in_channel;
  config->filter_height = filter_height;
  config->filter_width = filter_width;
  config->out_channel = out_channel;
  config->pad_height = pad_height;
  config->pad_width = pad_width;
  config->stride_height = stride_height;
  config->stride_width = stride_width;
}

/**
 * @brief Create convolution stride configuration structure
 * @param strides Pointer to stride configuration structure to fill
 * @param input_stride_b Input batch dimension stride
 * @param input_stride_h Input height dimension stride
 * @param input_stride_w Input width dimension stride
 * @param input_stride_c Input channel dimension stride
 * @param filter_stride_oc Filter output channel dimension stride
 * @param filter_stride_h Filter height dimension stride
 * @param filter_stride_w Filter width dimension stride
 * @param filter_stride_ic Filter input channel dimension stride
 * @param output_stride_b Output batch dimension stride
 * @param output_stride_h Output height dimension stride
 * @param output_stride_w Output width dimension stride
 * @param output_stride_c Output channel dimension stride
 */
void create_conv2d_stride_config(Conv2DStrideConfig *strides,
                                 int input_stride_b, int input_stride_h,
                                 int input_stride_w, int input_stride_c,
                                 int filter_stride_oc, int filter_stride_h,
                                 int filter_stride_w, int filter_stride_ic,
                                 int output_stride_b, int output_stride_h,
                                 int output_stride_w, int output_stride_c) {
  strides->input_stride_b = input_stride_b;
  strides->input_stride_h = input_stride_h;
  strides->input_stride_w = input_stride_w;
  strides->input_stride_c = input_stride_c;
  strides->filter_stride_oc = filter_stride_oc;
  strides->filter_stride_h = filter_stride_h;
  strides->filter_stride_w = filter_stride_w;
  strides->filter_stride_ic = filter_stride_ic;
  strides->output_stride_b = output_stride_b;
  strides->output_stride_h = output_stride_h;
  strides->output_stride_w = output_stride_w;
  strides->output_stride_c = output_stride_c;
}

/**
 * @brief Create pooling configuration structure
 * @param config Pointer to configuration structure to fill
 * @param batch Batch size
 * @param in_height Input tensor height
 * @param in_width Input tensor width
 * @param in_channel Input tensor channels
 * @param kernel_h Pooling kernel height
 * @param kernel_w Pooling kernel width
 * @param pad_height Height padding
 * @param pad_width Width padding
 * @param stride_height Height stride
 * @param stride_width Width stride
 * @param is_max 1 for max pooling, 0 for average pooling
 */
void create_pooling_config(PoolingConfig *config, int batch, int in_height,
                           int in_width, int in_channel, int kernel_h,
                           int kernel_w, int pad_height, int pad_width,
                           int stride_height, int stride_width, int is_max) {
  config->batch = batch;
  config->in_height = in_height;
  config->in_width = in_width;
  config->in_channel = in_channel;
  config->kernel_h = kernel_h;
  config->kernel_w = kernel_w;
  config->pad_height = pad_height;
  config->pad_width = pad_width;
  config->stride_height = stride_height;
  config->stride_width = stride_width;
  config->is_max = is_max;
}

/**
 * @brief Create pooling stride configuration structure
 * @param strides Pointer to stride configuration structure to fill
 * @param input_stride_b Input batch dimension stride
 * @param input_stride_h Input height dimension stride
 * @param input_stride_w Input width dimension stride
 * @param input_stride_c Input channel dimension stride
 * @param output_stride_b Output batch dimension stride
 * @param output_stride_h Output height dimension stride
 * @param output_stride_w Output width dimension stride
 * @param output_stride_c Output channel dimension stride
 */
void create_pooling_stride_config(PoolingStrideConfig *strides,
                                  int input_stride_b, int input_stride_h,
                                  int input_stride_w, int input_stride_c,
                                  int output_stride_b, int output_stride_h,
                                  int output_stride_w, int output_stride_c) {
  strides->input_stride_b = input_stride_b;
  strides->input_stride_h = input_stride_h;
  strides->input_stride_w = input_stride_w;
  strides->input_stride_c = input_stride_c;
  strides->output_stride_b = output_stride_b;
  strides->output_stride_h = output_stride_h;
  strides->output_stride_w = output_stride_w;
  strides->output_stride_c = output_stride_c;
}

/**
 * @brief Create subfloat matrix multiplication configuration structure
 * @param config Pointer to configuration structure to fill
 * @param input0_dim0 First dimension of first input matrix
 * @param input0_dim1 Second dimension of first input matrix
 * @param input1_dim0 First dimension of second input matrix
 * @param input1_dim1 Second dimension of second input matrix
 */
void create_matmul_subfloat_config(MatMulConfig *config,
                                   int input0_dim0, int input0_dim1,
                                   int input1_dim0, int input1_dim1) {
  config->input0_dim0 = input0_dim0;
  config->input0_dim1 = input0_dim1;
  config->input1_dim0 = input1_dim0;
  config->input1_dim1 = input1_dim1;
}

/**
 * @brief Create subfloat matrix multiplication stride configuration structure
 * @param strides Pointer to stride configuration structure to fill
 * @param input0_stride0 First input matrix first dimension stride
 * @param input0_stride1 First input matrix second dimension stride
 * @param input1_stride0 Second input matrix first dimension stride
 * @param input1_stride1 Second input matrix second dimension stride
 * @param output_stride0 Output matrix first dimension stride
 * @param output_stride1 Output matrix second dimension stride
 */
void create_matmul_subfloat_stride_config(
    MatMulStrideConfig *strides, int input0_stride0, int input0_stride1,
    int input1_stride0, int input1_stride1, int output_stride0,
    int output_stride1) {
  strides->input0_stride0 = input0_stride0;
  strides->input0_stride1 = input0_stride1;
  strides->input1_stride0 = input1_stride0;
  strides->input1_stride1 = input1_stride1;
  strides->output_stride0 = output_stride0;
  strides->output_stride1 = output_stride1;
}

#endif /* CONFIG_CUSTOM_TENSOR */
