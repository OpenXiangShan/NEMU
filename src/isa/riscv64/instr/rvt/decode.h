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

#include "register.h"

/*
  The decoding process of tensor compute instructions.
*/
static inline def_DHelper(tx_matmul_float) {
  decode_op_i(s, id_src1, s->isa.instr.tensor_compute_64.rs1, true);
  decode_op_i(s, id_src2, s->isa.instr.tensor_compute_64.rs2, true);
  decode_op_i(s, id_src3, s->isa.instr.tensor_compute_64.rs3, true);
}

static inline def_DHelper(tx_matmul_subfloat) {
  decode_op_i(s, id_src1, s->isa.instr.tensor_compute_64.rs1, true);
  decode_op_i(s, id_src2, s->isa.instr.tensor_compute_64.rs2, true);
  decode_op_i(s, id_src3, s->isa.instr.tensor_compute_64.rs3, true);
}

static inline def_DHelper(tx_matmul_fix) {
  decode_op_i(s, id_src1, s->isa.instr.tensor_compute_64.rs1, true);
  decode_op_i(s, id_src2, s->isa.instr.tensor_compute_64.rs2, true);
  decode_op_i(s, id_src3, s->isa.instr.tensor_compute_64.rs3, true);
}

static inline def_DHelper(tx_conv) {
  decode_op_i(s, id_src1, s->isa.instr.tensor_compute_64.rs1, true);
  decode_op_i(s, id_src2, s->isa.instr.tensor_compute_64.rs2, true);
  decode_op_i(s, id_src3, s->isa.instr.tensor_compute_64.rs3, true);
}

static inline def_DHelper(tx_pooling) {
  decode_op_i(s, id_src1, s->isa.instr.tensor_compute_64.rs1, true);
  decode_op_i(s, id_src3, s->isa.instr.tensor_compute_64.rs3, true);
}

/*
  The decoding process of tensor configuration instructions.
*/
static inline def_DHelper(tx_tcsr_xchg_nmask) {
  decode_op_i(s, id_src1, s->isa.instr.tensor_cfg_xchg_64.rs1, true);
  decode_op_i(s, id_src3, s->isa.instr.tensor_cfg_xchg_64.rs3, true);
  decode_op_i(s, id_dest, s->isa.instr.tensor_cfg_xchg_64.rd, true);
}

static inline def_DHelper(tx_tcsr_xchg_mask) {
  decode_op_i(s, id_src1, s->isa.instr.tensor_cfg_xchg_64.rs1, true);
  decode_op_i(s, id_src3, s->isa.instr.tensor_cfg_xchg_64.rs3, true);
  decode_op_i(s, id_dest, s->isa.instr.tensor_cfg_xchg_64.rd, true);
}

static inline def_DHelper(tx_tcsr_imm_nmask) {
  decode_op_i(s, id_src3, s->isa.instr.tensor_cfg_imm_64.rs3, true);
  decode_op_i(s, id_dest, s->isa.instr.tensor_cfg_imm_64.rd, true);
}

static inline def_DHelper(tx_tcsr_imm_mask) {
  decode_op_i(s, id_src3, s->isa.instr.tensor_cfg_imm_64.rs3, true);
  decode_op_i(s, id_dest, s->isa.instr.tensor_cfg_imm_64.rd, true);
}

/*
  The decoding process of tensor load/store instructions.
*/
static inline def_DHelper(tx_async_uram_copy) {
  decode_op_i(s, id_src1, s->isa.instr.tensor_load_store_64.rs1, true);
  decode_op_i(s, id_src3, s->isa.instr.tensor_load_store_64.rs3, true);
}
static inline def_DHelper(tx_async_uram_load) {
  decode_op_i(s, id_src1, s->isa.instr.tensor_load_store_64.rs1, true);
  decode_op_i(s, id_src3, s->isa.instr.tensor_load_store_64.rs3, true);
}
static inline def_DHelper(tx_async_uram_store) {
  decode_op_i(s, id_src1, s->isa.instr.tensor_load_store_64.rs1, true);
  decode_op_i(s, id_src3, s->isa.instr.tensor_load_store_64.rs3, true);
}

static inline def_DHelper(tx_sync_uram_copy) {
  decode_op_i(s, id_src1, s->isa.instr.tensor_load_store_64.rs1, true);
  decode_op_i(s, id_src3, s->isa.instr.tensor_load_store_64.rs3, true);
}
static inline def_DHelper(tx_sync_uram_load) {
  decode_op_i(s, id_src1, s->isa.instr.tensor_load_store_64.rs1, true);
  decode_op_i(s, id_src3, s->isa.instr.tensor_load_store_64.rs3, true);
}
static inline def_DHelper(tx_sync_uram_store) {
  decode_op_i(s, id_src1, s->isa.instr.tensor_load_store_64.rs1, true);
  decode_op_i(s, id_src3, s->isa.instr.tensor_load_store_64.rs3, true);
}

/*
  The decoding process of tensor element-wise compute  instructions.
*/
static inline def_DHelper(tx_int) {
  decode_op_i(s, id_src1, s->isa.instr.tensor_element_wise_compute_64.rs1, true);
  decode_op_i(s, id_src2, s->isa.instr.tensor_element_wise_compute_64.rs2, true);
  decode_op_i(s, id_src3, s->isa.instr.tensor_element_wise_compute_64.rs3, true);
}

static inline def_DHelper(tx_fp) {
  decode_op_i(s, id_src1, s->isa.instr.tensor_element_wise_compute_64.rs1, true);
  decode_op_i(s, id_src2, s->isa.instr.tensor_element_wise_compute_64.rs2, true);
  decode_op_i(s, id_src3, s->isa.instr.tensor_element_wise_compute_64.rs3, true);
}

static inline def_DHelper(tx_lut_int) {
  decode_op_i(s, id_src1, s->isa.instr.tensor_element_wise_compute_64.rs1, true);
  decode_op_i(s, id_src2, s->isa.instr.tensor_element_wise_compute_64.rs2, true);
  decode_op_i(s, id_src3, s->isa.instr.tensor_element_wise_compute_64.rs3, true);
}

static inline def_DHelper(tx_lut_fp) {
  decode_op_i(s, id_src1, s->isa.instr.tensor_element_wise_compute_64.rs1, true);
  decode_op_i(s, id_src2, s->isa.instr.tensor_element_wise_compute_64.rs2, true);
  decode_op_i(s, id_src3, s->isa.instr.tensor_element_wise_compute_64.rs3, true);
}

static inline def_DHelper(tx_int_reduce) {
  decode_op_i(s, id_src1, s->isa.instr.tensor_element_wise_compute_64.rs1, true);
  decode_op_i(s, id_src3, s->isa.instr.tensor_element_wise_compute_64.rs3, true);
}

static inline def_DHelper(tx_fp_reduce) {
  decode_op_i(s, id_src1, s->isa.instr.tensor_element_wise_compute_64.rs1, true);
  decode_op_i(s, id_src3, s->isa.instr.tensor_element_wise_compute_64.rs3, true);
}

static inline def_DHelper(tx_sfu) {
  decode_op_i(s, id_src1, s->isa.instr.tensor_sfu_64.rs1, true);
  decode_op_i(s, id_src3, s->isa.instr.tensor_sfu_64.rs3, true);
}

/*
  The decoding process of tensor convert instructions.
*/
static inline def_DHelper(tx_convert) {
  decode_op_i(s, id_src1, s->isa.instr.tensor_convert_64.rs1, true);
  decode_op_i(s, id_src3, s->isa.instr.tensor_convert_64.rs3, true);
}

#endif
