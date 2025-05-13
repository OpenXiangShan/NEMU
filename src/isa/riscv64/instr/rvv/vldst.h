/***************************************************************************************
* Copyright (c) 2020-2022 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <common.h>
#ifdef CONFIG_RVV

#include "vldst_impl.h"
#include "vcompute_impl.h"

// if we decode some information in decode stage
// when running in opt mode, these information will not be generated because
// it only runs the exec functions
void predecode_vls(Decode *s) {
#ifdef CONFIG_RVV
  cpu.isVldst = true;
  const int table [8] = {1, 0, 0, 0, 0, 2, 4, 8};
  s->vm = s->isa.instr.v_opv.v_vm; //1 for without mask; 0 for with mask
  s->v_width = table[s->isa.instr.vldfp.v_width];
  if (s->v_width == 0) {
    // reserved
    longjmp_exception(EX_II);
  }
  s->v_nf = s->isa.instr.vldfp.v_nf;
  s->v_lsumop = s->isa.instr.vldfp.v_lsumop;
#endif
}


def_EHelper(vle) { //unit-strided
  predecode_vls(s);
  require_vector(true);
  vld(s, MODE_UNIT, MMU_DIRECT);
}

def_EHelper(vlm) { //mask
  predecode_vls(s);
  require_vector(true);
  vld(s, MODE_MASK, MMU_DIRECT);
}

def_EHelper(vlr) { // whole register
  predecode_vls(s);
  require_vector(false);
  vlr(s, MMU_DIRECT);
}

def_EHelper(vlse) { //strided unsigned
  predecode_vls(s);
  require_vector(true);
  s->src2.reg = s->isa.instr.fp.rs2;
  rtl_lr(s, &(s->src2.val), s->src2.reg, 4);
  vld(s, MODE_STRIDED, MMU_DIRECT);
}

def_EHelper(vlxe) {
  predecode_vls(s);
  require_vector(true);
  s->src2.reg = s->isa.instr.fp.rs2;
  rtl_lr(s, &(s->src2.val), s->src2.reg, 4);
  vldx(s, MMU_DIRECT);
}

def_EHelper(vleff) {
  predecode_vls(s);
  require_vector(true);
  vldff(s, MODE_UNIT, MMU_DIRECT);
}


def_EHelper(vse) {
  predecode_vls(s);
  require_vector(true);
  vst(s, MODE_UNIT, MMU_DIRECT);
}

def_EHelper(vsm) {
  predecode_vls(s);
  require_vector(true);
  vst(s, MODE_MASK, MMU_DIRECT);
}

def_EHelper(vsr) {
  predecode_vls(s);
  require_vector(false);
  vsr(s, MMU_DIRECT);
}

def_EHelper(vsse) {
  predecode_vls(s);
  require_vector(true);
  s->src2.reg = s->isa.instr.fp.rs2;
  rtl_lr(s, &(s->src2.val), s->src2.reg, 4);
  vst(s, MODE_STRIDED, MMU_DIRECT);
}

def_EHelper(vsxe) {
  predecode_vls(s);
  require_vector(true);
  s->src2.reg = s->isa.instr.fp.rs2;
  rtl_lr(s, &(s->src2.val), s->src2.reg, 4);
  vstx(s, MMU_DIRECT);
}

def_EHelper(vle_mmu) { //unit-strided
  predecode_vls(s);
  require_vector(true);
  vld(s, MODE_UNIT, MMU_TRANSLATE);
}

def_EHelper(vlm_mmu) { //mask
  predecode_vls(s);
  require_vector(true);
  vld(s, MODE_MASK, MMU_TRANSLATE);
}

def_EHelper(vlr_mmu) { //whple register
  predecode_vls(s);
  require_vector(false);
  vlr(s, MMU_TRANSLATE);
}

def_EHelper(vlse_mmu) { //strided unsigned
  predecode_vls(s);
  require_vector(true);
  s->src2.reg = s->isa.instr.fp.rs2;
  rtl_lr(s, &(s->src2.val), s->src2.reg, 4);
  vld(s, MODE_STRIDED, MMU_TRANSLATE);
}

def_EHelper(vlxe_mmu) {
  predecode_vls(s);
  require_vector(true);
  s->src2.reg = s->isa.instr.fp.rs2;
  rtl_lr(s, &(s->src2.val), s->src2.reg, 4);
  vldx(s, MMU_TRANSLATE);
}

def_EHelper(vleff_mmu) {
  predecode_vls(s);
  require_vector(true);
  vldff(s, MODE_UNIT, MMU_TRANSLATE);
}

def_EHelper(vse_mmu) {
  predecode_vls(s);
  require_vector(true);
  vst(s, MODE_UNIT, MMU_TRANSLATE);
}

def_EHelper(vsm_mmu) {
  predecode_vls(s);
  require_vector(true);
  vst(s, MODE_MASK, MMU_TRANSLATE);
}

def_EHelper(vsr_mmu) {
  predecode_vls(s);
  require_vector(false);
  vsr(s, MMU_TRANSLATE);
}

def_EHelper(vsse_mmu) {
  predecode_vls(s);
  require_vector(true);
  s->src2.reg = s->isa.instr.fp.rs2;
  rtl_lr(s, &(s->src2.val), s->src2.reg, 4);
  vst(s, MODE_STRIDED, MMU_TRANSLATE);
}

def_EHelper(vsxe_mmu) {
  predecode_vls(s);
  require_vector(true);
  s->src2.reg = s->isa.instr.fp.rs2;
  rtl_lr(s, &(s->src2.val), s->src2.reg, 4);
  vstx(s, MMU_TRANSLATE);
}

#endif // CONFIG_RVV