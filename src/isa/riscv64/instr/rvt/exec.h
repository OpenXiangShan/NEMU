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

#ifndef RISCV_CUSTOM_TENSOR_EXEC_H
#define RISCV_CUSTOM_TENSOR_EXEC_H
#include "register.h"

#include "conv2d.h"
#include "convert.h"
#include "element_wise.h"
#include "matmul.h"
#include "matmul_subfloat.h"
#include "pooling.h"
#include "sfu.h"
#include "tensor_copy.h"
#include "tensor_init.h"
#include "type.h"
#include <memory/paddr.h>
#include <time.h>

// ======================================
// tx.matmul.float instructions
// ======================================

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

  if (!has_init_matmul_float_table) {
    init_matmul_float_table();
  }

  MatMulFunc func = matmul_float_table[rs1type][rs2type][rs3type];

  int8_t *host_input0 = (int8_t *)uram_guest_to_host((paddr_t)input0);
  int8_t *host_input1 = (int8_t *)uram_guest_to_host((paddr_t)input1);
  int8_t *host_output = (int8_t *)uram_guest_to_host((paddr_t)output);

  if (!func) {
    printf("Cannot find the function\n");
  } else {
    MatMulConfig config;
    MatMulStrideConfig strides;

    create_matmul_config(&config, input0_dim0, input0_dim1, input1_dim0,
                         input1_dim1);
    create_matmul_stride_config(&strides, input0_stride0, input0_stride1,
                                input1_stride0, input1_stride1, output_stride0,
                                output_stride1);

    func(host_input0, host_input1, host_output, &config, &strides);
  }
}

// ======================================
// tx.matmul.subfloat instructions
// ======================================
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
    init_matmul_subfloat_table();
  }

  MatMulConfig config;
  MatMulStrideConfig strides;

  create_matmul_config(&config, input0_dim0, input0_dim1, input1_dim0,
                       input1_dim1);
  create_matmul_stride_config(&strides, input0_stride0, input0_stride1,
                              input1_stride0, input1_stride1, output_stride0,
                              output_stride1);

  int8_t *host_input0 = (int8_t *)uram_guest_to_host((paddr_t)input0);
  int8_t *host_input1 = (int8_t *)uram_guest_to_host((paddr_t)input1);
  int8_t *host_output = (int8_t *)uram_guest_to_host((paddr_t)output);

  MatMulSubfloatFunc func = matmul_subfloat_table[rs1type][rs2type][rs3type];
  if (func) {
    func(host_input0, host_input1, host_output, &config, &strides);
  } else {
    printf("Cannot find subfloat matmul function for types %d, %d, %d\n",
           rs1type, rs2type, rs3type);
  }
}

// ======================================
// tx.matmul.fix instructions
// ======================================
// TODO. Support more types.
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

  MatMulConfig config;
  MatMulStrideConfig strides;

  create_matmul_config(&config, input0_dim0, input0_dim1, input1_dim0,
                       input1_dim1);
  create_matmul_stride_config(&strides, input0_stride0, input0_stride1,
                              input1_stride0, input1_stride1, output_stride0,
                              output_stride1);

  int8_t *host_input0 = (int8_t *)uram_guest_to_host((paddr_t)input0);
  int8_t *host_input1 = (int8_t *)uram_guest_to_host((paddr_t)input1);
  int8_t *host_output = (int8_t *)uram_guest_to_host((paddr_t)output);

  if (rs1type == MATMUL_INPUT_TYPE_U8 && rs2type == MATMUL_INPUT_TYPE_U8 &&
      rs3type == MATMUL_OUTPUT_TYPE_S32) {
    matmul_s32_u8_u8(host_input0, host_input1, host_output, &config, &strides);
  } else if (rs1type == MATMUL_INPUT_TYPE_U8 &&
             rs2type == MATMUL_INPUT_TYPE_U8 &&
             rs3type == MATMUL_OUTPUT_TYPE_S16) {
    matmul_s16_u8_u8(host_input0, host_input1, host_output, &config, &strides);
  } else if (rs1type == MATMUL_INPUT_TYPE_S8 &&
             rs2type == MATMUL_INPUT_TYPE_S8 &&
             rs3type == MATMUL_OUTPUT_TYPE_S32) {
    matmul_s32_s8_s8(host_input0, host_input1, host_output, &config, &strides);
  } else if (rs1type == MATMUL_INPUT_TYPE_S8 &&
             rs2type == MATMUL_INPUT_TYPE_S8 &&
             rs3type == MATMUL_OUTPUT_TYPE_S16) {
    matmul_s16_s8_s8(host_input0, host_input1, host_output, &config, &strides);
  } else {
    printf("Unsupported integer matmul type combination: %d x %d -> %d\n",
           rs1type, rs2type, rs3type);
  }
}

