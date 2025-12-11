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

#include "conv2d.h"
#include "type.h"

// ======================================
// Type conversion utilities
// ======================================

/* FP32 - Load and store operations for single precision floating point */
static float load_fp32(void *data, int idx) { return ((float *)data)[idx]; }
static void store_fp32(void *data, int idx, float value) {
  ((float *)data)[idx] = value;
}

/* FP16 - Load and store operations for half precision floating point */
static float load_fp16(void *data, int idx) {
  return fp16_to_fp32(((uint16_t *)data)[idx]);
}
static void store_fp16(void *data, int idx, float value) {
  ((uint16_t *)data)[idx] = fp32_to_fp16(value);
}

/**
 * @brief Core convolution kernel with configurable data type handling
 *
 * This is the core implementation of 2D convolution that supports different
 * data types through function pointers for loading and storing.
 *
 * @param convert_input Function to convert input data type to FP32
 * @param convert_filter Function to convert filter data type to FP32
 * @param load_output Function to load existing output value (for accumulation)
 * @param store_output Function to store result with appropriate data type
 *
 */
static inline void conv2d_accum_kernel(
    void *input, void *filter, void *output, const Conv2DConfig *config,
    const Conv2DStrideConfig *strides, float (*convert_input)(void *, int),
    float (*convert_filter)(void *, int), float (*load_output)(void *, int),
    void (*store_output)(void *, int, float)) {

  // Unpack configuration for readability
  int batch = config->batch;
  int in_height = config->in_height;
  int in_width = config->in_width;
  int in_channel = config->in_channel;
  int filter_height = config->filter_height;
  int filter_width = config->filter_width;
  int out_channel = config->out_channel;
  int pad_height = config->pad_height;
  int pad_width = config->pad_width;
  int stride_height = config->stride_height;
  int stride_width = config->stride_width;

  // Unpack strides for readability
  int input_stride_b = strides->input_stride_b;
  int input_stride_h = strides->input_stride_h;
  int input_stride_w = strides->input_stride_w;
  int input_stride_c = strides->input_stride_c;
  int filter_stride_oc = strides->filter_stride_oc;
  int filter_stride_h = strides->filter_stride_h;
  int filter_stride_w = strides->filter_stride_w;
  int filter_stride_ic = strides->filter_stride_ic;
  int output_stride_b = strides->output_stride_b;
  int output_stride_h = strides->output_stride_h;
  int output_stride_w = strides->output_stride_w;
  int output_stride_c = strides->output_stride_c;

  // Calculate output spatial dimensions
  // Formula: out_size = floor((in_size + 2*pad - filter_size) / stride) + 1
  int out_height =
      (in_height + 2 * pad_height - filter_height) / stride_height + 1;
  int out_width = (in_width + 2 * pad_width - filter_width) / stride_width + 1;

  // Main convolution loops
  for (int b = 0; b < batch; b++) { // Loop over batches
    for (int out_c = 0; out_c < out_channel;
         out_c++) { // Loop over output channels (filters)
      for (int out_h = 0; out_h < out_height;
           out_h++) { // Loop over output height
        for (int out_w = 0; out_w < out_width;
             out_w++) { // Loop over output width

          // Calculate starting position in input for current output position
          int start_h = out_h * stride_height - pad_height;
          int start_w = out_w * stride_width - pad_width;

          float sum = 0; // Accumulator for convolution result

          // Convolution window loops
          for (int k_h = 0; k_h < filter_height;
               k_h++) { // Loop over filter height
            for (int k_w = 0; k_w < filter_width;
                 k_w++) { // Loop over filter width

              // Map filter position to input position
              int in_h = start_h + k_h;
              int in_w = start_w + k_w;

              // Boundary check for padding
              if (in_h >= 0 && in_h < in_height && in_w >= 0 &&
                  in_w < in_width) {
                // Loop over input channels
                for (int in_c = 0; in_c < in_channel; in_c++) {

                  // Calculate linear indices using strides
                  int input_idx = b * input_stride_b + in_h * input_stride_h +
                                  in_w * input_stride_w + in_c * input_stride_c;

                  int filter_idx =
                      out_c * filter_stride_oc + k_h * filter_stride_h +
                      k_w * filter_stride_w + in_c * filter_stride_ic;

                  // Load and convert data types to FP32 for computation
                  float input_val = convert_input(input, input_idx);
                  float filter_val = convert_filter(filter, filter_idx);

                  // Multiply-accumulate operation
                  sum += input_val * filter_val;
                }
              }
            }
          }

          // Calculate output index
          int output_idx = b * output_stride_b + out_h * output_stride_h +
                           out_w * output_stride_w + out_c * output_stride_c;

          // Store result with appropriate data type conversion
          store_output(output, output_idx, sum);
        }
      }
    }
  }
}

/**
 * @brief Implementation: conv2d(fp16, fp16) --> fp32s
 */
void conv2d_fp32_fp16_fp16(void *input, void *filter, void *output,
                           const Conv2DConfig *config,
                           const Conv2DStrideConfig *strides) {
  conv2d_accum_kernel(input, filter, output, config, strides, load_fp16,
                      load_fp16, load_fp32, store_fp32);
}

/**
 * @brief Implementation: conv2d(fp16, fp16) --> fp16
 */
void conv2d_fp16_fp16_fp16(void *input, void *filter, void *output,
                           const Conv2DConfig *config,
                           const Conv2DStrideConfig *strides) {
  conv2d_accum_kernel(input, filter, output, config, strides, load_fp16,
                      load_fp16, load_fp16, store_fp16);
}

/**
 * @brief Implementation: conv2d(fp32, fp32) --> fp32
 */
void conv2d_fp32_fp32_fp32(void *input, void *filter, void *output,
                           const Conv2DConfig *config,
                           const Conv2DStrideConfig *strides) {
  conv2d_accum_kernel(input, filter, output, config, strides, load_fp32,
                      load_fp32, load_fp32, store_fp32);
}

#endif /* CONFIG_CUSTOM_TENSOR */
