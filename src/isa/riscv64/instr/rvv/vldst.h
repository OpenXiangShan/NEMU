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


def_EHelper(vle) { //unit-strided
  require_vector(true);
  VLD(MODE_UNIT, UNSIGNED, s, MMU_DIRECT)
}

def_EHelper(vlm) { //mask
  require_vector(true);
  VLD(MODE_MASK, UNSIGNED, s, MMU_DIRECT)
}

def_EHelper(vlr) { // whole register
  require_vector(true);
  VLR(MODE_UNIT, UNSIGNED, s, MMU_DIRECT)
}

def_EHelper(vlse) { //strided unsigned
  require_vector(true);
  s->src2.reg = s->isa.instr.fp.rs2;
  rtl_lr(s, &(s->src2.val), s->src2.reg, 4);
  VLD(MODE_STRIDED, UNSIGNED, s, MMU_DIRECT)
}

def_EHelper(vlxe) {
  require_vector(true);
  s->src2.reg = s->isa.instr.fp.rs2;
  rtl_lr(s, &(s->src2.val), s->src2.reg, 4);
  VLDX(MODE_INDEXED, UNSIGNED, s, MMU_DIRECT)
}

def_EHelper(vse) {
  require_vector(true);
  VST(MODE_UNIT, MMU_DIRECT)
}

def_EHelper(vsm) {
  require_vector(true);
  VST(MODE_MASK, MMU_DIRECT)
}

def_EHelper(vsr) {
  require_vector(true);
  VSR(MODE_UNIT, MMU_DIRECT)
}

def_EHelper(vsse) {
  require_vector(true);
  s->src2.reg = s->isa.instr.fp.rs2;
  rtl_lr(s, &(s->src2.val), s->src2.reg, 4);
  VST(MODE_STRIDED, MMU_DIRECT)
}

def_EHelper(vsxe) {
  require_vector(true);
  s->src2.reg = s->isa.instr.fp.rs2;
  rtl_lr(s, &(s->src2.val), s->src2.reg, 4);
  VSTX(MODE_INDEXED, MMU_DIRECT)
}

def_EHelper(vle_mmu) { //unit-strided
  require_vector(true);
  VLD(MODE_UNIT, UNSIGNED, s, MMU_TRANSLATE)
}

def_EHelper(vlm_mmu) { //mask
  require_vector(true);
  VLD(MODE_MASK, UNSIGNED, s, MMU_TRANSLATE)
}

def_EHelper(vlr_mmu) { //whple register
  require_vector(true);
  VLR(MODE_UNIT, UNSIGNED, s, MMU_TRANSLATE)
}

def_EHelper(vlse_mmu) { //strided unsigned
  require_vector(true);
  s->src2.reg = s->isa.instr.fp.rs2;
  rtl_lr(s, &(s->src2.val), s->src2.reg, 4);
  VLD(MODE_STRIDED, UNSIGNED, s, MMU_TRANSLATE)
}

def_EHelper(vlxe_mmu) {
  require_vector(true);
  s->src2.reg = s->isa.instr.fp.rs2;
  rtl_lr(s, &(s->src2.val), s->src2.reg, 4);
  VLDX(MODE_INDEXED, UNSIGNED, s, MMU_TRANSLATE)
}

def_EHelper(vse_mmu) {
  require_vector(true);
  VST(MODE_UNIT, MMU_TRANSLATE)
}

def_EHelper(vsm_mmu) {
  require_vector(true);
  VST(MODE_MASK, MMU_TRANSLATE)
}

def_EHelper(vsr_mmu) {
  require_vector(true);
  VSR(MODE_UNIT, MMU_TRANSLATE)
}

def_EHelper(vsse_mmu) {
  require_vector(true);
  VST(MODE_STRIDED, MMU_TRANSLATE)
}

def_EHelper(vsxe_mmu) {
  require_vector(true);
  s->src2.reg = s->isa.instr.fp.rs2;
  rtl_lr(s, &(s->src2.val), s->src2.reg, 4);
  VSTX(MODE_INDEXED, MMU_TRANSLATE)
}

#endif // CONFIG_RVV