// ======================================
// tx.conv.float instructions
// ======================================
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

  // check output
  uint32_t expected_out_height =
      (in_height + 2 * pad_height - filter_height) / stride_height + 1;
  uint32_t expected_out_width =
      (in_width + 2 * pad_width - filter_width) / stride_width + 1;
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
    init_conv2d_float_table();
  }

  Conv2DFunc func = conv2d_float_table[rs1type][rs2type][rs3type];

  int8_t *host_input = (int8_t *)uram_guest_to_host((paddr_t)input);
  int8_t *host_filter = (int8_t *)uram_guest_to_host((paddr_t)filter);
  int8_t *host_output = (int8_t *)uram_guest_to_host((paddr_t)output);

  if (!func) {
    printf("Cannot find the conv2d function for types %d, %d, %d\n", rs1type,
           rs2type, rs3type);
  } else {
    Conv2DConfig config;
    Conv2DStrideConfig strides;

    create_conv2d_config(&config, batch, in_height, in_width, in_channel,
                         filter_height, filter_width, out_channel, pad_height,
                         pad_width, stride_height, stride_width);

    create_conv2d_stride_config(
        &strides, input_stride_b, input_stride_h, input_stride_w,
        input_stride_c, filter_stride_oc, filter_stride_h, filter_stride_w,
        filter_stride_ic, output_stride_b, output_stride_h, output_stride_w,
        output_stride_c);

    func(host_input, host_filter, host_output, &config, &strides);
  }
}

// ======================================
// tx.pool.float instructions
// ======================================
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

  // get pooling parameters from control register
  uint32_t kernel_width = Pool_CSR.info.size_pad_stride_info.size0;
  uint32_t kernel_height = Pool_CSR.info.size_pad_stride_info.size1;
  uint32_t pad_width = Pool_CSR.info.size_pad_stride_info.pad0;
  uint32_t pad_height = Pool_CSR.info.size_pad_stride_info.pad1;
  uint32_t stride_width = Pool_CSR.info.size_pad_stride_info.stride0;
  uint32_t stride_height = Pool_CSR.info.size_pad_stride_info.stride1;

  // Verify output dimensions
  uint32_t expected_height =
      (in_height + 2 * pad_height - kernel_height) / stride_height + 1;
  uint32_t expected_width =
      (in_width + 2 * pad_width - kernel_width) / stride_width + 1;

  assert(output_batch == batch);
  assert(output_channel == in_channel);
  assert(out_height == expected_height);
  assert(out_width == expected_width);

  // get strides of the input and output tensors
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
    init_pooling_float_table();
  }

  // Get the appropriate pooling function from the table
  PoolFunc func = NULL;
  if (pool_op == POOL_OP_maxpool) {
    func = pooling_float_table[POOL_OP_maxpool][rs1type][rs3type];
  } else if (pool_op == POOL_OP_avgpool) {
    func = pooling_float_table[POOL_OP_avgpool][rs1type][rs3type];
  }

  int8_t *host_input = (int8_t *)uram_guest_to_host((paddr_t)input);
  int8_t *host_output = (int8_t *)uram_guest_to_host((paddr_t)output);

  if (!func) {
    printf("Cannot find the pooling function for types %d, %d\n", rs1type,
           rs3type);
  } else {
    // Create configuration structures
    PoolingConfig config;
    PoolingStrideConfig strides;

    int is_max = (pool_op == POOL_OP_maxpool) ? 1 : 0;
    create_pooling_config(&config, batch, in_height, in_width, in_channel,
                          kernel_height, kernel_width, pad_height, pad_width,
                          stride_height, stride_width, is_max);

    create_pooling_stride_config(&strides, input_stride_b, input_stride_h,
                                 input_stride_w, input_stride_c,
                                 output_stride_b, output_stride_h,
                                 output_stride_w, output_stride_c);

    // Call the pooling function
    func(host_input, host_output, &config, &strides);
  }
}

// ======================================
// tx.tcsr.xchg.nmask instruction
// ======================================
def_EHelper(tx_tcsr_xchg_nmask) {
  word_t rs3 = s->src3.imm;
  word_t rs1 = s->src1.imm;
  word_t rd = s->dest.imm;
  uint32_t tcsr_id = s->isa.instr.tensor_cfg_xchg_64.tcsr_id;
  uint32_t id = tcsr_id / 8;
  uint32_t index = tcsr_id % 8;

  uint32_t old_value = 0;

  if (id < 8) { // TCSR
    TensorConfigRegister *tcr = &TCR[id];
    old_value = tcr->info.val[index];
  } else {
    if (tcsr_id - 64 < 4) { // conv_csr
      old_value = Conv_CSR.info.val[tcsr_id - 64];
    } else { // pool_csr
      old_value = Pool_CSR.info.val[tcsr_id - 68];
    }
  }

  if (rd)
    cpu.gpr[rd]._64 = old_value;

  uint64_t rs3_val = cpu.gpr[rs3]._64;
  uint64_t rs1_val = cpu.gpr[rs1]._64;

  uint32_t new_val = (rs1_val & ~rs3_val) | (old_value & rs3_val);
  if (id < 8) { // TCSR
    TensorConfigRegister *tcr = &TCR[id];
    tcr->info.val[index] = new_val;
  } else {
    if (tcsr_id - 64 < 4) { // conv_csr
      Conv_CSR.info.val[tcsr_id - 64] = new_val;
    } else { // pool_csr
      Pool_CSR.info.val[tcsr_id - 68] = new_val;
    }
  }
}

