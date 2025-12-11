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

#ifndef POOLING_H
#define POOLING_H

#include <stdio.h>
#include <stdint.h>
#include <memory/paddr.h>

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
 * @brief Max pooling operation with FP16 input and FP16 output
 * 
 * Performs 2D max pooling where each output element is the maximum value
 * from the corresponding kernel window in the input.
 * 
 * Formula: output[b, c, h, w] = max_{i=0..kernel_h-1, j=0..kernel_w-1} 
 *                                input[b, c, h*stride_h+i-pad_h, w*stride_w+j-pad_w]
 * 
 * @param input Pointer to input tensor
 * @param output Pointer to output tensor
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
 * @param input_stride_b Input batch dimension stride
 * @param input_stride_h Input height dimension stride
 * @param input_stride_w Input width dimension stride
 * @param input_stride_c Input channel dimension stride
 * @param output_stride_b Output batch dimension stride
 * @param output_stride_h Output height dimension stride
 * @param output_stride_w Output width dimension stride
 * @param output_stride_c Output channel dimension stride
 */
void maxpool_fp16_fp16(
    void *input, void *output,
    int batch, int in_height, int in_width, int in_channel,
    int kernel_h, int kernel_w,
    int pad_height, int pad_width,
    int stride_height, int stride_width,
    int input_stride_b, int input_stride_h, int input_stride_w, int input_stride_c,
    int output_stride_b, int output_stride_h, int output_stride_w, int output_stride_c);

/**
 * @brief Average pooling operation with FP16 input and FP16 output
 * 
 * Performs 2D average pooling where each output element is the average value
 * from the corresponding kernel window in the input.
 * 
 * Formula: output[b, c, h, w] = (1/(kernel_h*kernel_w)) * 
 *           Σ_{i=0..kernel_h-1} Σ_{j=0..kernel_w-1} 
 *           input[b, c, h*stride_h+i-pad_h, w*stride_w+j-pad_w]
 * 
 * Only valid positions within input bounds contribute to the average
 * (padding areas are excluded from both numerator and denominator).
 * 
 * @param input Pointer to input tensor
 * @param output Pointer to output tensor
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
 * @param input_stride_b Input batch dimension stride
 * @param input_stride_h Input height dimension stride
 * @param input_stride_w Input width dimension stride
 * @param input_stride_c Input channel dimension stride
 * @param output_stride_b Output batch dimension stride
 * @param output_stride_h Output height dimension stride
 * @param output_stride_w Output width dimension stride
 * @param output_stride_c Output channel dimension stride
 */
void avgpool_fp16_fp16(
    void *input, void *output,
    int batch, int in_height, int in_width, int in_channel,
    int kernel_h, int kernel_w,
    int pad_height, int pad_width,
    int stride_height, int stride_width,
    int input_stride_b, int input_stride_h, int input_stride_w, int input_stride_c,
    int output_stride_b, int output_stride_h, int output_stride_w, int output_stride_c);

#endif /* POOLING_H */

#endif /* CONFIG_RVT */
