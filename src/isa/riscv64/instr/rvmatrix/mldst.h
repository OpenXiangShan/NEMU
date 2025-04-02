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

#ifdef CONFIG_RVMATRIX

#include "cpu/exec.h"
#include "mreg.h"
#include "../local-include/csr.h"
#include <stdio.h>
#include "../local-include/intr.h"
#include "../local-include/rtl.h"
#include "../local-include/reg.h"

// TODO: not consider mstart now
void mld(bool is_trans, char m_name) {
  uint64_t base_addr = reg_l(s->src1.reg);
  int64_t row_byte_stride = reg_l(s->src2.reg);
  uint64_t td = s->dest.reg;
  int rmax_mreg, cmax_mreg, rmax_mem, cmax_mem;
  /*switch (m_name) {
    case 'a':
      rmax_mreg  = mtilem->val;
      cmax_mreg  = mtilek->val;
      break;
    case 'b':
      rmax_mreg  = mtilek->val;
      cmax_mreg  = mtilen->val;
      break;
    case 'c':
      rmax_mreg  = mtilem->val;
      cmax_mreg  = mtilen->val;
      break;
    default:
      break;
  }*/
  int lmul = s->m_groupsize;
  switch (m_name) {
    case 'a':
      rmax_mreg  = is_trans ? (mtilem->val)/lmul : mtilem->val;
      cmax_mreg  = is_trans ? mtilek->val : (mtilek->val)/lmul;
      break;
    case 'b':
      rmax_mreg  = is_trans ? (mtilek->val)/lmul : mtilek->val;
      cmax_mreg  = is_trans ? mtilen->val : (mtilen->val)/lmul;
      break;
    case 'c':
      rmax_mreg  = is_trans ? (mtilem->val)/lmul : mtilem->val;
      cmax_mreg  = is_trans ? mtilen->val : (mtilen->val)/lmul;
      break;
    default:
      break;
  }
  rmax_mem = is_trans ? cmax_mreg : rmax_mreg;
  cmax_mem = is_trans ? rmax_mreg : cmax_mreg;

  Assert((rmax_mreg <= MRNUM) && (cmax_mreg <= MRENUM8/(s->m_width)), "mtile config should not larger than tile_reg size!\n");
  //Assert(s->m_groupsize == 1, "Only support groupsize=1 now!\n");

  uint64_t addr = base_addr;
  /*for (int row = 0; row < rmax_mem; row++) {
    for (int idx = 0; idx < cmax_mem; idx++) {
      addr = base_addr + idx * (s->m_width);
      rtl_lm(s, &tmp_reg[0], &addr, 0, s->m_width, MMU_TRANSLATE);
      int row_tr = is_trans ? idx : row;
      int idx_tr = is_trans ? row : idx;
      set_mtreg(td, row_tr, idx_tr, tmp_reg[0], s->m_eew);
    }
    base_addr += row_byte_stride;
  }*/
  for (int row = 0; row < rmax_mem; row++) {
    for (int m = 0; m < lmul; m++) {
      for (int idx = 0; idx < cmax_mem; idx++) {
        addr = base_addr + row * row_byte_stride + m * cmax_mem * (s->m_width) + idx * (s->m_width);
        rtl_lmm(s, &tmp_reg[0], &addr, 0, s->m_width, MMU_TRANSLATE);
        int row_tr = is_trans ? idx : row;
        int idx_tr = is_trans ? row : idx;
        set_mtreg(td+m, row_tr, idx_tr, tmp_reg[0], s->m_eew);
      }
    }
  }
  fprintf(stderr, "!!!! mld %c: base_addr=%lx, rmax_mem=%d, lmul=%d, cmax_mem=%d, row_byte_stride=%ld\n",
    m_name, base_addr, rmax_mem, lmul, cmax_mem, row_byte_stride);
}