// ======================================
// tx.tcsr.xchg.mask instruction
// ======================================
def_EHelper(tx_tcsr_xchg_mask) {
  word_t rs3 = s->src3.imm;
  word_t rs1 = s->src1.imm;
  word_t rd = s->dest.imm;
  uint32_t tcsr_id = s->isa.instr.tensor_cfg_xchg_64.tcsr_id;
  uint32_t id = tcsr_id / 8;
  uint32_t index = tcsr_id % 8;

  uint32_t old_value = 0;

  if (id < 8) { // TCSR
    TensorConfigRegister *tcr = &TCR[id];
    old_value = tcr->info.val[index];
  } else {
    if (tcsr_id - 64 < 4) { // conv_csr
      old_value = Conv_CSR.info.val[tcsr_id - 64];
    } else { // pool_csr
      old_value = Pool_CSR.info.val[tcsr_id - 68];
    }
  }

  if (rd)
    cpu.gpr[rd]._64 = old_value;

  uint64_t rs3_val = cpu.gpr[rs3]._64;
  uint64_t rs1_val = cpu.gpr[rs1]._64;

  uint32_t new_val = (rs1_val & rs3_val) | (old_value & ~rs3_val);
  if (id < 8) { // TCSR
    TensorConfigRegister *tcr = &TCR[id];
    tcr->info.val[index] = new_val;
  } else {
    if (tcsr_id - 64 < 4) { // conv_csr
      Conv_CSR.info.val[tcsr_id - 64] = new_val;
    } else { // pool_csr
      Pool_CSR.info.val[tcsr_id - 68] = new_val;
    }
  }
}

// ======================================
// tx.tcsr.imm.nmask instruction
// ======================================
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

// ======================================
// tx.tcsr.imm.mask instruction
// ======================================
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

// ======================================
// Helper macro for common tensor copy setup
// ======================================
#define SETUP_TENSOR_COPY(rs1_reg, rs3_reg)                                    \
  int rs1 = s->src1.imm;                                                       \
  int rs3 = s->src3.imm;                                                       \
                                                                               \
  void *src = (void *)cpu.gpr[rs1_reg]._64;                                    \
  void *dst = (void *)cpu.gpr[rs3_reg]._64;                                    \
                                                                               \
  uint32_t tcfgrs1 = s->isa.instr.tensor_load_store_64.tcfgrs1;                \
  uint32_t tcfgrs3 = s->isa.instr.tensor_load_store_64.tcfgrs3;                \
                                                                               \
  TensorConfigRegister *TCR1 = &TCR[tcfgrs1];                                  \
  TensorConfigRegister *TCR3 = &TCR[tcfgrs3];                                  \
                                                                               \
  /* Get source dimensions and strides */                                      \
  TensorDims src_dims = create_tensor_dims(                                    \
      TCR1->info.dim_stride_info.dim0, TCR1->info.dim_stride_info.dim1,        \
      TCR1->info.dim_stride_info.dim2, TCR1->info.dim_stride_info.dim3);       \
                                                                               \
  TensorStrides src_strides = create_tensor_strides(                           \
      TCR1->info.dim_stride_info.stride0, TCR1->info.dim_stride_info.stride1,  \
      TCR1->info.dim_stride_info.stride2, TCR1->info.dim_stride_info.stride3); \
                                                                               \
  /* Get destination dimensions and strides */                                 \
  TensorDims dst_dims = create_tensor_dims(                                    \
      TCR3->info.dim_stride_info.dim0, TCR3->info.dim_stride_info.dim1,        \
      TCR3->info.dim_stride_info.dim2, TCR3->info.dim_stride_info.dim3);       \
                                                                               \
  TensorStrides dst_strides = create_tensor_strides(                           \
      TCR3->info.dim_stride_info.stride0, TCR3->info.dim_stride_info.stride1,  \
      TCR3->info.dim_stride_info.stride2, TCR3->info.dim_stride_info.stride3); \
                                                                               \
  /* Get copy mode (log2 of element size) */                                   \
  int mode = s->isa.instr.tensor_load_store_64.log2w;                          \
                                                                               \
  /* Validate total number of elements */                                      \
  uint64_t src_total =                                                         \
      (uint64_t)src_dims.dim0 * src_dims.dim1 * src_dims.dim2 * src_dims.dim3; \
  uint64_t dst_total =                                                         \
      (uint64_t)dst_dims.dim0 * dst_dims.dim1 * dst_dims.dim2 * dst_dims.dim3; \
  assert(src_total == dst_total &&                                             \
         "Source and destination tensor size mismatch");

