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
#ifdef CONFIG_RVT

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
 * Key characteristics:
 * - Computational complexity: O(batch * out_channel * out_height * out_width *
 *                           filter_height * filter_width * in_channel)
 * - Memory access pattern: Strided access to input, filter, and output tensors
 * - Data type handling: All computations performed in FP32 internally
 *
 * @param convert_input Function to convert input data type to FP32
 * @param convert_filter Function to convert filter data type to FP32
 * @param load_output Function to load existing output value (for accumulation)
 * @param store_output Function to store result with appropriate data type
 *
 * @note All computations are performed in FP32 precision for accuracy
 * @note No SIMD optimizations in this baseline implementation
 * @note Padding handling: Only valid padding (no explicit zero padding buffer)
 */
static inline void conv2d_accum_kernel(
    void *input, void *filter, void *output, int batch, int in_height,
    int in_width, int in_channel, int filter_height, int filter_width,
    int out_channel, int pad_height, int pad_width, int stride_height,
    int stride_width, int input_stride_b, int input_stride_h,
    int input_stride_w, int input_stride_c, int filter_stride_oc,
    int filter_stride_h, int filter_stride_w, int filter_stride_ic,
    int output_stride_b, int output_stride_h, int output_stride_w,
    int output_stride_c, float (*convert_input)(void *, int),
    float (*convert_filter)(void *, int), float (*load_output)(void *, int),
    void (*store_output)(void *, int, float)) {

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

              // Boundary check for padding (implicit zero padding)
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
              // Implicit zero padding: positions outside input bounds
              // contribute 0
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
 * @brief Implementation: conv2d(fp16, fp16) --> fp32
 *
 * This version accumulates in FP32 precision for higher accuracy while
 * using FP16 for storage to save memory bandwidth.
 *
 * Memory usage:
 * - Input: 2 bytes per element (FP16)
 * - Filter: 2 bytes per element (FP16)
 * - Output: 4 bytes per element (FP32)
 *
 * Typical use: Mixed-precision training accumulation
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
                           int output_stride_c) {
  conv2d_accum_kernel(input, filter, output, batch, in_height, in_width,
                      in_channel, filter_height, filter_width, out_channel,
                      pad_height, pad_width, stride_height, stride_width,
                      input_stride_b, input_stride_h, input_stride_w,
                      input_stride_c, filter_stride_oc, filter_stride_h,
                      filter_stride_w, filter_stride_ic, output_stride_b,
                      output_stride_h, output_stride_w, output_stride_c,
                      load_fp16, // Input: FP16 -> FP32
                      load_fp16, // Filter: FP16 -> FP32
                      load_fp32, // Load existing output (not used in this case)
                      store_fp32); // Store as FP32
}

/**
 * @brief Implementation: conv2d(fp16, fp16) --> fp16
 *
 * This version uses FP16 throughout, minimizing memory usage at the cost
 * of reduced numerical precision due to FP16 accumulation.
 *
 * Memory usage:
 * - Input: 2 bytes per element (FP16)
 * - Filter: 2 bytes per element (FP16)
 * - Output: 2 bytes per element (FP16)
 *
 * Typical use: Inference, memory-constrained applications
 *
 * @warning FP16 accumulation may lead to precision loss for large sums
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
                           int output_stride_c) {
  conv2d_accum_kernel(input, filter, output, batch, in_height, in_width,
                      in_channel, filter_height, filter_width, out_channel,
                      pad_height, pad_width, stride_height, stride_width,
                      input_stride_b, input_stride_h, input_stride_w,
                      input_stride_c, filter_stride_oc, filter_stride_h,
                      filter_stride_w, filter_stride_ic, output_stride_b,
                      output_stride_h, output_stride_w, output_stride_c,
                      load_fp16,   // Input: FP16 -> FP32
                      load_fp16,   // Filter: FP16 -> FP32
                      load_fp16,   // Load existing output (not used)
                      store_fp16); // Store as FP16 (convert FP32 -> FP16)
}

/**
 * @brief Implementation: conv2d(fp32, fp32) --> fp32
 *
 * This version uses FP32 throughout, providing the highest numerical
 * precision at the cost of increased memory usage and bandwidth.
 *
 * Memory usage:
 * - Input: 4 bytes per element (FP32)
 * - Filter: 4 bytes per element (FP32)
 * - Output: 4 bytes per element (FP32)
 *
 * Typical use: Training, high-precision applications
 *
 * @note No data type conversions needed, direct FP32 operations
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
                           int output_stride_c) {
  conv2d_accum_kernel(input, filter, output, batch, in_height, in_width,
                      in_channel, filter_height, filter_width, out_channel,
                      pad_height, pad_width, stride_height, stride_width,
                      input_stride_b, input_stride_h, input_stride_w,
                      input_stride_c, filter_stride_oc, filter_stride_h,
                      filter_stride_w, filter_stride_ic, output_stride_b,
                      output_stride_h, output_stride_w, output_stride_c,
                      load_fp32,   // Input: FP32 direct
                      load_fp32,   // Filter: FP32 direct
                      load_fp32,   // Load existing output
                      store_fp32); // Store as FP32
}
#endif
