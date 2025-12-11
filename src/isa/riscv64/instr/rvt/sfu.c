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

#include <stdio.h>
#include "sfu.h"

typedef float (*sfu_func_t)(float* in);

static float sfu_recip(float* in) {
    return 1.0f / *in;
}

static float sfu_rsqrt(float *in) {
    return 1.0f / sqrtf(*in);
}

static float sfu_sin(float *in) {
    return sinf(*in);
}

static float sfu_cos(float *in) {
    return cosf(*in);
}

static float sfu_exp(float *in) {
    return expf(*in);
}

static float sfu_relu(float *in) {
    float t = *in;
    return t > 0.f ? t : 0.f;
}

bool tensor_sfu_fp32(float *input, float *output, int32_t input_dims[4],
                     int32_t input_strides[4], int32_t output_strides[4], SpecialFunctionUnitType op, uint8_t mode)
{
    sfu_func_t func = NULL;
    switch (op)
    {
    case SFU_RECIP:
        func = sfu_recip;
        break;
    case SFU_RSQRT:
        func = sfu_rsqrt;
        break;
    case SFU_SIN:
        func = sfu_sin;
        break;
    case SFU_COS:
        func = sfu_cos;
        break;
    case SFU_EXP:
        func = sfu_exp;
        break;
    case SFU_RELU:
        func = sfu_relu;
        break;
    default:
    #ifdef DEBUG_CUSTOM_TENSOR
      printf("Unsupported SFU op %d\n", op);
    #endif    
        return false;
    }

    for(int dim3 = 0 ; dim3 < input_dims[3] ; dim3 ++)
    for(int dim2 = 0 ; dim2 < input_dims[2] ; dim2 ++)
    for(int dim1 = 0 ; dim1 < input_dims[1] ; dim1 ++)
    for(int dim0 = 0 ; dim0 < input_dims[0] ; dim0 ++)
        output[dim3 * output_strides[3] + dim2 * output_strides[2] + dim1 * output_strides[1] + dim0 * output_strides[0]] = 
        func(input + 
               dim3 *  input_strides[3] + dim2 *  input_strides[2] + dim1 *  input_strides[1] + dim0 *  input_strides[0]);
    return true;
}

#endif  /* CONFIG_CUSTOM_TENSOR */
