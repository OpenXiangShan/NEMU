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
  CONV2D_INPUT_TYPE_fp16 = 0,  /**< 16-bit half precision floating point */
  CONV2D_INPUT_TYPE_bf16 = 1,  /**< 16-bit brain floating point */
  CONV2D_INPUT_TYPE_fp32 = 2,  /**< 32-bit single precision floating point */
  CONV2D_INPUT_TYPE_tf32 = 3,  /**< 19-bit TensorFloat-32 */
  CONV2D_INPUT_TYPE_COUNT = 5, /**< Total count of input data types */
};

/**
 * @brief Output data type enumerations
 *
 * Defines supported output data types for convolution operations.
 * The naming convention follows: CONV2D_OUTPUT_TYPE_[datatype]
 */
enum {
  CONV2D_OUTPUT_TYPE_fp32 = 0,  /**< 32-bit single precision floating point */
  CONV2D_OUTPUT_TYPE_fp16 = 1,  /**< 16-bit half precision floating point */
  CONV2D_OUTPUT_TYPE_COUNT = 2, /**< Total count of output data types */
};

/**
 * @brief Convolution with FP16 input, FP16 filter, FP16 output
 *
 * Performs 2D convolution operation where:
 * - Input tensor: FP16 (16-bit half precision)
 * - Filter tensor: FP16 (16-bit half precision)
 * - Output tensor: FP16 (16-bit half precision)
 *
 * Computation is done in FP32 precision internally for accuracy.
 *
 * @param input Pointer to input tensor data (FP16 format)
 * @param filter Pointer to filter/weight tensor data (FP16 format)
 * @param output Pointer to output tensor data (FP16 format)
 * @param batch Batch size (typical range: 1-128)
 * @param in_height Input tensor height (typical range: 1-1024)
 * @param in_width Input tensor width (typical range: 1-1024)
 * @param in_channel Input tensor channels (typical range: 1-512)
 * @param filter_height Filter height (typical range: 1-11)
 * @param filter_width Filter width (typical range: 1-11)
 * @param out_channel Output channels/filters (typical range: 1-512)
 * @param pad_height Height padding (typical range: 0-5)
 * @param pad_width Width padding (typical range: 0-5)
 * @param stride_height Height stride (typical range: 1-4)
 * @param stride_width Width stride (typical range: 1-4)
 * @param input_stride_b Input batch dimension stride (in elements)
 * @param input_stride_h Input height dimension stride (in elements)
 * @param input_stride_w Input width dimension stride (in elements)
 * @param input_stride_c Input channel dimension stride (in elements)
 * @param filter_stride_oc Filter output channel dimension stride (in elements)
 * @param filter_stride_h Filter height dimension stride (in elements)
 * @param filter_stride_w Filter width dimension stride (in elements)
 * @param filter_stride_ic Filter input channel dimension stride (in elements)
 * @param output_stride_b Output batch dimension stride (in elements)
 * @param output_stride_h Output height dimension stride (in elements)
 * @param output_stride_w Output width dimension stride (in elements)
 * @param output_stride_c Output channel dimension stride (in elements)
 *
 * @note Output height = floor((in_height + 2*pad_height -
 * filter_height)/stride_height) + 1
 * @note Output width = floor((in_width + 2*pad_width -
 * filter_width)/stride_width) + 1
 */
void conv2d_fp16_fp16_fp16(void *input, void *filter, void *output, int batch,
                           int in_height, int in_width, int in_channel,
                           int filter_height, int filter_width, int out_channel,
                           int pad_height, int pad_width, int stride_height,
                           int stride_width, int input_stride_b,
                           int input_stride_h, int input_stride_w,
                           int input_stride_c, int filter_stride_oc,
                           int filter_stride_h, int filter_stride_w,
                           int filter_stride_ic, int output_stride_b,
                           int output_stride_h, int output_stride_w,
                           int output_stride_c);