// ======================================
// tx.async.uram.copy instruction
// ======================================
def_EHelper(tx_async_uram_copy) {
  SETUP_TENSOR_COPY(rs1, rs3);

  uint8_t *uram_src = (uint8_t *)uram_guest_to_host((paddr_t)src);
  uint8_t *uram_dst = (uint8_t *)uram_guest_to_host((paddr_t)dst);

  TensorCopyConfig config = create_tensor_copy_config(
      &src_dims, &src_strides, &dst_dims, &dst_strides, mode);

  tensor_copy(uram_src, uram_dst, &config);
}

// ======================================
// tx.async.uram.load instruction
// ======================================
def_EHelper(tx_async_uram_load) {
  SETUP_TENSOR_COPY(rs1, rs3);

  uint8_t *dram_src = (uint8_t *)guest_to_host((paddr_t)src);
  uint8_t *uram_dst = (uint8_t *)uram_guest_to_host((paddr_t)dst);

  TensorCopyConfig config = create_tensor_copy_config(
      &src_dims, &src_strides, &dst_dims, &dst_strides, mode);

  tensor_copy(dram_src, uram_dst, &config);
}

// ======================================
// tx.async.uram.store instruction
// ======================================
def_EHelper(tx_async_uram_store) {
  SETUP_TENSOR_COPY(rs1, rs3);

  uint8_t *uram_src = (uint8_t *)uram_guest_to_host((paddr_t)src);
  uint8_t *dram_dst = (uint8_t *)guest_to_host((paddr_t)dst);

  TensorCopyConfig config = create_tensor_copy_config(
      &src_dims, &src_strides, &dst_dims, &dst_strides, mode);

  tensor_copy(uram_src, dram_dst, &config);
}

// ======================================
// tx.sync.uram.copy instruction
// ======================================
def_EHelper(tx_sync_uram_copy) {
  SETUP_TENSOR_COPY(rs1, rs3);

  uint8_t *uram_src = (uint8_t *)uram_guest_to_host((paddr_t)src);
  uint8_t *uram_dst = (uint8_t *)uram_guest_to_host((paddr_t)dst);

  TensorCopyConfig config = create_tensor_copy_config(
      &src_dims, &src_strides, &dst_dims, &dst_strides, mode);

  tensor_copy(uram_src, uram_dst, &config);
}

// ======================================
// tx.sync.uram.load instruction
// ======================================
def_EHelper(tx_sync_uram_load) {
  SETUP_TENSOR_COPY(rs1, rs3);

  uint8_t *dram_src = (uint8_t *)guest_to_host((paddr_t)src);
  uint8_t *uram_dst = (uint8_t *)uram_guest_to_host((paddr_t)dst);

  TensorCopyConfig config = create_tensor_copy_config(
      &src_dims, &src_strides, &dst_dims, &dst_strides, mode);

  tensor_copy(dram_src, uram_dst, &config);
}

// ======================================
// tx.sync.uram.store instruction
// ======================================
def_EHelper(tx_sync_uram_store) {
  SETUP_TENSOR_COPY(rs1, rs3);

  uint8_t *uram_src = (uint8_t *)uram_guest_to_host((paddr_t)src);
  uint8_t *dram_dst = (uint8_t *)guest_to_host((paddr_t)dst);

  TensorCopyConfig config = create_tensor_copy_config(
      &src_dims, &src_strides, &dst_dims, &dst_strides, mode);

  tensor_copy(uram_src, dram_dst, &config);
}

// ======================================
// tx.int instruction
// ======================================
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

// ======================================
// tx.fp instruction
// ======================================
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

// ======================================
// tx.lut.int instruction
// ======================================
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

// ======================================
// tx.lut.fp instruction
// ======================================
// TODO
def_EHelper(tx_lut_fp) {}

// ======================================
// tx.int.reduce instruction
// ======================================
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

// ======================================
// tx.fp.reduce instruction
// ======================================
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

// ======================================
// tx.convert instruction
// ======================================
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

// ======================================
// tx.sfu instruction
// ======================================
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

#endif /* RISCV_CUSTOM_TENSOR_EXEC_H */

#endif /* CONFIG_CUSTOM_TENSOR */
