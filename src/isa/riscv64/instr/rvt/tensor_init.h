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

#ifndef TENSOR_INIT_H
#define TENSOR_INIT_H

#include "conv2d.h"
#include "matmul.h"
#include "matmul_subfloat.h"
#include "pooling.h"

#define MATMUL_INPUT_TYPE_U8 6
#define MATMUL_INPUT_TYPE_S8 7
#define MATMUL_OUTPUT_TYPE_S32 0
#define MATMUL_OUTPUT_TYPE_S16 1

// ====================================================
// Function Pointer Type Definitions
// ====================================================

/**
 * @brief Matrix multiplication function pointer type
 * @param input0 First input tensor pointer
 * @param input1 Second input tensor pointer
 * @param output Output tensor pointer
 * @param config Matrix multiplication configuration
 * @param strides Stride configuration for all tensors
 */
typedef void (*MatMulFunc)(void *input0, void *input1, void *output,
                           const MatMulConfig *config,
                           const MatMulStrideConfig *strides);

/**
 * @brief Convolution function pointer type
 * @param input Input tensor pointer
 * @param filter Filter/weight tensor pointer
 * @param output Output tensor pointer
 * @param config Convolution configuration
 * @param strides Stride configuration for all tensors
 */
typedef void (*Conv2DFunc)(void *input, void *filter, void *output,
                           const Conv2DConfig *config,
                           const Conv2DStrideConfig *strides);

/**
 * @brief Pooling function pointer type
 * @param input Input tensor pointer
 * @param output Output tensor pointer
 * @param config Pooling configuration
 * @param strides Stride configuration for input and output tensors
 */
typedef void (*PoolFunc)(void *input, void *output, const PoolingConfig *config,
                         const PoolingStrideConfig *strides);

/**
 * @brief Subfloat matrix multiplication function pointer type
 * @param input0 First input tensor pointer
 * @param input1 Second input tensor pointer
 * @param output Output tensor pointer
 * @param config Matrix multiplication configuration
 * @param strides Stride configuration for all tensors
 */
typedef void (*MatMulSubfloatFunc)(void *input0, void *input1, void *output,
                                   const MatMulConfig *config,
                                   const MatMulStrideConfig *strides);

// ====================================================
// Global Dispatch Table Declarations
// ====================================================

// Matrix multiplication dispatch table
// Format: matmul_float_table[input1_type][input2_type][output_type]
extern MatMulFunc matmul_float_table[MATMUL_INPUT_TYPE_COUNT]
                                    [MATMUL_INPUT_TYPE_COUNT]
                                    [MATMUL_OUTPUT_TYPE_COUNT];

// Convolution dispatch table
// Format: conv2d_float_table[input_type][filter_type][output_type]
extern Conv2DFunc conv2d_float_table[CONV2D_INPUT_TYPE_COUNT]
                                    [CONV2D_INPUT_TYPE_COUNT]
                                    [CONV2D_OUTPUT_TYPE_COUNT];

// Pooling dispatch table
// Format: pooling_float_table[pool_op][input_type][output_type]
extern PoolFunc pooling_float_table[POOL_OP_COUNT][POOLING_INPUT_TYPE_COUNT]
                                   [POOLING_OUTPUT_TYPE_COUNT];

// Subfloat matrix multiplication dispatch table
// Format: matmul_subfloat_table[input1_type][input2_type][output_type]
extern MatMulSubfloatFunc
    matmul_subfloat_table[MATMUL_SUBFLOAT_INPUT_TYPE_COUNT]
                         [MATMUL_SUBFLOAT_INPUT_TYPE_COUNT]
                         [MATMUL_SUBFLOAT_OUTPUT_TYPE_COUNT];

// ====================================================
// Initialization Status Flags
// ====================================================

extern bool has_init_matmul_float_table;
extern bool has_init_conv2d_float_table;
extern bool has_init_pooling_float_table;
extern bool has_init_matmul_subfloat_table;

// ====================================================
// Table Initialization Function Declarations
// ====================================================

/**
 * @brief Initialize matrix multiplication function dispatch table
 */
void init_matmul_float_table(void);

/**
 * @brief Initialize convolution function dispatch table
 */
void init_conv2d_float_table(void);

/**
 * @brief Initialize pooling function dispatch table
 */
void init_pooling_float_table(void);

/**
 * @brief Initialize subfloat matrix multiplication function dispatch table
 */
void init_matmul_subfloat_table(void);

// ====================================================
// Configuration Creation Helper Functions
// ====================================================

// Matrix multiplication configuration helpers
void create_matmul_config(MatMulConfig *config, int input0_dim0,
                          int input0_dim1, int input1_dim0, int input1_dim1);

void create_matmul_stride_config(MatMulStrideConfig *strides,
                                 int input0_stride0, int input0_stride1,
                                 int input1_stride0, int input1_stride1,
                                 int output_stride0, int output_stride1);

// Convolution configuration helpers
void create_conv2d_config(Conv2DConfig *config, int batch, int in_height,
                          int in_width, int in_channel, int filter_height,
                          int filter_width, int out_channel, int pad_height,
                          int pad_width, int stride_height, int stride_width);

void create_conv2d_stride_config(Conv2DStrideConfig *strides,
                                 int input_stride_b, int input_stride_h,
                                 int input_stride_w, int input_stride_c,
                                 int filter_stride_oc, int filter_stride_h,
                                 int filter_stride_w, int filter_stride_ic,
                                 int output_stride_b, int output_stride_h,
                                 int output_stride_w, int output_stride_c);

// Pooling configuration helpers
void create_pooling_config(PoolingConfig *config, int batch, int in_height,
                           int in_width, int in_channel, int kernel_h,
                           int kernel_w, int pad_height, int pad_width,
                           int stride_height, int stride_width, int is_max);

void create_pooling_stride_config(PoolingStrideConfig *strides,
                                  int input_stride_b, int input_stride_h,
                                  int input_stride_w, int input_stride_c,
                                  int output_stride_b, int output_stride_h,
                                  int output_stride_w, int output_stride_c);

#endif /* TENSOR_INIT_H */

#endif /* CONFIG_CUSTOM_TENSOR */
