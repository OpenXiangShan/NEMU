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

#ifndef POOLING_H
#define POOLING_H

#include <memory/paddr.h>
#include <stdint.h>
#include <stdio.h>

enum {
  POOLING_INPUT_TYPE_fp16 = 0,
  POOLING_INPUT_TYPE_bf16 = 1,
  POOLING_INPUT_TYPE_tf32 = 3,
  POOLING_INPUT_TYPE_COUNT = 5,
};

enum {
  POOLING_OUTPUT_TYPE_fp32 = 0,
  POOLING_OUTPUT_TYPE_fp16 = 1,
  POOLING_OUTPUT_TYPE_COUNT = 2,
};

enum {
  POOL_OP_maxpool = 0,
  POOL_OP_avgpool = 1,
  POOL_OP_COUNT = 2,
};

/**
 * @brief Pooling stride configuration structure
 *
 * Encapsulates stride parameters for pooling operations.
 */
typedef struct {
  // Input tensor strides
  int input_stride_b; /* Input batch dimension stride */
  int input_stride_h; /* Input height dimension stride */
  int input_stride_w; /* Input width dimension stride */
  int input_stride_c; /* Input channel dimension stride */

  // Output tensor strides
  int output_stride_b; /* Output batch dimension stride */
  int output_stride_h; /* Output height dimension stride */
  int output_stride_w; /* Output width dimension stride */
  int output_stride_c; /* Output channel dimension stride */
} PoolingStrideConfig;

/**
 * @brief Pooling operation configuration structure
 *
 * Encapsulates pooling operation parameters
 */
typedef struct {
  // Tensor dimensions
  int batch;      /* Batch size */
  int in_height;  /* Input tensor height */
  int in_width;   /* Input tensor width */
  int in_channel; /* Input tensor channels */

  // Pooling kernel parameters
  int kernel_h; /* Pooling kernel height */
  int kernel_w; /* Pooling kernel width */

  // Pooling operation parameters
  int pad_height;    /* Height padding */
  int pad_width;     /* Width padding */
  int stride_height; /* Height stride */
  int stride_width;  /* Width stride */

  // Pooling type
  int is_max; /* 1 for max pooling, 0 for average pooling */
} PoolingConfig;

/**
 * @brief Max pooling operation with FP16 input and FP16 output
 *
 * @param input Pointer to input tensor
 * @param output Pointer to output tensor
 * @param config Pooling configuration parameters
 * @param strides Stride configuration for input and output tensors
 */
void maxpool_fp16_fp16(void *input, void *output, const PoolingConfig *config,
                       const PoolingStrideConfig *strides);

/**
 * @brief Average pooling operation with FP16 input and FP16 output
 *
 * @param input Pointer to input tensor
 * @param output Pointer to output tensor
 * @param config Pooling configuration parameters
 * @param strides Stride configuration for input and output tensors
 */
void avgpool_fp16_fp16(void *input, void *output, const PoolingConfig *config,
                       const PoolingStrideConfig *strides);

#endif /* POOLING_H */

#endif /* CONFIG_CUSTOM_TENSOR */
