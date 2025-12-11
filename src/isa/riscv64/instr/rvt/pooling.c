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

#include "pooling.h"
#include "type.h"
#include <float.h>
#include <stdint.h>
#include <stdio.h>

/* FP16 load/store functions */
static float load_fp16(void *data, int idx) {
  return fp16_to_fp32(((uint16_t *)data)[idx]);
}

static void store_fp16(void *data, int idx, float value) {
  ((uint16_t *)data)[idx] = fp32_to_fp16(value);
}

/**
 * @brief Core pooling kernel with configurable data type handling
 *
 * @param convert_input Function to convert input data type to FP32
 * @param load_output Function to load existing output value (for accumulation)
 * @param store_output Function to store result with appropriate data type
 * @param is_max 1 for max pooling, 0 for average pooling
 */
static inline void pool2d_accum_kernel(void *input, void *output,
                                       const PoolingConfig *config,
                                       const PoolingStrideConfig *strides,
                                       float (*convert_input)(void *, int),
                                       float (*load_output)(void *, int),
                                       void (*store_output)(void *, int, float),
                                       int is_max) {

  // Unpack configuration for readability
  int batch = config->batch;
  int in_height = config->in_height;
  int in_width = config->in_width;
  int in_channel = config->in_channel;
  int kernel_h = config->kernel_h;
  int kernel_w = config->kernel_w;
  int pad_height = config->pad_height;
  int pad_width = config->pad_width;
  int stride_height = config->stride_height;
  int stride_width = config->stride_width;

  // Unpack strides for readability
  int input_stride_b = strides->input_stride_b;
  int input_stride_h = strides->input_stride_h;
  int input_stride_w = strides->input_stride_w;
  int input_stride_c = strides->input_stride_c;
  int output_stride_b = strides->output_stride_b;
  int output_stride_h = strides->output_stride_h;
  int output_stride_w = strides->output_stride_w;
  int output_stride_c = strides->output_stride_c;

  // Calculate output spatial dimensions
  const int out_height =
      (in_height + 2 * pad_height - kernel_h) / stride_height + 1;
  const int out_width =
      (in_width + 2 * pad_width - kernel_w) / stride_width + 1;

  // Main pooling loops
  for (int b = 0; b < batch; ++b) {
    for (int oh = 0; oh < out_height; ++oh) {
      const int start_h = oh * stride_height - pad_height;

      for (int ow = 0; ow < out_width; ++ow) {
        const int start_w = ow * stride_width - pad_width;

        for (int c = 0; c < in_channel; ++c) {
          const int out_idx = b * output_stride_b + oh * output_stride_h +
                              ow * output_stride_w + c * output_stride_c;

          float pool_result = 0;
          float accum = is_max ? -FLT_MAX : 0.0f;
          int valid_count = 0;

          // Loop over pooling window
          for (int kh = 0; kh < kernel_h; ++kh) {
            const int ih = start_h + kh;
            if ((unsigned)ih >= (unsigned)in_height)
              continue;

            for (int kw = 0; kw < kernel_w; ++kw) {
              const int iw = start_w + kw;
              if ((unsigned)iw >= (unsigned)in_width)
                continue;

              const int in_idx = b * input_stride_b + ih * input_stride_h +
                                 iw * input_stride_w + c * input_stride_c;

              const float value = convert_input(input, in_idx);

              if (is_max) {
                // Max pooling: track maximum value
                accum = (value > accum) ? value : accum;
              } else {
                // Average pooling: accumulate sum
                accum += value;
              }
              ++valid_count;
            }
          }

          // Finalize pooling result
          if (is_max) {
            // For max pooling, if no valid values found, use 0
            pool_result = (valid_count == 0) ? 0.0f : accum;
          } else {
            // For average pooling, compute average over valid positions
            pool_result =
                (valid_count > 0) ? (accum / (float)valid_count) : 0.0f;
          }

          // Store the result
          store_output(output, out_idx, pool_result);
        }
      }
    }
  }
}

/**
 * @brief Max pooling operation with FP16 input and FP16 output
 *
 * Performs 2D max pooling operation.
 */
void maxpool_fp16_fp16(void *input, void *output, const PoolingConfig *config,
                       const PoolingStrideConfig *strides) {
  // Create a copy of config with is_max set to 1
  PoolingConfig max_config = *config;
  max_config.is_max = 1;

  pool2d_accum_kernel(input, output, &max_config, strides, load_fp16, load_fp16,
                      store_fp16, 1);
}

/**
 * @brief Average pooling operation with FP16 input and FP16 output
 *
 * Performs 2D average pooling operation.
 */
void avgpool_fp16_fp16(void *input, void *output, const PoolingConfig *config,
                       const PoolingStrideConfig *strides) {
  // Create a copy of config with is_max set to 0
  PoolingConfig avg_config = *config;
  avg_config.is_max = 0;

  pool2d_accum_kernel(input, output, &avg_config, strides, load_fp16, load_fp16,
                      store_fp16, 0);
}

#endif /* CONFIG_CUSTOM_TENSOR */
