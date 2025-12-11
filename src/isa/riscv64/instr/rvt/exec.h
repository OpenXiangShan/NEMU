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
#ifndef RISCV_RVT_EXEC_H
#define RISCV_RVT_EXEC_H
#include "register.h"

#include "convert.h"
#include "conv2d.h"
#include "pooling.h"
#include "element_wise.h"
#include "matmul.h"
#include "matmul_subfloat.h"
#include "sfu.h"
#include "type.h"
#include <memory/paddr.h>
#include <time.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef void (*MatMulFunc)(void *input0, void *input1, void *output,
                           int input0_dim0, int input0_dim1, int input1_dim0,
                           int input1_dim1, int input0_stride0,
                           int input0_stride1, int input1_stride0,
                           int input1_stride1, int output_stride0,
                           int output_stride1);

#define INIT_MATMUL_FLOAT_TABLE_ENTRY(rs1type, rs2type, rs3type)               \
  matmul_float_table[MATMUL_INPUT_TYPE_##rs1type][MATMUL_INPUT_TYPE_##rs2type] \
                    [MATMUL_OUTPUT_TYPE_##rs3type] =                           \
                        matmul_##rs3type##_##rs1type##_##rs2type

MatMulFunc matmul_float_table[MATMUL_INPUT_TYPE_COUNT][MATMUL_INPUT_TYPE_COUNT]
                             [MATMUL_OUTPUT_TYPE_COUNT] = {0};

bool has_init_matmul_float_table = 0;
bool has_init_matmul_subfloat_table = 0;

void init_matmul_float_table() {

  INIT_MATMUL_FLOAT_TABLE_ENTRY(fp16, fp16,
                                fp32); // fp16 x fp16 -> fp32
  INIT_MATMUL_FLOAT_TABLE_ENTRY(fp16, fp16,
                                fp16); // fp16 x fp16 -> fp16
  INIT_MATMUL_FLOAT_TABLE_ENTRY(fp16, bf16,
                                fp32); // fp16 x bf16 -> fp32
  INIT_MATMUL_FLOAT_TABLE_ENTRY(fp16, bf16,
                                fp16); // fp16 x bf16 -> fp16
  INIT_MATMUL_FLOAT_TABLE_ENTRY(fp16, tf32,
                                fp32); // fp16 x TF16 -> fp32
  INIT_MATMUL_FLOAT_TABLE_ENTRY(fp16, tf32,
                                fp16); // fp16 x TF16 -> fp16

  INIT_MATMUL_FLOAT_TABLE_ENTRY(bf16, fp16,
                                fp32); // bf16 x fp16 -> fp32
  INIT_MATMUL_FLOAT_TABLE_ENTRY(bf16, fp16,
                                fp16); // bf16 x fp16 -> fp16
  INIT_MATMUL_FLOAT_TABLE_ENTRY(bf16, bf16,
                                fp32); // bf16 x bf16 -> fp32
  INIT_MATMUL_FLOAT_TABLE_ENTRY(bf16, bf16,
                                fp16); // bf16 x bf16 -> fp16
  INIT_MATMUL_FLOAT_TABLE_ENTRY(bf16, tf32,
                                fp32); // bf16 x TF16 -> fp32
  INIT_MATMUL_FLOAT_TABLE_ENTRY(bf16, tf32,
                                fp16); // bf16 x TF16 -> fp16

  INIT_MATMUL_FLOAT_TABLE_ENTRY(tf32, fp16,
                                fp32); // tf32 x fp16 -> fp32
  INIT_MATMUL_FLOAT_TABLE_ENTRY(tf32, fp16,
                                fp16); // tf32 x fp16 -> fp16
  INIT_MATMUL_FLOAT_TABLE_ENTRY(tf32, bf16,
                                fp32); // tf32 x bf16 -> fp32
  INIT_MATMUL_FLOAT_TABLE_ENTRY(tf32, bf16,
                                fp16); // tf32 x bf16 -> fp16
  INIT_MATMUL_FLOAT_TABLE_ENTRY(tf32, tf32,
                                fp32); // tf32 x TF16 -> fp32
  INIT_MATMUL_FLOAT_TABLE_ENTRY(tf32, tf32,
                                fp16); // tf32 x TF16 -> fp16

  INIT_MATMUL_FLOAT_TABLE_ENTRY(fp32, fp32,
                                fp32); // fp32 x fp32 -> fp32
}

typedef void (*Conv2DFunc)(void *input, void *filter, void *output,
                          int batch, int in_height, int in_width, int in_channel,
                          int filter_height, int filter_width, int out_channel,
                          int pad_height, int pad_width,
                          int stride_height, int stride_width,
                          int input_stride_b, int input_stride_h, int input_stride_w, int input_stride_c,
                          int filter_stride_oc, int filter_stride_h, int filter_stride_w, int filter_stride_ic,
                          int output_stride_b, int output_stride_h, int output_stride_w, int output_stride_c);

#define INIT_CONV2D_FLOAT_TABLE_ENTRY(rs1type, rs2type, rs3type)               \
  conv2d_float_table[CONV2D_INPUT_TYPE_##rs1type][CONV2D_INPUT_TYPE_##rs2type] \
                    [CONV2D_OUTPUT_TYPE_##rs3type] =                           \
                        conv2d_##rs3type##_##rs1type##_##rs2type

Conv2DFunc conv2d_float_table[CONV2D_INPUT_TYPE_COUNT][CONV2D_INPUT_TYPE_COUNT]
                             [CONV2D_OUTPUT_TYPE_COUNT] = {0};

bool has_init_conv2d_float_table = 0;

void init_conv2d_float_table() {
  INIT_CONV2D_FLOAT_TABLE_ENTRY(fp16, fp16, fp32);
  INIT_CONV2D_FLOAT_TABLE_ENTRY(fp16, fp16, fp16);
  INIT_CONV2D_FLOAT_TABLE_ENTRY(fp32, fp32, fp32);
}

bool has_init_pooling_float_table = 0;

typedef void (*PoolFunc)(void *input, void *output,
                            int batch, int in_height, int in_width, int in_channel,
                            int kernel_h, int kernel_w,
                            int pad_height, int pad_width,
                            int stride_height, int stride_width,
                            int input_stride_b, int input_stride_h, int input_stride_w, int input_stride_c,
                            int output_stride_b, int output_stride_h, int output_stride_w, int output_stride_c);

#define INIT_POOLING_FLOAT_TABLE_ENTRY(optype, rs1type, rs3type)                   \
  pooling_float_table[POOL_OP_##optype][POOLING_INPUT_TYPE_##rs1type][POOLING_OUTPUT_TYPE_##rs3type]\
                         = optype##_##rs3type##_##rs1type

PoolFunc pooling_float_table[POOL_OP_COUNT][POOLING_INPUT_TYPE_COUNT][POOLING_OUTPUT_TYPE_COUNT] = {0};

void init_pooling_float_table() {
  INIT_POOLING_FLOAT_TABLE_ENTRY(maxpool, fp16, fp16);

  // INIT_POOLING_FLOAT_TABLE_ENTRY(maxpool, fp32, fp32);

  INIT_POOLING_FLOAT_TABLE_ENTRY(avgpool, fp16, fp16);

  // INIT_POOLING_FLOAT_TABLE_ENTRY(avgpool, fp32, fp32);
}


#define INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(rs1type, rs2type, rs3type)            \
  matmul_subfloat_table[MATMUL_INPUT_TYPE_##rs1type]                           \
                       [MATMUL_INPUT_TYPE_##rs2type]                           \
                       [MATMUL_OUTPUT_TYPE_##rs3type] =                        \
                           matmul_##rs3type##_##rs1type##_##rs2type

MatMulFunc matmul_subfloat_table[MATMUL_SUBFLOAT_INPUT_TYPE_COUNT]
                                [MATMUL_SUBFLOAT_INPUT_TYPE_COUNT]
                                [MATMUL_SUBFLOAT_OUTPUT_TYPE_COUNT] = {0};

void init_matmul_subfloat_table() {
  /* ue1m7 */
  /* ue1m7 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue1m7, ue1m7, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue1m7, ue1m7, fp16);

  /* e1m6 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e1m6, e1m6, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e1m6, e1m6, fp16);

  /* e1m5 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e1m5, e1m5, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e1m5, e1m5, fp16);

  /* e1m4 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e1m4, e1m4, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e1m4, e1m4, fp16);

  /* e1m3 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e1m3, e1m3, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e1m3, e1m3, fp16);

  /* e1m2 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e1m2, e1m2, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e1m2, e1m2, fp16);

  /* e1m1 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e1m1, e1m1, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e1m1, e1m1, fp16);

  /* e1m0 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e1m0, e1m0, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e1m0, e1m0, fp16);

  /* ue1m6 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue1m6, ue1m6, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue1m6, ue1m6, fp16);

  /* ue2m6 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue2m6, ue2m6, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue2m6, ue2m6, fp16);

  /* e2m5 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e2m5, e2m5, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e2m5, e2m5, fp16);

  /* e2m4 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e2m4, e2m4, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e2m4, e2m4, fp16);

  /* e2m3 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e2m3, e2m3, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e2m3, e2m3, fp16);

  /* e2m2 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e2m2, e2m2, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e2m2, e2m2, fp16);

  /* e2m1 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e2m1, e2m1, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e2m1, e2m1, fp16);

  /* e2m0 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e2m0, e2m0, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e2m0, e2m0, fp16);

  /* ue1m5 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue1m5, ue1m5, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue1m5, ue1m5, fp16);

  /* ue2m5 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue2m5, ue2m5, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue2m5, ue2m5, fp16);

  /* ue3m5 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue3m5, ue3m5, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue3m5, ue3m5, fp16);

  /* e3m4 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e3m4, e3m4, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e3m4, e3m4, fp16);

  /* e3m3 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e3m3, e3m3, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e3m3, e3m3, fp16);

  /* e3m2 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e3m2, e3m2, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e3m2, e3m2, fp16);

  /* e3m1 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e3m1, e3m1, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e3m1, e3m1, fp16);

  /* e3m0 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e3m0, e3m0, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e3m0, e3m0, fp16);

  /* ue1m4 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue1m4, ue1m4, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue1m4, ue1m4, fp16);

  /* ue2m4 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue2m4, ue2m4, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue2m4, ue2m4, fp16);

  /* ue3m4 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue3m4, ue3m4, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue3m4, ue3m4, fp16);

  /* ue4m4 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue4m4, ue4m4, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue4m4, ue4m4, fp16);

  /* e4m3 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e4m3, e4m3, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e4m3, e4m3, fp16);

  /* e4m2 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e4m2, e4m2, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e4m2, e4m2, fp16);

  /* e4m1 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e4m1, e4m1, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e4m1, e4m1, fp16);

  /* e4m0 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e4m0, e4m0, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e4m0, e4m0, fp16);

  /* ue1m3 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue1m3, ue1m3, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue1m3, ue1m3, fp16);

  /* ue2m3 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue2m3, ue2m3, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue2m3, ue2m3, fp16);

  /* ue3m3 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue3m3, ue3m3, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue3m3, ue3m3, fp16);

  /* ue4m3 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue4m3, ue4m3, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue4m3, ue4m3, fp16);

  /* ue5m3 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue5m3, ue5m3, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue5m3, ue5m3, fp16);

  /* e5m2 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e5m2, e5m2, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e5m2, e5m2, fp16);

  /* e5m1 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e5m1, e5m1, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e5m1, e5m1, fp16);

  /* e5m0 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e5m0, e5m0, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e5m0, e5m0, fp16);

  /* ue1m2 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue1m2, ue1m2, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue1m2, ue1m2, fp16);

  /* ue2m2 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue2m2, ue2m2, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue2m2, ue2m2, fp16);

  /* ue3m2 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue3m2, ue3m2, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue3m2, ue3m2, fp16);

  /* ue4m2 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue4m2, ue4m2, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue4m2, ue4m2, fp16);

  /* ue5m2 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue5m2, ue5m2, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue5m2, ue5m2, fp16);

  /* ue6m2 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue6m2, ue6m2, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue6m2, ue6m2, fp16);

  /* e6m1 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e6m1, e6m1, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e6m1, e6m1, fp16);

  /* e6m0 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e6m0, e6m0, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e6m0, e6m0, fp16);

  /* ue1m1 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue1m1, ue1m1, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue1m1, ue1m1, fp16);

  /* ue2m1 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue2m1, ue2m1, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue2m1, ue2m1, fp16);

  /* ue3m1 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue3m1, ue3m1, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue3m1, ue3m1, fp16);

  /* ue4m1 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue4m1, ue4m1, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue4m1, ue4m1, fp16);

  /* ue5m1 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue5m1, ue5m1, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue5m1, ue5m1, fp16);

  /* ue6m1 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue6m1, ue6m1, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue6m1, ue6m1, fp16);

  /* ue7m1 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue7m1, ue7m1, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue7m1, ue7m1, fp16);

  /* e7m0 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e7m0, e7m0, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(e7m0, e7m0, fp16);

  /* ue1m0 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue1m0, ue1m0, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue1m0, ue1m0, fp16);

  /* ue2m0 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue2m0, ue2m0, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue2m0, ue2m0, fp16);

  /* ue3m0 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue3m0, ue3m0, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue3m0, ue3m0, fp16);

  /* ue4m0 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue4m0, ue4m0, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue4m0, ue4m0, fp16);

  /* ue5m0 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue5m0, ue5m0, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue5m0, ue5m0, fp16);

  /* ue6m0 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue6m0, ue6m0, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue6m0, ue6m0, fp16);

  /* ue7m0 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue7m0, ue7m0, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue7m0, ue7m0, fp16);

  /* ue8m0 */
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue8m0, ue8m0, fp32);
  INIT_MATMUL_SUBFLOAT_TABLE_ENTRY(ue8m0, ue8m0, fp16);
}

#define MATMUL_INPUT_TYPE_U8 6
#define MATMUL_INPUT_TYPE_S8 7
#define MATMUL_OUTPUT_TYPE_S32 0
#define MATMUL_OUTPUT_TYPE_S16 1

/*
  The execution process of tensor compute instructions.
*/
def_EHelper(tx_matmul_subfloat) {
  int rs1 = s->src1.imm;
  int rs2 = s->src2.imm;
  int rs3 = s->src3.imm;

  void *input0 = (void *)cpu.gpr[rs1]._64;
  void *input1 = (void *)cpu.gpr[rs2]._64;
  void *output = (void *)cpu.gpr[rs3]._64;

  uint32_t tcfgrs1 = s->isa.instr.tensor_compute_64.tcfgrs1;
  uint32_t tcfgrs2 = s->isa.instr.tensor_compute_64.tcfgrs2;
  uint32_t tcfgrs3 = s->isa.instr.tensor_compute_64.tcfgrs3;

  TensorConfigRegister *TCR1 = &TCR[tcfgrs1];
  TensorConfigRegister *TCR2 = &TCR[tcfgrs2];
  TensorConfigRegister *TCR3 = &TCR[tcfgrs3];

  uint32_t input0_dim0 = TCR1->info.dim_stride_info.dim0;
  uint32_t input0_dim1 = TCR1->info.dim_stride_info.dim1;
  uint32_t input0_stride0 = TCR1->info.dim_stride_info.stride0;
  uint32_t input0_stride1 = TCR1->info.dim_stride_info.stride1;

  uint32_t input1_dim0 = TCR2->info.dim_stride_info.dim0;
  uint32_t input1_dim1 = TCR2->info.dim_stride_info.dim1;
  uint32_t input1_stride0 = TCR2->info.dim_stride_info.stride0;
  uint32_t input1_stride1 = TCR2->info.dim_stride_info.stride1;

  uint32_t output_dim0 = TCR3->info.dim_stride_info.dim0;
  uint32_t output_dim1 = TCR3->info.dim_stride_info.dim1;
  uint32_t output_stride0 = TCR3->info.dim_stride_info.stride0;
  uint32_t output_stride1 = TCR3->info.dim_stride_info.stride1;

  uint32_t rs1type = s->isa.instr.tensor_compute_64.rs1type;
  uint32_t rs2type = s->isa.instr.tensor_compute_64.rs2type;
  uint32_t rs3type = s->isa.instr.tensor_compute_64.rs3type;

  assert(input0_dim1 == input1_dim0);
  assert(input0_dim0 == output_dim0);
  assert(input1_dim1 == output_dim1);

  if (!has_init_matmul_subfloat_table) {
    has_init_matmul_subfloat_table = 1;
    init_matmul_subfloat_table();
  }

  int8_t *host_input0 = (int8_t *)uram_guest_to_host((paddr_t)input0);
  int8_t *host_input1 = (int8_t *)uram_guest_to_host((paddr_t)input1);
  int8_t *host_output = (int8_t *)uram_guest_to_host((paddr_t)output);
  MatMulFunc func = matmul_subfloat_table[rs1type][rs2type][rs3type];
  if (func) {
    func(host_input0, host_input1, host_output, input0_dim0, input0_dim1,
         input1_dim0, input1_dim1, input0_stride0, input0_stride1,
         input1_stride0, input1_stride1, output_stride0, output_stride1);
  }
}

def_EHelper(tx_matmul_float) {
  int rs1 = s->src1.imm;
  int rs2 = s->src2.imm;
  int rs3 = s->src3.imm;

  float *input0 = (float *)cpu.gpr[rs1]._64;
  float *input1 = (float *)cpu.gpr[rs2]._64;
  float *output = (float *)cpu.gpr[rs3]._64;

  uint32_t tcfgrs1 = s->isa.instr.tensor_compute_64.tcfgrs1;
  uint32_t tcfgrs2 = s->isa.instr.tensor_compute_64.tcfgrs2;
  uint32_t tcfgrs3 = s->isa.instr.tensor_compute_64.tcfgrs3;

  TensorConfigRegister *TCR1 = &TCR[tcfgrs1];
  TensorConfigRegister *TCR2 = &TCR[tcfgrs2];
  TensorConfigRegister *TCR3 = &TCR[tcfgrs3];

  uint32_t input0_dim0 = TCR1->info.dim_stride_info.dim0;
  uint32_t input0_dim1 = TCR1->info.dim_stride_info.dim1;
  uint32_t input0_stride0 = TCR1->info.dim_stride_info.stride0;
  uint32_t input0_stride1 = TCR1->info.dim_stride_info.stride1;

  uint32_t input1_dim0 = TCR2->info.dim_stride_info.dim0;
  uint32_t input1_dim1 = TCR2->info.dim_stride_info.dim1;
  uint32_t input1_stride0 = TCR2->info.dim_stride_info.stride0;
  uint32_t input1_stride1 = TCR2->info.dim_stride_info.stride1;

  uint32_t output_dim0 = TCR3->info.dim_stride_info.dim0;
  uint32_t output_dim1 = TCR3->info.dim_stride_info.dim1;
  uint32_t output_stride0 = TCR3->info.dim_stride_info.stride0;
  uint32_t output_stride1 = TCR3->info.dim_stride_info.stride1;

  uint32_t rs1type = s->isa.instr.tensor_compute_64.rs1type;
  uint32_t rs2type = s->isa.instr.tensor_compute_64.rs2type;
  uint32_t rs3type = s->isa.instr.tensor_compute_64.rs3type;

  assert(input0_dim1 == input1_dim0);
  assert(input0_dim0 == output_dim0);
  assert(input1_dim1 == output_dim1);
  printf("input0_dim1 is %d\n", input0_dim1);
  if (!has_init_matmul_float_table) {
    has_init_matmul_float_table = 1;
    init_matmul_float_table();
  }
  MatMulFunc func = matmul_float_table[rs1type][rs2type][rs3type];

  int8_t *host_input0 = (int8_t *)uram_guest_to_host((paddr_t)input0);
  int8_t *host_input1 = (int8_t *)uram_guest_to_host((paddr_t)input1);
  int8_t *host_output = (int8_t *)uram_guest_to_host((paddr_t)output);
  if (!func) {
    printf("Cannot find the function\n");
  } else {


    func(host_input0, host_input1, host_output, input0_dim0, input0_dim1,
         input1_dim0, input1_dim1, input0_stride0, input0_stride1,
         input1_stride0, input1_stride1, output_stride0, output_stride1);

  
  }
}

// TODO
def_EHelper(tx_matmul_fix) {
  int rs1 = s->src1.imm;
  int rs2 = s->src2.imm;
  int rs3 = s->src3.imm;

  void *input0 = (void *)cpu.gpr[rs1]._64;
  void *input1 = (void *)cpu.gpr[rs2]._64;
  void *output = (void *)cpu.gpr[rs3]._64;

  uint32_t tcfgrs1 = s->isa.instr.tensor_compute_64.tcfgrs1;
  uint32_t tcfgrs2 = s->isa.instr.tensor_compute_64.tcfgrs2;
  uint32_t tcfgrs3 = s->isa.instr.tensor_compute_64.tcfgrs3;

  TensorConfigRegister *TCR1 = &TCR[tcfgrs1];
  TensorConfigRegister *TCR2 = &TCR[tcfgrs2];
  TensorConfigRegister *TCR3 = &TCR[tcfgrs3];

  uint32_t input0_dim0 = TCR1->info.dim_stride_info.dim0;
  uint32_t input0_dim1 = TCR1->info.dim_stride_info.dim1;
  uint32_t input0_stride0 = TCR1->info.dim_stride_info.stride0;
  uint32_t input0_stride1 = TCR1->info.dim_stride_info.stride1;

  uint32_t input1_dim0 = TCR2->info.dim_stride_info.dim0;
  uint32_t input1_dim1 = TCR2->info.dim_stride_info.dim1;
  uint32_t input1_stride0 = TCR2->info.dim_stride_info.stride0;
  uint32_t input1_stride1 = TCR2->info.dim_stride_info.stride1;

  uint32_t output_dim0 = TCR3->info.dim_stride_info.dim0;
  uint32_t output_dim1 = TCR3->info.dim_stride_info.dim1;
  uint32_t output_stride0 = TCR3->info.dim_stride_info.stride0;
  uint32_t output_stride1 = TCR3->info.dim_stride_info.stride1;

  uint32_t rs1type = s->isa.instr.tensor_compute_64.rs1type;
  uint32_t rs2type = s->isa.instr.tensor_compute_64.rs2type;
  uint32_t rs3type = s->isa.instr.tensor_compute_64.rs3type;

  assert(input0_dim1 == input1_dim0);
  assert(input0_dim0 == output_dim0);
  assert(input1_dim1 == output_dim1);

  if (rs1type == MATMUL_INPUT_TYPE_U8 && rs2type == MATMUL_INPUT_TYPE_U8 &&
      rs3type == MATMUL_OUTPUT_TYPE_S32) {

    matmul_s32_u8_u8(input0, input1, output, input0_dim0, input0_dim1,
                     input1_dim0, input1_dim1, input0_stride0, input0_stride1,
                     input1_stride0, input1_stride1, output_stride0,
                     output_stride1);
  }
  if (rs1type == MATMUL_INPUT_TYPE_U8 && rs2type == MATMUL_INPUT_TYPE_U8 &&
      rs3type == MATMUL_OUTPUT_TYPE_S16) {

    matmul_s16_u8_u8(input0, input1, output, input0_dim0, input0_dim1,
                     input1_dim0, input1_dim1, input0_stride0, input0_stride1,
                     input1_stride0, input1_stride1, output_stride0,
                     output_stride1);
  }

  if (rs1type == MATMUL_INPUT_TYPE_S8 && rs2type == MATMUL_INPUT_TYPE_S8 &&
      rs3type == MATMUL_OUTPUT_TYPE_S32) {

    matmul_s32_s8_s8(input0, input1, output, input0_dim0, input0_dim1,
                     input1_dim0, input1_dim1, input0_stride0, input0_stride1,
                     input1_stride0, input1_stride1, output_stride0,
                     output_stride1);
  }
  if (rs1type == MATMUL_INPUT_TYPE_S8 && rs2type == MATMUL_INPUT_TYPE_S8 &&
      rs3type == MATMUL_OUTPUT_TYPE_S16) {

    matmul_s16_s8_s8(input0, input1, output, input0_dim0, input0_dim1,
                     input1_dim0, input1_dim1, input0_stride0, input0_stride1,
                     input1_stride0, input1_stride1, output_stride0,
                     output_stride1);
  }
}


def_EHelper(tx_conv) {
  int rs1 = s->src1.imm;
  int rs2 = s->src2.imm;
  int rs3 = s->src3.imm;

  float *input = (float *)cpu.gpr[rs1]._64;
  float *filter = (float *)cpu.gpr[rs2]._64;
  float *output = (float *)cpu.gpr[rs3]._64;

  uint32_t tcfgrs1 = s->isa.instr.tensor_compute_64.tcfgrs1;
  uint32_t tcfgrs2 = s->isa.instr.tensor_compute_64.tcfgrs2;
  uint32_t tcfgrs3 = s->isa.instr.tensor_compute_64.tcfgrs3;

  TensorConfigRegister *TCR1 = &TCR[tcfgrs1];
  TensorConfigRegister *TCR2 = &TCR[tcfgrs2];
  TensorConfigRegister *TCR3 = &TCR[tcfgrs3];

  // input [batch, height, width, in_channel]
  uint32_t batch = TCR1->info.dim_stride_info.dim3;
  uint32_t in_height = TCR1->info.dim_stride_info.dim2;
  uint32_t in_width = TCR1->info.dim_stride_info.dim1;
  uint32_t in_channel = TCR1->info.dim_stride_info.dim0;

  // kernel [out_channel, filter_height, filter_width, in_channel]
  uint32_t filter_height = TCR2->info.dim_stride_info.dim2;
  uint32_t filter_width = TCR2->info.dim_stride_info.dim1;
  uint32_t out_channel = TCR2->info.dim_stride_info.dim3;

  // output [batch, out_height, out_width, out_channel]
  uint32_t output_batch = TCR3->info.dim_stride_info.dim3;
  uint32_t out_height = TCR3->info.dim_stride_info.dim2;
  uint32_t out_width = TCR3->info.dim_stride_info.dim1;
  uint32_t output_channel = TCR3->info.dim_stride_info.dim0;

  // check input
  assert(in_channel == TCR2->info.dim_stride_info.dim0);
  assert(batch == output_batch);
  assert(out_channel == output_channel);

  // get padding and stride for conv2d
  uint32_t pad_width = Conv_CSR.info.pad_stride_info.pad0;
  uint32_t pad_height = Conv_CSR.info.pad_stride_info.pad1;
  uint32_t stride_width = Conv_CSR.info.pad_stride_info.stride0;
  uint32_t stride_height = Conv_CSR.info.pad_stride_info.stride1;

  // checkou output
  uint32_t expected_out_height = (in_height + 2 * pad_height - filter_height) / stride_height + 1;
  uint32_t expected_out_width = (in_width + 2 * pad_width - filter_width) / stride_width + 1;
  assert(out_height == expected_out_height);
  assert(out_width == expected_out_width);

  // get strides of the input and the kernel
  uint32_t input_stride_b = TCR1->info.dim_stride_info.stride3;
  uint32_t input_stride_h = TCR1->info.dim_stride_info.stride2;
  uint32_t input_stride_w = TCR1->info.dim_stride_info.stride1;
  uint32_t input_stride_c = TCR1->info.dim_stride_info.stride0;

  uint32_t filter_stride_oc = TCR2->info.dim_stride_info.stride3;
  uint32_t filter_stride_h = TCR2->info.dim_stride_info.stride2;
  uint32_t filter_stride_w = TCR2->info.dim_stride_info.stride1;
  uint32_t filter_stride_ic = TCR2->info.dim_stride_info.stride0;

  uint32_t output_stride_b = TCR3->info.dim_stride_info.stride3;
  uint32_t output_stride_h = TCR3->info.dim_stride_info.stride2;
  uint32_t output_stride_w = TCR3->info.dim_stride_info.stride1;
  uint32_t output_stride_c = TCR3->info.dim_stride_info.stride0;

  uint32_t rs1type = s->isa.instr.tensor_compute_64.rs1type;
  uint32_t rs2type = s->isa.instr.tensor_compute_64.rs2type;
  uint32_t rs3type = s->isa.instr.tensor_compute_64.rs3type;

  if (!has_init_conv2d_float_table) {
    has_init_conv2d_float_table = 1;
    init_conv2d_float_table();
  }

  Conv2DFunc func = conv2d_float_table[rs1type][rs2type][rs3type];

  int8_t *host_input = (int8_t *)uram_guest_to_host((paddr_t)input);
  int8_t *host_filter = (int8_t *)uram_guest_to_host((paddr_t)filter);
  int8_t *host_output = (int8_t *)uram_guest_to_host((paddr_t)output);

  if (!func) {
    printf("Cannot find the conv2d function for types %d, %d, %d\n", rs1type, rs2type, rs3type);
  } else {
    func(host_input, host_filter, host_output, batch, in_height, in_width, in_channel,
         filter_height, filter_width, out_channel, pad_height, pad_width,
         stride_height, stride_width, input_stride_b, input_stride_h, input_stride_w, input_stride_c,
         filter_stride_oc, filter_stride_h, filter_stride_w, filter_stride_ic,
         output_stride_b, output_stride_h, output_stride_w, output_stride_c);
  }
}

def_EHelper(tx_pooling) {
  int rs1 = s->src1.imm;
  int rs3 = s->src3.imm;

  float *input = (float *)cpu.gpr[rs1]._64;
  float *output = (float *)cpu.gpr[rs3]._64;

  uint32_t tcfgrs1 = s->isa.instr.tensor_compute_64.tcfgrs1;
  uint32_t tcfgrs3 = s->isa.instr.tensor_compute_64.tcfgrs3;

  TensorConfigRegister *TCR1 = &TCR[tcfgrs1];
  TensorConfigRegister *TCR3 = &TCR[tcfgrs3];

  // input [batch, height, width, in_channel]
  uint32_t batch = TCR1->info.dim_stride_info.dim3;
  uint32_t in_height = TCR1->info.dim_stride_info.dim2;
  uint32_t in_width = TCR1->info.dim_stride_info.dim1;
  uint32_t in_channel = TCR1->info.dim_stride_info.dim0;

  // output [batch, out_height, out_width, out_channel]
  uint32_t output_batch = TCR3->info.dim_stride_info.dim3;
  uint32_t out_height = TCR3->info.dim_stride_info.dim2;
  uint32_t out_width = TCR3->info.dim_stride_info.dim1;
  uint32_t output_channel = TCR3->info.dim_stride_info.dim0;

  // get padding and stride for conv2d
  uint32_t kernel_width = Pool_CSR.info.size_pad_stride_info.size0;
  uint32_t kernel_height = Pool_CSR.info.size_pad_stride_info.size1;
  uint32_t pad_width = Pool_CSR.info.size_pad_stride_info.pad0;
  uint32_t pad_height = Pool_CSR.info.size_pad_stride_info.pad1;
  uint32_t stride_width = Pool_CSR.info.size_pad_stride_info.stride0;
  uint32_t stride_height = Pool_CSR.info.size_pad_stride_info.stride1;

  uint32_t expect_h = (in_height + 2 * pad_height - kernel_height) / stride_height + 1;
  uint32_t expect_w = (in_width  + 2 * pad_width - kernel_width) / stride_width + 1;

  assert(output_batch   == batch);
  assert(output_channel == in_channel);
  assert(out_height     == expect_h);
  assert(out_width      == expect_w);

  // get strides of the input and the kernel
  uint32_t input_stride_b = TCR1->info.dim_stride_info.stride3;
  uint32_t input_stride_h = TCR1->info.dim_stride_info.stride2;
  uint32_t input_stride_w = TCR1->info.dim_stride_info.stride1;
  uint32_t input_stride_c = TCR1->info.dim_stride_info.stride0;

  uint32_t output_stride_b = TCR3->info.dim_stride_info.stride3;
  uint32_t output_stride_h = TCR3->info.dim_stride_info.stride2;
  uint32_t output_stride_w = TCR3->info.dim_stride_info.stride1;
  uint32_t output_stride_c = TCR3->info.dim_stride_info.stride0;

  uint32_t rs1type = s->isa.instr.tensor_compute_64.rs1type;
  uint32_t pool_op = s->isa.instr.tensor_compute_64.rs2type;
  uint32_t rs3type = s->isa.instr.tensor_compute_64.rs3type;

  if (!has_init_pooling_float_table) {
    has_init_pooling_float_table = 1;
    init_pooling_float_table();
  }
  
  PoolFunc func = NULL;

  int8_t *host_input = (int8_t *)uram_guest_to_host((paddr_t)input);
  int8_t *host_output = (int8_t *)uram_guest_to_host((paddr_t)output);

  if (pool_op == POOL_OP_maxpool) {
    func = pooling_float_table[POOL_OP_maxpool][rs1type][rs3type];
  }else if(pool_op == POOL_OP_avgpool){
    func = pooling_float_table[POOL_OP_avgpool][rs1type][rs3type];
  }

  if (!func) {
    printf("Cannot find the pooling function for types %d, %d\n", rs1type, rs3type);
  } else {
    func(host_input, host_output, batch, in_height, in_width, in_channel,
        kernel_height, kernel_width,
        pad_height, pad_width,
        stride_height, stride_width,
        input_stride_b, input_stride_h, input_stride_w, input_stride_c,
        output_stride_b, output_stride_h, output_stride_w, output_stride_c);
  }
}


/*
  The execution process of tensor config instructions.
*/
def_EHelper(tx_tcsr_xchg_nmask) {
  word_t rs3 = s->src3.imm;
  word_t rs1 = s->src1.imm;
  word_t rd = s->dest.imm;
  uint32_t tcsr_id = s->isa.instr.tensor_cfg_xchg_64.tcsr_id;
  uint32_t id = tcsr_id / 8;
  uint32_t index = tcsr_id % 8;

  uint32_t old_value = 0;

  if(id < 8) {         //TCSR
    TensorConfigRegister *tcr = &TCR[id];
    old_value = tcr->info.val[index];
  } else {
    if(tcsr_id - 64 < 4) {  //conv_csr
    old_value = Conv_CSR.info.val[tcsr_id - 64];
    } else {           //pool_csr
      old_value = Pool_CSR.info.val[tcsr_id - 68];
    }
  }

  if (rd)
    cpu.gpr[rd]._64 = old_value;

  uint64_t rs3_val = cpu.gpr[rs3]._64;
  uint64_t rs1_val = cpu.gpr[rs1]._64;
  
  uint32_t new_val = (rs1_val & ~rs3_val) | (old_value & rs3_val);
  if(id < 8) {         //TCSR
    TensorConfigRegister *tcr = &TCR[id];
    tcr->info.val[index] = new_val;
  } else {
    if(tcsr_id - 64 < 4) {  //conv_csr
      Conv_CSR.info.val[tcsr_id - 64] = new_val;
    } else {           //pool_csr
      Pool_CSR.info.val[tcsr_id - 68] = new_val;
    }
  }
}

def_EHelper(tx_tcsr_xchg_mask) {
  word_t rs3 = s->src3.imm;
  word_t rs1 = s->src1.imm;
  word_t rd = s->dest.imm;
  uint32_t tcsr_id = s->isa.instr.tensor_cfg_xchg_64.tcsr_id;
  uint32_t id = tcsr_id / 8;
  uint32_t index = tcsr_id % 8;

  uint32_t old_value = 0;

  if(id < 8) {         //TCSR
    TensorConfigRegister *tcr = &TCR[id];
    old_value = tcr->info.val[index];
  } else {
    if(tcsr_id - 64 < 4) {  //conv_csr
    old_value = Conv_CSR.info.val[tcsr_id - 64];
    } else {           //pool_csr
      old_value = Pool_CSR.info.val[tcsr_id - 68];
    }
  }

  if (rd)
    cpu.gpr[rd]._64 = old_value;

  uint64_t rs3_val = cpu.gpr[rs3]._64;
  uint64_t rs1_val = cpu.gpr[rs1]._64;
  
  uint32_t new_val = (rs1_val & rs3_val) | (old_value & ~rs3_val);
  if(id < 8) {         //TCSR
    TensorConfigRegister *tcr = &TCR[id];
    tcr->info.val[index] = new_val;
  } else {
    if(tcsr_id - 64 < 4) {  //conv_csr
      Conv_CSR.info.val[tcsr_id - 64] = new_val;
    } else {           //pool_csr
      Pool_CSR.info.val[tcsr_id - 68] = new_val;
    }
  }
}

def_EHelper(tx_tcsr_imm_nmask) {
  word_t rs3 = s->src3.imm;
  word_t rd = s->dest.imm;
  uint32_t imm_lo = s->isa.instr.tensor_cfg_imm_64.imm_lo;
  uint32_t imm_hi = s->isa.instr.tensor_cfg_imm_64.imm_hi;
  uint32_t tcsr_id = s->isa.instr.tensor_cfg_imm_64.tcsr_id;
  uint32_t imm = (imm_hi << 15) | imm_lo;
  uint32_t id = tcsr_id / 8;
  uint32_t index = tcsr_id % 8;
  TensorConfigRegister *tcr = &TCR[id];
  if (rd)
    cpu.gpr[rd]._64 = tcr->info.val[index];
  uint64_t rs3_val = cpu.gpr[rs3]._64;
  tcr->info.val[index] = (imm & ~rs3_val) | (imm & rs3_val);
}

def_EHelper(tx_tcsr_imm_mask) {
  word_t rs3 = s->src3.imm;
  word_t rd = s->dest.imm;
  uint32_t imm_lo = s->isa.instr.tensor_cfg_imm_64.imm_lo;
  uint32_t imm_hi = s->isa.instr.tensor_cfg_imm_64.imm_hi;
  uint32_t tcsr_id = s->isa.instr.tensor_cfg_imm_64.tcsr_id;
  uint32_t imm = (imm_hi << 15) | imm_lo;
  uint32_t id = tcsr_id / 8;
  uint32_t index = tcsr_id % 8;
  TensorConfigRegister *tcr = &TCR[id];
  if (rd)
    cpu.gpr[rd]._64 = tcr->info.val[index];
  uint64_t rs3_val = cpu.gpr[rs3]._64;
  tcr->info.val[index] = (imm & rs3_val) | (imm & ~rs3_val);
}

void tx_copy(void *src, void *dst, uint32_t src_dim0, uint32_t src_dim1,
                    uint32_t src_dim2, uint32_t src_dim3, uint32_t dst_dim0,
                    uint32_t dst_dim1, uint32_t dst_dim2, uint32_t dst_dim3,
                    uint32_t src_stride0, uint32_t src_stride1,
                    uint32_t src_stride2, uint32_t src_stride3,
                    uint32_t dst_stride0, uint32_t dst_stride1,
                    uint32_t dst_stride2, uint32_t dst_stride3,
                    uint32_t log2Length) {
  uint64_t src0 = 0, src1 = 0, src2 = 0, src3 = 0;
  uint64_t dst0 = 0, dst1 = 0, dst2 = 0, dst3 = 0;
  size_t sz = 1 << log2Length;
  while (1) {
    uint64_t src_offset = src0 * src_stride0 + src1 * src_stride1 +
                          src2 * src_stride2 + src3 * src_stride3;
    uint64_t dst_offset = dst0 * dst_stride0 + dst1 * dst_stride1 +
                          dst2 * dst_stride2 + dst3 * dst_stride3;
    memcpy(dst + dst_offset * sz, src + src_offset * sz, sz);
    src0 += 1;
    dst0 += 1;
    if (src0 >= src_dim0) {
      src0 = 0;
      src1 += 1;
    }
    if (src1 >= src_dim1) {
      src1 = 0;
      src2 += 1;
    }
    if (src2 >= src_dim2) {
      src2 = 0;
      src3 += 1;
    }
    if (src3 >= src_dim3) {
      src3 = 0;
      break;
    }
    if (dst0 >= dst_dim0) {
      dst0 = 0;
      dst1 += 1;
    }
    if (dst1 >= dst_dim1) {
      dst1 = 0;
      dst2 += 1;
    }
    if (dst2 >= dst_dim2) {
      dst2 = 0;
      dst3 += 1;
    }
  }
}

/*
  The execution process of tensor load store instructions.
*/
def_EHelper(tx_async_uram_copy) {
  int rs1 = s->src1.imm;
  int rs3 = s->src3.imm;

  void *src = (void *)cpu.gpr[rs1]._64;
  void *dst = (void *)cpu.gpr[rs3]._64;

  uint32_t tcfgrs1 = s->isa.instr.tensor_load_store_64.tcfgrs1;
  uint32_t tcfgrs3 = s->isa.instr.tensor_load_store_64.tcfgrs3;

  TensorConfigRegister *TCR1 = &TCR[tcfgrs1];
  TensorConfigRegister *TCR3 = &TCR[tcfgrs3];
  uint32_t src_dim0 = TCR1->info.dim_stride_info.dim0;
  uint32_t src_dim1 = TCR1->info.dim_stride_info.dim1;
  uint32_t src_dim2 = TCR1->info.dim_stride_info.dim2;
  uint32_t src_dim3 = TCR1->info.dim_stride_info.dim3;

  uint32_t src_stride0 = TCR1->info.dim_stride_info.stride0;
  uint32_t src_stride1 = TCR1->info.dim_stride_info.stride1;
  uint32_t src_stride2 = TCR1->info.dim_stride_info.stride2;
  uint32_t src_stride3 = TCR1->info.dim_stride_info.stride3;

  uint32_t dst_dim0 = TCR3->info.dim_stride_info.dim0;
  uint32_t dst_dim1 = TCR3->info.dim_stride_info.dim1;
  uint32_t dst_dim2 = TCR3->info.dim_stride_info.dim2;
  uint32_t dst_dim3 = TCR3->info.dim_stride_info.dim3;
  uint32_t dst_stride0 = TCR3->info.dim_stride_info.stride0;
  uint32_t dst_stride1 = TCR3->info.dim_stride_info.stride1;
  uint32_t dst_stride2 = TCR3->info.dim_stride_info.stride2;
  uint32_t dst_stride3 = TCR3->info.dim_stride_info.stride3;

  int mode = s->isa.instr.tensor_load_store_64.log2w;

  long size = src_dim0 * src_dim1 * src_dim2 * src_dim3;
  assert(size == dst_dim0 * dst_dim1 * dst_dim2 * dst_dim3);

  uint8_t *uram_src = (uint8_t *)uram_guest_to_host((paddr_t)src);
  uint8_t *uram_dst = (uint8_t *)uram_guest_to_host((paddr_t)dst);

  tx_copy(uram_src, uram_dst, src_dim0, src_dim1, src_dim2, src_dim3, dst_dim0,
          dst_dim1, dst_dim2, dst_dim3, src_stride0, src_stride1, src_stride2,
          src_stride3, dst_stride0, dst_stride1, dst_stride2, dst_stride3,
          mode);
}

def_EHelper(tx_async_uram_load) {
  int rs1 = s->src1.imm;
  int rs3 = s->src3.imm;

  void *src = (void *)cpu.gpr[rs1]._64;
  void *dst = (void *)cpu.gpr[rs3]._64;

  uint32_t tcfgrs1 = s->isa.instr.tensor_load_store_64.tcfgrs1;
  uint32_t tcfgrs3 = s->isa.instr.tensor_load_store_64.tcfgrs3;

  TensorConfigRegister *TCR1 = &TCR[tcfgrs1];
  TensorConfigRegister *TCR3 = &TCR[tcfgrs3];
  uint32_t src_dim0 = TCR1->info.dim_stride_info.dim0;
  uint32_t src_dim1 = TCR1->info.dim_stride_info.dim1;
  uint32_t src_dim2 = TCR1->info.dim_stride_info.dim2;
  uint32_t src_dim3 = TCR1->info.dim_stride_info.dim3;

  uint32_t src_stride0 = TCR1->info.dim_stride_info.stride0;
  uint32_t src_stride1 = TCR1->info.dim_stride_info.stride1;
  uint32_t src_stride2 = TCR1->info.dim_stride_info.stride2;
  uint32_t src_stride3 = TCR1->info.dim_stride_info.stride3;

  uint32_t dst_dim0 = TCR3->info.dim_stride_info.dim0;
  uint32_t dst_dim1 = TCR3->info.dim_stride_info.dim1;
  uint32_t dst_dim2 = TCR3->info.dim_stride_info.dim2;
  uint32_t dst_dim3 = TCR3->info.dim_stride_info.dim3;
  uint32_t dst_stride0 = TCR3->info.dim_stride_info.stride0;
  uint32_t dst_stride1 = TCR3->info.dim_stride_info.stride1;
  uint32_t dst_stride2 = TCR3->info.dim_stride_info.stride2;
  uint32_t dst_stride3 = TCR3->info.dim_stride_info.stride3;

  int mode = s->isa.instr.tensor_load_store_64.log2w;

  long size = src_dim0 * src_dim1 * src_dim2 * src_dim3;
  assert(size == dst_dim0 * dst_dim1 * dst_dim2 * dst_dim3);

  uint8_t *dram_src = (uint8_t *)uram_guest_to_host((paddr_t)src);
  uint8_t *uram_dst = (uint8_t *)uram_guest_to_host((paddr_t)dst);

  tx_copy(dram_src, uram_dst, src_dim0, src_dim1, src_dim2, src_dim3, dst_dim0,
          dst_dim1, dst_dim2, dst_dim3, src_stride0, src_stride1, src_stride2,
          src_stride3, dst_stride0, dst_stride1, dst_stride2, dst_stride3,
          mode);
}
def_EHelper(tx_async_uram_store) {
  int rs1 = s->src1.imm;
  int rs3 = s->src3.imm;

  void *src = (void *)cpu.gpr[rs1]._64;
  void *dst = (void *)cpu.gpr[rs3]._64;

  uint32_t tcfgrs1 = s->isa.instr.tensor_load_store_64.tcfgrs1;
  uint32_t tcfgrs3 = s->isa.instr.tensor_load_store_64.tcfgrs3;

  TensorConfigRegister *TCR1 = &TCR[tcfgrs1];
  TensorConfigRegister *TCR3 = &TCR[tcfgrs3];
  uint32_t src_dim0 = TCR1->info.dim_stride_info.dim0;
  uint32_t src_dim1 = TCR1->info.dim_stride_info.dim1;
  uint32_t src_dim2 = TCR1->info.dim_stride_info.dim2;
  uint32_t src_dim3 = TCR1->info.dim_stride_info.dim3;

  uint32_t src_stride0 = TCR1->info.dim_stride_info.stride0;
  uint32_t src_stride1 = TCR1->info.dim_stride_info.stride1;
  uint32_t src_stride2 = TCR1->info.dim_stride_info.stride2;
  uint32_t src_stride3 = TCR1->info.dim_stride_info.stride3;

  uint32_t dst_dim0 = TCR3->info.dim_stride_info.dim0;
  uint32_t dst_dim1 = TCR3->info.dim_stride_info.dim1;
  uint32_t dst_dim2 = TCR3->info.dim_stride_info.dim2;
  uint32_t dst_dim3 = TCR3->info.dim_stride_info.dim3;
  uint32_t dst_stride0 = TCR3->info.dim_stride_info.stride0;
  uint32_t dst_stride1 = TCR3->info.dim_stride_info.stride1;
  uint32_t dst_stride2 = TCR3->info.dim_stride_info.stride2;
  uint32_t dst_stride3 = TCR3->info.dim_stride_info.stride3;

  int mode = s->isa.instr.tensor_load_store_64.log2w;

  long size = src_dim0 * src_dim1 * src_dim2 * src_dim3;
  assert(size == dst_dim0 * dst_dim1 * dst_dim2 * dst_dim3);

  uint8_t *uram_src = (uint8_t *)uram_guest_to_host((paddr_t)src);
  uint8_t *dram_dst = (uint8_t *)uram_guest_to_host((paddr_t)dst);

  tx_copy(uram_src, dram_dst, src_dim0, src_dim1, src_dim2, src_dim3, dst_dim0,
          dst_dim1, dst_dim2, dst_dim3, src_stride0, src_stride1, src_stride2,
          src_stride3, dst_stride0, dst_stride1, dst_stride2, dst_stride3,
          mode);
}
def_EHelper(tx_sync_uram_copy) {
  int rs1 = s->src1.imm;
  int rs3 = s->src3.imm;

  void *src = (void *)cpu.gpr[rs1]._64;
  void *dst = (void *)cpu.gpr[rs3]._64;

  uint32_t tcfgrs1 = s->isa.instr.tensor_load_store_64.tcfgrs1;
  uint32_t tcfgrs3 = s->isa.instr.tensor_load_store_64.tcfgrs3;

  TensorConfigRegister *TCR1 = &TCR[tcfgrs1];
  TensorConfigRegister *TCR3 = &TCR[tcfgrs3];
  uint32_t src_dim0 = TCR1->info.dim_stride_info.dim0;
  uint32_t src_dim1 = TCR1->info.dim_stride_info.dim1;
  uint32_t src_dim2 = TCR1->info.dim_stride_info.dim2;
  uint32_t src_dim3 = TCR1->info.dim_stride_info.dim3;

  uint32_t src_stride0 = TCR1->info.dim_stride_info.stride0;
  uint32_t src_stride1 = TCR1->info.dim_stride_info.stride1;
  uint32_t src_stride2 = TCR1->info.dim_stride_info.stride2;
  uint32_t src_stride3 = TCR1->info.dim_stride_info.stride3;

  uint32_t dst_dim0 = TCR3->info.dim_stride_info.dim0;
  uint32_t dst_dim1 = TCR3->info.dim_stride_info.dim1;
  uint32_t dst_dim2 = TCR3->info.dim_stride_info.dim2;
  uint32_t dst_dim3 = TCR3->info.dim_stride_info.dim3;
  uint32_t dst_stride0 = TCR3->info.dim_stride_info.stride0;
  uint32_t dst_stride1 = TCR3->info.dim_stride_info.stride1;
  uint32_t dst_stride2 = TCR3->info.dim_stride_info.stride2;
  uint32_t dst_stride3 = TCR3->info.dim_stride_info.stride3;

  int mode = s->isa.instr.tensor_load_store_64.log2w;

  long size = src_dim0 * src_dim1 * src_dim2 * src_dim3;
  assert(size == dst_dim0 * dst_dim1 * dst_dim2 * dst_dim3);

  uint8_t *uram_src = (uint8_t *)uram_guest_to_host((paddr_t)src);
  uint8_t *uram_dst = (uint8_t *)uram_guest_to_host((paddr_t)dst);

  tx_copy(uram_src, uram_dst, src_dim0, src_dim1, src_dim2, src_dim3, dst_dim0,
          dst_dim1, dst_dim2, dst_dim3, src_stride0, src_stride1, src_stride2,
          src_stride3, dst_stride0, dst_stride1, dst_stride2, dst_stride3,
          mode);
}
def_EHelper(tx_sync_uram_load) {
  int rs1 = s->src1.imm;
  int rs3 = s->src3.imm;

  void *src = (void *)cpu.gpr[rs1]._64;
  void *dst = (void *)cpu.gpr[rs3]._64;

  uint32_t tcfgrs1 = s->isa.instr.tensor_load_store_64.tcfgrs1;
  uint32_t tcfgrs3 = s->isa.instr.tensor_load_store_64.tcfgrs3;

  TensorConfigRegister *TCR1 = &TCR[tcfgrs1];
  TensorConfigRegister *TCR3 = &TCR[tcfgrs3];
  uint32_t src_dim0 = TCR1->info.dim_stride_info.dim0;
  uint32_t src_dim1 = TCR1->info.dim_stride_info.dim1;
  uint32_t src_dim2 = TCR1->info.dim_stride_info.dim2;
  uint32_t src_dim3 = TCR1->info.dim_stride_info.dim3;

  uint32_t src_stride0 = TCR1->info.dim_stride_info.stride0;
  uint32_t src_stride1 = TCR1->info.dim_stride_info.stride1;
  uint32_t src_stride2 = TCR1->info.dim_stride_info.stride2;
  uint32_t src_stride3 = TCR1->info.dim_stride_info.stride3;

  uint32_t dst_dim0 = TCR3->info.dim_stride_info.dim0;
  uint32_t dst_dim1 = TCR3->info.dim_stride_info.dim1;
  uint32_t dst_dim2 = TCR3->info.dim_stride_info.dim2;
  uint32_t dst_dim3 = TCR3->info.dim_stride_info.dim3;
  uint32_t dst_stride0 = TCR3->info.dim_stride_info.stride0;
  uint32_t dst_stride1 = TCR3->info.dim_stride_info.stride1;
  uint32_t dst_stride2 = TCR3->info.dim_stride_info.stride2;
  uint32_t dst_stride3 = TCR3->info.dim_stride_info.stride3;

  int mode = s->isa.instr.tensor_load_store_64.log2w;
  uint8_t *dram_src = (uint8_t *)uram_guest_to_host((paddr_t)src);
  uint8_t *uram_dst = (uint8_t *)uram_guest_to_host((paddr_t)dst);

  tx_copy(dram_src, uram_dst, src_dim0, src_dim1, src_dim2, src_dim3, dst_dim0,
          dst_dim1, dst_dim2, dst_dim3, src_stride0, src_stride1, src_stride2,
          src_stride3, dst_stride0, dst_stride1, dst_stride2, dst_stride3,
          mode);
}

// TODO
def_EHelper(tx_sync_uram_store) {
  int rs1 = s->src1.imm;
  int rs3 = s->src3.imm;

  void *src = (void *)cpu.gpr[rs1]._64;
  void *dst = (void *)cpu.gpr[rs3]._64;

  uint32_t tcfgrs1 = s->isa.instr.tensor_load_store_64.tcfgrs1;
  uint32_t tcfgrs3 = s->isa.instr.tensor_load_store_64.tcfgrs3;

  TensorConfigRegister *TCR1 = &TCR[tcfgrs1];
  TensorConfigRegister *TCR3 = &TCR[tcfgrs3];
  uint32_t src_dim0 = TCR1->info.dim_stride_info.dim0;
  uint32_t src_dim1 = TCR1->info.dim_stride_info.dim1;
  uint32_t src_dim2 = TCR1->info.dim_stride_info.dim2;
  uint32_t src_dim3 = TCR1->info.dim_stride_info.dim3;

  uint32_t src_stride0 = TCR1->info.dim_stride_info.stride0;
  uint32_t src_stride1 = TCR1->info.dim_stride_info.stride1;
  uint32_t src_stride2 = TCR1->info.dim_stride_info.stride2;
  uint32_t src_stride3 = TCR1->info.dim_stride_info.stride3;

  uint32_t dst_dim0 = TCR3->info.dim_stride_info.dim0;
  uint32_t dst_dim1 = TCR3->info.dim_stride_info.dim1;
  uint32_t dst_dim2 = TCR3->info.dim_stride_info.dim2;
  uint32_t dst_dim3 = TCR3->info.dim_stride_info.dim3;
  uint32_t dst_stride0 = TCR3->info.dim_stride_info.stride0;
  uint32_t dst_stride1 = TCR3->info.dim_stride_info.stride1;
  uint32_t dst_stride2 = TCR3->info.dim_stride_info.stride2;
  uint32_t dst_stride3 = TCR3->info.dim_stride_info.stride3;

  int mode = s->isa.instr.tensor_load_store_64.log2w;

  long size = src_dim0 * src_dim1 * src_dim2 * src_dim3;
  assert(size == dst_dim0 * dst_dim1 * dst_dim2 * dst_dim3);

  uint8_t *uram_src = (uint8_t *)uram_guest_to_host((paddr_t)src);
  uint8_t *dram_dst = (uint8_t *)uram_guest_to_host((paddr_t)dst);

  tx_copy(uram_src, dram_dst, src_dim0, src_dim1, src_dim2, src_dim3, dst_dim0,
          dst_dim1, dst_dim2, dst_dim3, src_stride0, src_stride1, src_stride2,
          src_stride3, dst_stride0, dst_stride1, dst_stride2, dst_stride3,
          mode);
}

/*
  The execution process of tensor elment-wise instructions.
*/
def_EHelper(tx_int) {
  int rs1 = s->src1.imm;
  int rs2 = s->src2.imm;
  int rs3 = s->src3.imm;

  float *input0 = (float *)cpu.gpr[rs1]._64;
  float *input1 = (float *)cpu.gpr[rs2]._64;
  float *dst = (float *)cpu.gpr[rs3]._64;
  uint32_t tcfgrs1 = s->isa.instr.tensor_element_wise_compute_64.tcfgrs1;
  uint32_t tcfgrs2 = s->isa.instr.tensor_element_wise_compute_64.tcfgrs2;
  // uint32_t tcfgrs3 = s->isa.instr.tensor_element_wise_compute_64.tcfgrs3;

  TensorConfigRegister *TCR1 = &TCR[tcfgrs1];
  TensorConfigRegister *TCR2 = &TCR[tcfgrs2];
  // TensorConfigRegister *TCR3 = &TCR[tcfgrs3];
  uint32_t input0_dim0 = TCR1->info.dim_stride_info.dim0;
  uint32_t input0_dim1 = TCR1->info.dim_stride_info.dim1;
  uint32_t input0_dim2 = TCR1->info.dim_stride_info.dim2;
  uint32_t input0_dim3 = TCR1->info.dim_stride_info.dim3;
  uint32_t input0_stride0 = TCR1->info.dim_stride_info.stride0;
  uint32_t input0_stride1 = TCR1->info.dim_stride_info.stride1;
  uint32_t input0_stride2 = TCR1->info.dim_stride_info.stride2;
  uint32_t input0_stride3 = TCR1->info.dim_stride_info.stride3;

  uint32_t input1_dim0 = TCR2->info.dim_stride_info.dim0;
  uint32_t input1_dim1 = TCR2->info.dim_stride_info.dim1;
  uint32_t input1_dim2 = TCR2->info.dim_stride_info.dim2;
  uint32_t input1_dim3 = TCR2->info.dim_stride_info.dim3;
  uint32_t input1_stride0 = TCR2->info.dim_stride_info.stride0;
  uint32_t input1_stride1 = TCR2->info.dim_stride_info.stride1;
  uint32_t input1_stride2 = TCR2->info.dim_stride_info.stride2;
  uint32_t input1_stride3 = TCR2->info.dim_stride_info.stride3;

  uint32_t type_lo = s->isa.instr.tensor_element_wise_compute_64.type_lo;

  uint32_t op = s->isa.instr.tensor_element_wise_compute_64.op;

  uint32_t broadcast = s->isa.instr.tensor_element_wise_compute_64.broadcast;

  int8_t *host_input0 = (int8_t *)uram_guest_to_host((paddr_t)input0);
  int8_t *host_input1 = (int8_t *)uram_guest_to_host((paddr_t)input1);
  int8_t *host_output = (int8_t *)uram_guest_to_host((paddr_t)dst);

  if (broadcast == 0) {
    assert(input0_dim0 == input1_dim0);
    assert(input0_dim1 == input1_dim1);
    assert(input0_dim2 == input1_dim2);
    assert(input0_dim3 == input1_dim3);
    tensor_op(host_input0, host_input1, host_output, input0_dim0, input0_dim1,
              input0_dim2, input0_dim3, input0_stride0, input0_stride1,
              input0_stride2, input0_stride3, type_lo, op);
  } else {
    int32_t dim1[4] = {input0_dim0, input0_dim1, input0_dim2, input0_dim3};
    int32_t dim2[4] = {input1_dim0, input1_dim1, input1_dim2, input1_dim3};
    int32_t stride1[4] = {input0_stride0, input0_stride1, input0_stride2,
                          input0_stride3};
    int32_t stride2[4] = {input1_stride0, input1_stride1, input1_stride2,
                          input1_stride3};
    tensor_op_broadcast(host_input0, host_input1, host_output, dim1, dim2,
                        stride1, stride2, type_lo, op);
  }
}

def_EHelper(tx_fp) {
  int rs1 = s->src1.imm;
  int rs2 = s->src2.imm;
  int rs3 = s->src3.imm;

  float *input0 = (float *)cpu.gpr[rs1]._64;
  float *input1 = (float *)cpu.gpr[rs2]._64;
  float *dst = (float *)cpu.gpr[rs3]._64;

  uint32_t tcfgrs1 = s->isa.instr.tensor_element_wise_compute_64.tcfgrs1;
  uint32_t tcfgrs2 = s->isa.instr.tensor_element_wise_compute_64.tcfgrs2;

  TensorConfigRegister *TCR1 = &TCR[tcfgrs1];
  TensorConfigRegister *TCR2 = &TCR[tcfgrs2];

  uint32_t input0_dim0 = TCR1->info.dim_stride_info.dim0;
  uint32_t input0_dim1 = TCR1->info.dim_stride_info.dim1;
  uint32_t input0_dim2 = TCR1->info.dim_stride_info.dim2;
  uint32_t input0_dim3 = TCR1->info.dim_stride_info.dim3;
  uint32_t input0_stride0 = TCR1->info.dim_stride_info.stride0;
  uint32_t input0_stride1 = TCR1->info.dim_stride_info.stride1;
  uint32_t input0_stride2 = TCR1->info.dim_stride_info.stride2;
  uint32_t input0_stride3 = TCR1->info.dim_stride_info.stride3;

  uint32_t input1_dim0 = TCR2->info.dim_stride_info.dim0;
  uint32_t input1_dim1 = TCR2->info.dim_stride_info.dim1;
  uint32_t input1_dim2 = TCR2->info.dim_stride_info.dim2;
  uint32_t input1_dim3 = TCR2->info.dim_stride_info.dim3;
  uint32_t input1_stride0 = TCR2->info.dim_stride_info.stride0;
  uint32_t input1_stride1 = TCR2->info.dim_stride_info.stride1;
  uint32_t input1_stride2 = TCR2->info.dim_stride_info.stride2;
  uint32_t input1_stride3 = TCR2->info.dim_stride_info.stride3;

  uint32_t type_lo = s->isa.instr.tensor_element_wise_compute_64.type_lo;

  uint32_t op = s->isa.instr.tensor_element_wise_compute_64.op;

  uint32_t broadcast = s->isa.instr.tensor_element_wise_compute_64.broadcast;

  int8_t *host_input0 = (int8_t *)uram_guest_to_host((paddr_t)input0);
  int8_t *host_input1 = (int8_t *)uram_guest_to_host((paddr_t)input1);
  int8_t *host_output = (int8_t *)uram_guest_to_host((paddr_t)dst);

  if (broadcast == 0) {
    assert(input0_dim0 == input1_dim0);
    assert(input0_dim1 == input1_dim1);
    assert(input0_dim2 == input1_dim2);
    assert(input0_dim3 == input1_dim3);
    tensor_op_fp(host_input0, host_input1, host_output, input0_dim0,
                 input0_dim1, input0_dim2, input0_dim3, input0_stride0,
                 input0_stride1, input0_stride2, input0_stride3, type_lo, op);

  } else {
    int32_t dim1[4] = {input0_dim0, input0_dim1, input0_dim2, input0_dim3};
    int32_t dim2[4] = {input1_dim0, input1_dim1, input1_dim2, input1_dim3};
    int32_t stride1[4] = {input0_stride0, input0_stride1, input0_stride2,
                          input0_stride3};
    int32_t stride2[4] = {input1_stride0, input1_stride1, input1_stride2,
                          input1_stride3};
    if (!tensor_op_broadcast_fp(host_input0, host_input1, host_output, dim1,
                                dim2, stride1, stride2, type_lo, op))
      printf("Error tx_fp\n");
  }
}

def_EHelper(tx_lut_int) {
  int rs1 = s->src1.imm;
  int rs2 = s->src2.imm;
  int rs3 = s->src3.imm;

  uint64_t *input = (uint64_t *)cpu.gpr[rs1]._64;
  uint64_t *lut_table = (uint64_t *)cpu.gpr[rs2]._64;
  uint64_t *output = (uint64_t *)cpu.gpr[rs3]._64;

  uint32_t tcfgrs1 = s->isa.instr.tensor_element_wise_compute_64.tcfgrs1;
  uint32_t width = s->isa.instr.tensor_element_wise_compute_64.tcfgrs2;

  TensorConfigRegister *TCR1 = &TCR[tcfgrs1];
  uint32_t input_dim0 = TCR1->info.dim_stride_info.dim0;

  uint32_t log2Length = s->isa.instr.tensor_element_wise_compute_64.op;

  int32_t *host_input = (int32_t *)uram_guest_to_host((paddr_t)input);
  int32_t *host_lut_table = (int32_t *)uram_guest_to_host((paddr_t)lut_table);
  int32_t *host_output = (int32_t *)uram_guest_to_host((paddr_t)output);

  for (int i = 0; i < input_dim0; i++) {
    uint32_t val = host_input[i];
    uint32_t index = (uint32_t)(val & ((1U << log2Length) - 1));
    for (int j = 0; j < width; j++) {
      host_output[i * width + j] = host_lut_table[index * width + j];
    }
  }
}

def_EHelper(tx_lut_fp) {}

def_EHelper(tx_int_reduce) {
  int rs1 = s->src1.imm;
  int rs3 = s->src3.imm;

  float *input0 = (float *)cpu.gpr[rs1]._64;
  float *dst = (float *)cpu.gpr[rs3]._64;

  uint32_t tcfgrs1 = s->isa.instr.tensor_element_wise_compute_64.tcfgrs1;

  TensorConfigRegister *TCR1 = &TCR[tcfgrs1];

  uint32_t input0_dim0 = TCR1->info.dim_stride_info.dim0;
  uint32_t input0_dim1 = TCR1->info.dim_stride_info.dim1;
  uint32_t input0_dim2 = TCR1->info.dim_stride_info.dim2;
  uint32_t input0_dim3 = TCR1->info.dim_stride_info.dim3;
  uint32_t input0_stride0 = TCR1->info.dim_stride_info.stride0;
  uint32_t input0_stride1 = TCR1->info.dim_stride_info.stride1;
  uint32_t input0_stride2 = TCR1->info.dim_stride_info.stride2;
  uint32_t input0_stride3 = TCR1->info.dim_stride_info.stride3;

  uint32_t type_lo = s->isa.instr.tensor_element_wise_compute_64.type_lo;

  uint32_t op = s->isa.instr.tensor_element_wise_compute_64.op;
  uint32_t reduce = s->isa.instr.tensor_element_wise_compute_64.dim;

  int32_t dims[4] = {input0_dim0, input0_dim1, input0_dim2, input0_dim3};
  int32_t strides[4] = {input0_stride0, input0_stride1, input0_stride2,
                        input0_stride3};
  int reduce_axis[4] = {0};
  if (reduce == 0) {
    reduce_axis[0] = 1;
  }
  if (reduce == 1) {
    reduce_axis[1] = 1;
  }
  if (reduce == 2) {
    reduce_axis[2] = 1;
  }
  if (reduce == 3) {
    reduce_axis[3] = 1;
  }

  int8_t *host_input = (int8_t *)uram_guest_to_host((paddr_t)input0);
  int8_t *host_output = (int8_t *)uram_guest_to_host((paddr_t)dst);
  tensor_reduce(host_input, host_output, dims, strides, reduce_axis, type_lo,
                op);
}

def_EHelper(tx_fp_reduce) {
  int rs1 = s->src1.imm;
  int rs3 = s->src3.imm;

  float *input0 = (float *)cpu.gpr[rs1]._64;
  float *dst = (float *)cpu.gpr[rs3]._64;

  uint32_t tcfgrs1 = s->isa.instr.tensor_element_wise_compute_64.tcfgrs1;

  TensorConfigRegister *TCR1 = &TCR[tcfgrs1];

  uint32_t input0_dim0 = TCR1->info.dim_stride_info.dim0;
  uint32_t input0_dim1 = TCR1->info.dim_stride_info.dim1;
  uint32_t input0_dim2 = TCR1->info.dim_stride_info.dim2;
  uint32_t input0_dim3 = TCR1->info.dim_stride_info.dim3;
  uint32_t input0_stride0 = TCR1->info.dim_stride_info.stride0;
  uint32_t input0_stride1 = TCR1->info.dim_stride_info.stride1;
  uint32_t input0_stride2 = TCR1->info.dim_stride_info.stride2;
  uint32_t input0_stride3 = TCR1->info.dim_stride_info.stride3;

  uint32_t type_lo = s->isa.instr.tensor_element_wise_compute_64.type_lo;

  uint32_t op = s->isa.instr.tensor_element_wise_compute_64.op;
  uint32_t reduce = s->isa.instr.tensor_element_wise_compute_64.dim;

  int32_t dims[4] = {input0_dim0, input0_dim1, input0_dim2, input0_dim3};
  int32_t strides[4] = {input0_stride0, input0_stride1, input0_stride2,
                        input0_stride3};
  int reduce_axis[4] = {0};
  if (reduce == 0) {
    reduce_axis[0] = 1;
  }
  if (reduce == 1) {
    reduce_axis[1] = 1;
  }
  if (reduce == 2) {
    reduce_axis[2] = 1;
  }
  if (reduce == 3) {
    reduce_axis[3] = 1;
  }
  int8_t *host_input = (int8_t *)uram_guest_to_host((paddr_t)input0);
  int8_t *host_output = (int8_t *)uram_guest_to_host((paddr_t)dst);
  tensor_reduce_fp(host_input, host_output, dims, strides, reduce_axis, type_lo,
                   op);
}

/*
  The execution process of convert instructions.
*/
def_EHelper(tx_convert) {
  int rs1 = s->src1.imm;
  int rs3 = s->src3.imm;

  float *input = (float *)cpu.gpr[rs1]._64;
  float *output = (float *)cpu.gpr[rs3]._64;

  uint32_t tcfgrs1 = s->isa.instr.tensor_convert_64.tcfgrs1;
  uint32_t tcfgrs3 = s->isa.instr.tensor_convert_64.tcfgrs3;
  uint8_t mode = s->isa.instr.tensor_convert_64.mode;

  TensorConfigRegister *TCR1 = &TCR[tcfgrs1];
  TensorConfigRegister *TCR3 = &TCR[tcfgrs3];
  uint32_t input_dim0 = TCR1->info.dim_stride_info.dim0;
  uint32_t input_dim1 = TCR1->info.dim_stride_info.dim1;
  uint32_t input_dim2 = TCR1->info.dim_stride_info.dim2;
  uint32_t input_dim3 = TCR1->info.dim_stride_info.dim3;
  uint32_t input_stride0 = TCR1->info.dim_stride_info.stride0;
  uint32_t input_stride1 = TCR1->info.dim_stride_info.stride1;
  uint32_t input_stride2 = TCR1->info.dim_stride_info.stride2;
  uint32_t input_stride3 = TCR1->info.dim_stride_info.stride3;

  uint32_t output_dim0 = TCR3->info.dim_stride_info.dim0;
  uint32_t output_dim1 = TCR3->info.dim_stride_info.dim1;
  uint32_t output_dim2 = TCR3->info.dim_stride_info.dim2;
  uint32_t output_dim3 = TCR3->info.dim_stride_info.dim3;
  uint32_t output_stride0 = TCR3->info.dim_stride_info.stride0;
  uint32_t output_stride1 = TCR3->info.dim_stride_info.stride1;
  uint32_t output_stride2 = TCR3->info.dim_stride_info.stride2;
  uint32_t output_stride3 = TCR3->info.dim_stride_info.stride3;

  uint32_t type_hi = s->isa.instr.tensor_convert_64.rs1type_hi;
  uint32_t type_lo = s->isa.instr.tensor_convert_64.rs1type_lo;
  uint32_t input_type = (type_hi << 3) | type_lo;
  uint32_t dst_type = s->isa.instr.tensor_convert_64.rs3type;

  int32_t input_dims[4] = {input_dim0, input_dim1, input_dim2, input_dim3};
  int32_t input_strides[4] = {input_stride0, input_stride1, input_stride2,
                              input_stride3};

  int32_t output_strides[4] = {output_stride0, output_stride1, output_stride2,
                               output_stride3};

  assert(input_dim0 == output_dim0);
  assert(input_dim1 == output_dim1);
  assert(input_dim2 == output_dim2);
  assert(input_dim3 == output_dim3);
  int8_t *host_input = (int8_t *)uram_guest_to_host((paddr_t)input);
  int8_t *host_output = (int8_t *)uram_guest_to_host((paddr_t)output);
  tensor_convert(host_input, host_output, input_dims, input_strides,
                 output_strides, input_type, dst_type, mode);
}

/*
  The execution process of SFU instructions.
*/
def_EHelper(tx_sfu) {
  int rs1 = s->src1.imm;
  int rs3 = s->src3.imm;

  float *input = (float *)cpu.gpr[rs1]._64;
  float *output = (float *)cpu.gpr[rs3]._64;

  uint32_t tcfgrs1 = s->isa.instr.tensor_sfu_64.tcfgrs1;
  uint32_t tcfgrs3 = s->isa.instr.tensor_sfu_64.tcfgrs3;
  uint8_t mode = s->isa.instr.tensor_sfu_64.mode;
  uint8_t op = s->isa.instr.tensor_sfu_64.op;

  TensorConfigRegister *TCR1 = &TCR[tcfgrs1];
  TensorConfigRegister *TCR3 = &TCR[tcfgrs3];
  uint32_t input_dim0 = TCR1->info.dim_stride_info.dim0;
  uint32_t input_dim1 = TCR1->info.dim_stride_info.dim1;
  uint32_t input_dim2 = TCR1->info.dim_stride_info.dim2;
  uint32_t input_dim3 = TCR1->info.dim_stride_info.dim3;
  uint32_t input_stride0 = TCR1->info.dim_stride_info.stride0;
  uint32_t input_stride1 = TCR1->info.dim_stride_info.stride1;
  uint32_t input_stride2 = TCR1->info.dim_stride_info.stride2;
  uint32_t input_stride3 = TCR1->info.dim_stride_info.stride3;

  uint32_t output_dim0 = TCR3->info.dim_stride_info.dim0;
  uint32_t output_dim1 = TCR3->info.dim_stride_info.dim1;
  uint32_t output_dim2 = TCR3->info.dim_stride_info.dim2;
  uint32_t output_dim3 = TCR3->info.dim_stride_info.dim3;
  uint32_t output_stride0 = TCR3->info.dim_stride_info.stride0;
  uint32_t output_stride1 = TCR3->info.dim_stride_info.stride1;
  uint32_t output_stride2 = TCR3->info.dim_stride_info.stride2;
  uint32_t output_stride3 = TCR3->info.dim_stride_info.stride3;

  uint32_t type_hi = s->isa.instr.tensor_sfu_64.type_hi;
  uint32_t type_lo = s->isa.instr.tensor_sfu_64.type_lo;
  uint32_t input_type = (type_hi << 3) | type_lo;

  int32_t input_dims[4] = {input_dim0, input_dim1, input_dim2, input_dim3};
  int32_t input_strides[4] = {input_stride0, input_stride1, input_stride2,
                              input_stride3};

  int32_t output_strides[4] = {output_stride0, output_stride1, output_stride2,
                               output_stride3};

  assert(input_dim0 == output_dim0);
  assert(input_dim1 == output_dim1);
  assert(input_dim2 == output_dim2);
  assert(input_dim3 == output_dim3);
  assert(input_type == CONVERT_FP32);
  int8_t *host_input = (int8_t *)uram_guest_to_host((paddr_t)input);
  int8_t *host_output = (int8_t *)uram_guest_to_host((paddr_t)output);
  tensor_sfu_fp32((float *)host_input, (float *)host_output, input_dims,
                  input_strides, output_strides, op, mode);
}

#endif
#endif
