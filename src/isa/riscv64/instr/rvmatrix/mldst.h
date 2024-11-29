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

// TODO: only support group_size=1 now
// TODO: not consider mstart now
void mld(bool is_trans, char m_name) {
  uint64_t base_addr = s->src1.val;
  int64_t row_byte_stride = s->src2.val;
  uint64_t td = s->dest.reg;
  int rmax_mreg, cmax_mreg, rmax_mem, cmax_mem;
  switch (m_name) {
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
  }
  rmax_mem = is_trans ? cmax_mreg : rmax_mreg;
  cmax_mem = is_trans ? rmax_mreg : cmax_mreg;

  Assert((rmax_mreg <= MRNUM) && (cmax_mreg <= MRENUM8/(s->m_width)), "mtile config should not larger than tile_reg size!\n");
  Assert(s->m_groupsize == 1, "TODO: only support groupsize=1 now!\n");

  uint64_t addr = base_addr;
  for (int row = 0; row < rmax_mem; row++) {
    for (int idx = 0; idx < cmax_mem; idx++) {
      addr = base_addr + idx * (s->m_width);
      rtl_lm(s, &tmp_reg[0], &addr, 0, s->m_width, MMU_DIRECT);
      int row_tr = is_trans ? idx : row;
      int idx_tr = is_trans ? row : idx;
      set_mtreg(td, row_tr, idx_tr, tmp_reg[0], s->m_eew);
    }
    base_addr += row_byte_stride;
  }
}

void mst(bool is_trans, char m_name) {
  uint64_t base_addr = s->src1.val;
  int64_t row_byte_stride = s->src2.val;
  uint64_t ts3 = s->dest.reg;
  int rmax_mreg, cmax_mreg, rmax_mem, cmax_mem;
  switch (m_name) {
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
  }
  rmax_mem = is_trans ? cmax_mreg : rmax_mreg;
  cmax_mem = is_trans ? rmax_mreg : cmax_mreg;

  Assert((rmax_mreg <= MRNUM) && (cmax_mreg <= MRENUM8/(s->m_width)), "mtile config should not larger than tile_reg size!\n");
  Assert(s->m_groupsize == 1, "TODO: only support groupsize=1 now!\n");
  
  uint64_t addr = base_addr;
  for (int row = 0; row < rmax_mem; row++) {
    for (int idx = 0; idx < cmax_mem; idx++) {
      int row_tr = is_trans ? idx : row;
      int idx_tr = is_trans ? row : idx;
      get_mtreg(ts3, row_tr, idx_tr, &tmp_reg[0], s->m_eew);
      addr = base_addr + idx * (s->m_width);
      rtl_sm(s, &tmp_reg[0], &addr, 0, s->m_width, MMU_DIRECT);
    }
    base_addr += row_byte_stride;
  }
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

#endif // CONFIG_RVMATRIX