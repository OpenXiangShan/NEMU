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

#ifndef CONV2D_H
#define CONV2D_H

#include <memory/paddr.h>
#include <stdint.h>
#include <stdio.h>

/**
 * @brief Input data type enumerations
 *
 * Defines supported input data types for convolution operations.
 * The naming convention follows: CONV2D_INPUT_TYPE_[datatype]
 */
enum {
  CONV2D_INPUT_TYPE_fp16 = 0,  /* 16-bit half precision floating point */
  CONV2D_INPUT_TYPE_bf16 = 1,  /* 16-bit brain floating point */
  CONV2D_INPUT_TYPE_fp32 = 2,  /* 32-bit single precision floating point */
  CONV2D_INPUT_TYPE_tf32 = 3,  /* 19-bit TensorFloat-32 */
  CONV2D_INPUT_TYPE_COUNT = 5, /* Total count of input data types */
};

/**
 * @brief Output data type enumerations
 *
 * Defines supported output data types for convolution operations.
 * The naming convention follows: CONV2D_OUTPUT_TYPE_[datatype]
 */
enum {
  CONV2D_OUTPUT_TYPE_fp32 = 0,  /* 32-bit single precision floating point */
  CONV2D_OUTPUT_TYPE_fp16 = 1,  /* 16-bit half precision floating point */
  CONV2D_OUTPUT_TYPE_COUNT = 2, /* Total count of output data types */
};

/**
 * @brief Convolution stride configuration structure
 *
 * Encapsulates all stride parameters for input, filter, and output tensors.
 * This reduces the number of function parameters and improves code readability.
 */
typedef struct {
  // Input tensor strides
  int input_stride_b; /* Input batch dimension stride */
  int input_stride_h; /* Input height dimension stride */
  int input_stride_w; /* Input width dimension stride */
  int input_stride_c; /* Input channel dimension stride */

  // Filter tensor strides
  int filter_stride_oc; /* Filter output channel dimension stride */
  int filter_stride_h;  /* Filter height dimension stride */
  int filter_stride_w;  /* Filter width dimension stride */
  int filter_stride_ic; /* Filter input channel dimension stride */

  // Output tensor strides
  int output_stride_b; /* Output batch dimension stride */
  int output_stride_h; /* Output height dimension stride */
  int output_stride_w; /* Output width dimension stride */
  int output_stride_c; /* Output channel dimension stride */
} Conv2DStrideConfig;

/**
 * @brief Convolution operation configuration structure
 *
 * Encapsulates all convolution operation parameters.
 */
typedef struct {
  // Tensor dimensions
  int batch;         /* Batch size */
  int in_height;     /* Input tensor */
  int in_width;      /* Input tensor width */
  int in_channel;    /* Input tensor channels */
  int filter_height; /* Filter height */
  int filter_width;  /* Filter width */
  int out_channel;   /* Output channels/filters */

  // Convolution parameters
  int pad_height;    /* Height padding */
  int pad_width;     /* Width padding */
  int stride_height; /* Height stride */
  int stride_width;  /* Width stride */
} Conv2DConfig;

/**
 * @brief Convolution with FP16 input, FP16 filter, FP16 output
 *
 * @param input Pointer to input tensor data (FP16 format)
 * @param filter Pointer to filter/weight tensor data (FP16 format)
 * @param output Pointer to output tensor data (FP16 format)
 * @param config Convolution configuration parameters
 * @param strides Stride configuration for all tensors
 *
 * @note Output height = floor((in_height + 2*pad_height -
 * filter_height)/stride_height) + 1
 * @note Output width = floor((in_width + 2*pad_width -
 * filter_width)/stride_width) + 1
 */
void conv2d_fp16_fp16_fp16(void *input, void *filter, void *output,
                           const Conv2DConfig *config,
                           const Conv2DStrideConfig *strides);

/**
 * @brief Convolution with FP16 input, FP16 filter, FP32 output
 *
 * @param input Pointer to input tensor data (FP16 format)
 * @param filter Pointer to filter/weight tensor data (FP16 format)
 * @param output Pointer to output tensor data (FP32 format)
 * @param config Convolution configuration parameters
 * @param strides Stride configuration for all tensors
 */
void conv2d_fp32_fp16_fp16(void *input, void *filter, void *output,
                           const Conv2DConfig *config,
                           const Conv2DStrideConfig *strides);

/**
 * @brief Convolution with FP32 input, FP32 filter, FP32 output
 *
 * @param input Pointer to input tensor data (FP32 format)
 * @param filter Pointer to filter/weight tensor data (FP32 format)
 * @param output Pointer to output tensor data (FP32 format)
 * @param config Convolution configuration parameters
 * @param strides Stride configuration for all tensors
 */
void conv2d_fp32_fp32_fp32(void *input, void *filter, void *output,
                           const Conv2DConfig *config,
                           const Conv2DStrideConfig *strides);

#endif /* CONV2D_H */

#endif /* CONFIG_CUSTOM_TENSOR */