/**
 * @brief Convolution with FP16 input, FP16 filter, FP32 output
 *
 * Performs 2D convolution operation where:
 * - Input tensor: FP16 (16-bit half precision)
 * - Filter tensor: FP16 (16-bit half precision)
 * - Output tensor: FP32 (32-bit single precision)
 *
 * Useful for mixed-precision training where higher precision accumulation is
 * needed.
 *
 * @param input Pointer to input tensor data (FP16 format)
 * @param filter Pointer to filter/weight tensor data (FP16 format)
 * @param output Pointer to output tensor data (FP32 format)
 * @param batch Batch size (typical range: 1-128)
 * @param in_height Input tensor height (typical range: 1-1024)
 * @param in_width Input tensor width (typical range: 1-1024)
 * @param in_channel Input tensor channels (typical range: 1-512)
 * @param filter_height Filter height (typical range: 1-11)
 * @param filter_width Filter width (typical range: 1-11)
 * @param out_channel Output channels/filters (typical range: 1-512)
 * @param pad_height Height padding (typical range: 0-5)
 * @param pad_width Width padding (typical range: 0-5)
 * @param stride_height Height stride (typical range: 1-4)
 * @param stride_width Width stride (typical range: 1-4)
 * @param input_stride_b Input batch dimension stride (in elements)
 * @param input_stride_h Input height dimension stride (in elements)
 * @param input_stride_w Input width dimension stride (in elements)
 * @param input_stride_c Input channel dimension stride (in elements)
 * @param filter_stride_oc Filter output channel dimension stride (in elements)
 * @param filter_stride_h Filter height dimension stride (in elements)
 * @param filter_stride_w Filter width dimension stride (in elements)
 * @param filter_stride_ic Filter input channel dimension stride (in elements)
 * @param output_stride_b Output batch dimension stride (in elements)
 * @param output_stride_h Output height dimension stride (in elements)
 * @param output_stride_w Output width dimension stride (in elements)
 * @param output_stride_c Output channel dimension stride (in elements)
 *
 * @note This version provides higher precision accumulation than fp16_fp16_fp16
 */
void conv2d_fp32_fp16_fp16(void *input, void *filter, void *output, int batch,
                           int in_height, int in_width, int in_channel,
                           int filter_height, int filter_width, int out_channel,
                           int pad_height, int pad_width, int stride_height,
                           int stride_width, int input_stride_b,
                           int input_stride_h, int input_stride_w,
                           int input_stride_c, int filter_stride_oc,
                           int filter_stride_h, int filter_stride_w,
                           int filter_stride_ic, int output_stride_b,
                           int output_stride_h, int output_stride_w,
                           int output_stride_c);

/**
 * @brief Convolution with FP32 input, FP32 filter, FP32 output
 *
 * Performs 2D convolution operation where:
 * - Input tensor: FP32 (32-bit single precision)
 * - Filter tensor: FP32 (32-bit single precision)
 * - Output tensor: FP32 (32-bit single precision)
 *
 * Provides highest numerical precision at the cost of memory and computation.
 *
 * @param input Pointer to input tensor data (FP32 format)
 * @param filter Pointer to filter/weight tensor data (FP32 format)
 * @param output Pointer to output tensor data (FP32 format)
 * @param batch Batch size (typical range: 1-128)
 * @param in_height Input tensor height (typical range: 1-1024)
 * @param in_width Input tensor width (typical range: 1-1024)
 * @param in_channel Input tensor channels (typical range: 1-512)
 * @param filter_height Filter height (typical range: 1-11)
 * @param filter_width Filter width (typical range: 1-11)
 * @param out_channel Output channels/filters (typical range: 1-512)
 * @param pad_height Height padding (typical range: 0-5)
 * @param pad_width Width padding (typical range: 0-5)
 * @param stride_height Height stride (typical range: 1-4)
 * @param stride_width Width stride (typical range: 1-4)
 * @param input_stride_b Input batch dimension stride (in elements)
 * @param input_stride_h Input height dimension stride (in elements)
 * @param input_stride_w Input width dimension stride (in elements)
 * @param input_stride_c Input channel dimension stride (in elements)
 * @param filter_stride_oc Filter output channel dimension stride (in elements)
 * @param filter_stride_h Filter height dimension stride (in elements)
 * @param filter_stride_w Filter width dimension stride (in elements)
 * @param filter_stride_ic Filter input channel dimension stride (in elements)
 * @param output_stride_b Output batch dimension stride (in elements)
 * @param output_stride_h Output height dimension stride (in elements)
 * @param output_stride_w Output width dimension stride (in elements)
 * @param output_stride_c Output channel dimension stride (in elements)
 *
 * @note Recommended for applications requiring maximum numerical accuracy
 */
void conv2d_fp32_fp32_fp32(void *input, void *filter, void *output, int batch,
                           int in_height, int in_width, int in_channel,
                           int filter_height, int filter_width, int out_channel,
                           int pad_height, int pad_width, int stride_height,
                           int stride_width, int input_stride_b,
                           int input_stride_h, int input_stride_w,
                           int input_stride_c, int filter_stride_oc,
                           int filter_stride_h, int filter_stride_w,
                           int filter_stride_ic, int output_stride_b,
                           int output_stride_h, int output_stride_w,
                           int output_stride_c);

#endif /* CONV2D_H */

#endif