void mst(bool is_trans, char m_name) {
  uint64_t base_addr = reg_l(s->src1.reg);
  int64_t row_byte_stride = reg_l(s->src2.reg);
  uint64_t ts3 = s->dest.reg;
  int rmax_mreg, cmax_mreg, rmax_mem, cmax_mem;
  /*switch (m_name) {
    case 'a':
      rmax_mreg  = mtilem->val;
      cmax_mreg  = mtilek->val;
      break;
    case 'b':
      rmax_mreg  = mtilek->val;
      cmax_mreg  = mtilen->val;
      break;
    case 'c':
      rmax_mreg  = mtilem->val;
      cmax_mreg  = mtilen->val;
      break;
    default:
      break;
  }*/
  int lmul = s->m_groupsize;
  switch (m_name) {
    case 'a':
      rmax_mreg  = is_trans ? (mtilem->val)/lmul : mtilem->val;
      cmax_mreg  = is_trans ? mtilek->val : (mtilek->val)/lmul;
      break;
    case 'b':
      rmax_mreg  = is_trans ? (mtilek->val)/lmul : mtilek->val;
      cmax_mreg  = is_trans ? mtilen->val : (mtilen->val)/lmul;
      break;
    case 'c':
      rmax_mreg  = is_trans ? (mtilem->val)/lmul : mtilem->val;
      cmax_mreg  = is_trans ? mtilen->val : (mtilen->val)/lmul;
      break;
    default:
      break;
  }
  rmax_mem = is_trans ? cmax_mreg : rmax_mreg;
  cmax_mem = is_trans ? rmax_mreg : cmax_mreg;

  Assert((rmax_mreg <= MRNUM) && (cmax_mreg <= MRENUM8/(s->m_width)), "mtile config should not larger than tile_reg size!\n");
  //Assert(s->m_groupsize == 1, "Only support groupsize=1 now!\n");
  
  uint64_t addr = base_addr;
  /*for (int row = 0; row < rmax_mem; row++) {
    for (int idx = 0; idx < cmax_mem; idx++) {
      int row_tr = is_trans ? idx : row;
      int idx_tr = is_trans ? row : idx;
      get_mtreg(ts3, row_tr, idx_tr, &tmp_reg[0], s->m_eew, false);
      addr = base_addr + idx * (s->m_width);
      rtl_sm(s, &tmp_reg[0], &addr, 0, s->m_width, MMU_TRANSLATE);
    }
    base_addr += row_byte_stride;
  }*/
  for (int row = 0; row < rmax_mem; row++) {
    for (int m = 0; m < lmul; m++) {
      for (int idx = 0; idx < cmax_mem; idx++) {
        int row_tr = is_trans ? idx : row;
        int idx_tr = is_trans ? row : idx;
        get_mtreg(ts3+m, row_tr, idx_tr, &tmp_reg[0], s->m_eew, false);
        addr = base_addr + row * row_byte_stride + m * cmax_mem * (s->m_width) + idx * (s->m_width);
        rtl_smm(s, &tmp_reg[0], &addr, 0, s->m_width, MMU_TRANSLATE);
      }
    }
  }
  fprintf(stderr, "!!!! mst: base_addr=%lx, rmax_mem=%d, lmul=%d, cmax_mem=%d, row_byte_stride=%ld\n",
    base_addr, rmax_mem, lmul, cmax_mem, row_byte_stride);
}


def_EHelper(mla) {
  mld(false, 'a');
}

def_EHelper(mlb) {
  mld(false, 'b');
}

def_EHelper(mlc) {
  mld(false, 'c');
}

def_EHelper(mlat) {
  mld(true, 'a');
}

def_EHelper(mlbt) {
  mld(true, 'b');
}

def_EHelper(mlct) {
  mld(true, 'c');
}

def_EHelper(msa) {
  mst(false, 'a');
}

def_EHelper(msb) {
  mst(false, 'b');
}

def_EHelper(msc) {
  mst(false, 'c');
}

def_EHelper(msat) {
  mst(true, 'a');
}

def_EHelper(msbt) {
  mst(true, 'b');
}

def_EHelper(msct) {
  mst(true, 'c');
}

def_EHelper(mlr) {
}
def_EHelper(msr) {
}

#endif // CONFIG_RVMATRIX