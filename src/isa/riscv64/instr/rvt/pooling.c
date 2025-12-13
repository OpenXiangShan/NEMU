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

#include "pooling.h"
#include "type.h"
#include <float.h>
#include <stdint.h>
#include <stdio.h>

/* FP16 */
static float load_fp16(void *data, int idx) {
  return fp16_to_fp32(((uint16_t *)data)[idx]);
}
static void store_fp16(void *data, int idx, float value) {
  ((uint16_t *)data)[idx] = fp32_to_fp16(value);
}

static inline void pool2d_accum_kernel(
    void *input, void *output, int batch, int in_height, int in_width,
    int in_channel, int kernel_h, int kernel_w, int pad_height, int pad_width,
    int stride_height, int stride_width, int input_stride_b, int input_stride_h,
    int input_stride_w, int input_stride_c, int output_stride_b,
    int output_stride_h, int output_stride_w, int output_stride_c,
    float (*convert_input)(void *, int), float (*load_output)(void *, int),
    void (*store_output)(void *, int, float), int is_max) {
  const int out_height =
      (in_height + 2 * pad_height - kernel_h) / stride_height + 1;
  const int out_width =
      (in_width + 2 * pad_width - kernel_w) / stride_width + 1;

  for (int b = 0; b < batch; ++b) {
    for (int oh = 0; oh < out_height; ++oh) {
      const int start_h = oh * stride_height - pad_height;

      for (int ow = 0; ow < out_width; ++ow) {
        const int start_w = ow * stride_width - pad_width;

        for (int c = 0; c < in_channel; ++c) {
          const int out_idx = b * output_stride_b + oh * output_stride_h +
                              ow * output_stride_w + c * output_stride_c;

          float acc = 0;
          float red = is_max ? -FLT_MAX : 0.0f;
          int cnt = 0;

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

              const float v = convert_input(input, in_idx);
              if (is_max) {
                red = (v > red) ? v : red;
              } else {
                red += v;
              }
              ++cnt;
            }
          }

          if (!is_max) {
            if (cnt > 0)
              red /= (float)cnt;
          } else {
            if (cnt == 0)
              red = 0.0f;
          }

          acc += red;
          store_output(output, out_idx, acc);
        }
      }
    }
  }
}

void maxpool_fp16_fp16(void *input, void *output, int batch, int in_height,
                       int in_width, int in_channel, int kernel_h, int kernel_w,
                       int pad_height, int pad_width, int stride_height,
                       int stride_width, int input_stride_b, int input_stride_h,
                       int input_stride_w, int input_stride_c,
                       int output_stride_b, int output_stride_h,
                       int output_stride_w, int output_stride_c) {
  pool2d_accum_kernel(input, output, batch, in_height, in_width, in_channel,
                      kernel_h, kernel_w, pad_height, pad_width, stride_height,
                      stride_width, input_stride_b, input_stride_h,
                      input_stride_w, input_stride_c, output_stride_b,
                      output_stride_h, output_stride_w, output_stride_c,
                      load_fp16, load_fp16, store_fp16, 1);
}

void avgpool_fp16_fp16(void *input, void *output, int batch, int in_height,
                       int in_width, int in_channel, int kernel_h, int kernel_w,
                       int pad_height, int pad_width, int stride_height,
                       int stride_width, int input_stride_b, int input_stride_h,
                       int input_stride_w, int input_stride_c,
                       int output_stride_b, int output_stride_h,
                       int output_stride_w, int output_stride_c) {
  pool2d_accum_kernel(input, output, batch, in_height, in_width, in_channel,
                      kernel_h, kernel_w, pad_height, pad_width, stride_height,
                      stride_width, input_stride_b, input_stride_h,
                      input_stride_w, input_stride_c, output_stride_b,
                      output_stride_h, output_stride_w, output_stride_c,
                      load_fp16, load_fp16, store_fp16, 0);
}
#endif
