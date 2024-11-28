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

void set_mtreg(int mtr_num, int mtr_row, int mtr_idx, rtlreg_t src_data, uint64_t msew) {
  Assert(msew <= 3, "msew >= 4 is reserved\n");
  // TODO: optimize for speed here
  switch (msew) {
    case 0 : src_data = src_data & 0xff; break;
    case 1 : src_data = src_data & 0xffff; break;
    case 2 : src_data = src_data & 0xffffffff; break;
    case 3 : src_data = src_data & 0xffffffffffffffff; break;
  }
  switch (msew) {
    case 0 : mtreg_l8(mtr_num, mtr_row, mtr_idx)  = (uint8_t  )src_data; break;
    case 1 : mtreg_l16(mtr_num, mtr_row, mtr_idx) = (uint16_t )src_data; break;
    case 2 : mtreg_l32(mtr_num, mtr_row, mtr_idx) = (uint32_t )src_data; break;
    case 3 : mtreg_l64(mtr_num, mtr_row, mtr_idx) = (uint64_t )src_data; break;
  }
}

void get_mtreg(int mtr_num, int mtr_row, int mtr_idx, rtlreg_t *dst, uint64_t msew) {
  Assert(msew <= 3, "msew >= 4 is reserved\n");
  // TODO: optimize for speed here
  switch (msew) {
    case 0 : *dst = mtreg_l8(mtr_num, mtr_row, mtr_idx) ; break;
    case 1 : *dst = mtreg_l16(mtr_num, mtr_row, mtr_idx); break;
    case 2 : *dst = mtreg_l32(mtr_num, mtr_row, mtr_idx); break;
    case 3 : *dst = mtreg_l64(mtr_num, mtr_row, mtr_idx); break;
  }
}

// TODO: only support group_size=1 now
// TODO: not consider mstart now
def_EHelper(mla) {
  uint64_t base_addr = s->src1.val;
  int64_t row_byte_stride = s->src2.val;
  uint64_t td = s->dest.reg;
  uint64_t tilem = mtilem->val;
  uint64_t tilek = mtilek->val;

  Assert((tilem <= MRNUM) && (tilek <= MRENUM8/(s->m_width)), "mtile config should not larger than tile_reg size!\n");
  Assert(s->m_groupsize == 1, "TODO: only support groupsize=1 now!\n");
  
  uint64_t addr = base_addr;
  for (int row = 0; row < tilem; row++) {
    for (int idx = 0; idx < tilek; idx++) {
      addr = base_addr + idx * (s->m_width);
      rtl_lm(s, &tmp_reg[0], &addr, 0, s->m_width, MMU_DIRECT);
      set_mtreg(td, row, idx, tmp_reg[0], s->m_eew);
    }
    base_addr += row_byte_stride;
  }
}

def_EHelper(mlb) {
  uint64_t base_addr = s->src1.val;
  int64_t row_byte_stride = s->src2.val;
  uint64_t td = s->dest.reg;
  uint64_t tilek = mtilek->val;
  uint64_t tilen = mtilen->val;

  Assert((tilek <= MRNUM) && (tilen <= MRENUM8/(s->m_width)), "mtile config should not larger than tile_reg size!\n");
  Assert(s->m_groupsize == 1, "TODO: only support groupsize=1 now!\n");
  
  uint64_t addr = base_addr;
  for (int row = 0; row < tilek; row++) {
    for (int idx = 0; idx < tilen; idx++) {
      addr = base_addr + idx * (s->m_width);
      rtl_lm(s, &tmp_reg[0], &addr, 0, s->m_width, MMU_DIRECT);
      set_mtreg(td, row, idx, tmp_reg[0], s->m_eew);
    }
    base_addr += row_byte_stride;
  }
}

def_EHelper(mlc) {
  uint64_t base_addr = s->src1.val;
  int64_t row_byte_stride = s->src2.val;
  uint64_t td = s->dest.reg;
  uint64_t tilem = mtilem->val;
  uint64_t tilen = mtilen->val;

  Assert((tilem <= MRNUM) && (tilen <= MRENUM8/(s->m_width)), "mtile config should not larger than tile_reg size!\n");
  Assert(s->m_groupsize == 1, "TODO: only support groupsize=1 now!\n");
  
  uint64_t addr = base_addr;
  for (int row = 0; row < tilem; row++) {
    for (int idx = 0; idx < tilen; idx++) {
      addr = base_addr + idx * (s->m_width);
      rtl_lm(s, &tmp_reg[0], &addr, 0, s->m_width, MMU_DIRECT);
      set_mtreg(td, row, idx, tmp_reg[0], s->m_eew);
    }
    base_addr += row_byte_stride;
  }
}

def_EHelper(mlat) {
  uint64_t base_addr = s->src1.val;
  int64_t row_byte_stride = s->src2.val;
  uint64_t td = s->dest.reg;
  uint64_t tilem = mtilem->val;
  uint64_t tilek = mtilek->val;

  Assert((tilem <= MRNUM) && (tilek <= MRENUM8/(s->m_width)), "mtile config should not larger than tile_reg size!\n");
  Assert(s->m_groupsize == 1, "TODO: only support groupsize=1 now!\n");
  
  uint64_t addr = base_addr;
  for (int row_mem = 0; row_mem < tilek; row_mem++) {
    for (int idx_mem = 0; idx_mem < tilem; idx_mem++) {
      addr = base_addr + idx_mem * (s->m_width);
      rtl_lm(s, &tmp_reg[0], &addr, 0, s->m_width, MMU_DIRECT);
      set_mtreg(td, idx_mem, row_mem, tmp_reg[0], s->m_eew);
    }
    base_addr += row_byte_stride;
  }
}

def_EHelper(mlbt) {
  uint64_t base_addr = s->src1.val;
  int64_t row_byte_stride = s->src2.val;
  uint64_t td = s->dest.reg;
  uint64_t tilek = mtilek->val;
  uint64_t tilen = mtilen->val;

  Assert((tilek <= MRNUM) && (tilen <= MRENUM8/(s->m_width)), "mtile config should not larger than tile_reg size!\n");
  Assert(s->m_groupsize == 1, "TODO: only support groupsize=1 now!\n");
  
  uint64_t addr = base_addr;
  for (int row_mem = 0; row_mem < tilen; row_mem++) {
    for (int idx_mem = 0; idx_mem < tilek; idx_mem++) {
      addr = base_addr + idx_mem * (s->m_width);
      rtl_lm(s, &tmp_reg[0], &addr, 0, s->m_width, MMU_DIRECT);
      set_mtreg(td, idx_mem, row_mem, tmp_reg[0], s->m_eew);
    }
    base_addr += row_byte_stride;
  }
}

def_EHelper(mlct) {
  uint64_t base_addr = s->src1.val;
  int64_t row_byte_stride = s->src2.val;
  uint64_t td = s->dest.reg;
  uint64_t tilem = mtilem->val;
  uint64_t tilen = mtilen->val;

  Assert((tilem <= MRNUM) && (tilen <= MRENUM8/(s->m_width)), "mtile config should not larger than tile_reg size!\n");
  Assert(s->m_groupsize == 1, "TODO: only support groupsize=1 now!\n");
  
  uint64_t addr = base_addr;
  for (int row_mem = 0; row_mem < tilen; row_mem++) {
    for (int idx_mem = 0; idx_mem < tilem; idx_mem++) {
      addr = base_addr + idx_mem * (s->m_width);
      rtl_lm(s, &tmp_reg[0], &addr, 0, s->m_width, MMU_DIRECT);
      set_mtreg(td, idx_mem, row_mem, tmp_reg[0], s->m_eew);
    }
    base_addr += row_byte_stride;
  }
}

def_EHelper(msa) {
  uint64_t base_addr = s->src1.val;
  int64_t row_byte_stride = s->src2.val;
  uint64_t ts3 = s->dest.reg;
  uint64_t tilem = mtilem->val;
  uint64_t tilek = mtilek->val;
  
  uint64_t addr = base_addr;
  for (int row = 0; row < tilem; row++) {
    for (int idx = 0; idx < tilek; idx++) {
      get_mtreg(ts3, row, idx, &tmp_reg[0], s->m_eew);
      addr = base_addr + idx * (s->m_width);
      rtl_sm(s, &tmp_reg[0], &addr, 0, s->m_width, MMU_DIRECT);
    }
    base_addr += row_byte_stride;
  }
}

def_EHelper(msb) {
  uint64_t base_addr = s->src1.val;
  int64_t row_byte_stride = s->src2.val;
  uint64_t ts3 = s->dest.reg;
  uint64_t tilek = mtilek->val;
  uint64_t tilen = mtilen->val;
  
  uint64_t addr = base_addr;
  for (int row = 0; row < tilek; row++) {
    for (int idx = 0; idx < tilen; idx++) {
      get_mtreg(ts3, row, idx, &tmp_reg[0], s->m_eew);
      addr = base_addr + idx * (s->m_width);
      rtl_sm(s, &tmp_reg[0], &addr, 0, s->m_width, MMU_DIRECT);
    }
    base_addr += row_byte_stride;
  }
}

def_EHelper(msc) {
  uint64_t base_addr = s->src1.val;
  int64_t row_byte_stride = s->src2.val;
  uint64_t ts3 = s->dest.reg;
  uint64_t tilem = mtilem->val;
  uint64_t tilen = mtilen->val;
  
  uint64_t addr = base_addr;
  for (int row = 0; row < tilem; row++) {
    for (int idx = 0; idx < tilen; idx++) {
      get_mtreg(ts3, row, idx, &tmp_reg[0], s->m_eew);
      addr = base_addr + idx * (s->m_width);
      rtl_sm(s, &tmp_reg[0], &addr, 0, s->m_width, MMU_DIRECT);
    }
    base_addr += row_byte_stride;
  }
}

def_EHelper(msat) {
  uint64_t base_addr = s->src1.val;
  int64_t row_byte_stride = s->src2.val;
  uint64_t ts3 = s->dest.reg;
  uint64_t tilem = mtilem->val;
  uint64_t tilek = mtilek->val;
  
  uint64_t addr = base_addr;
  for (int row = 0; row < tilek; row++) {
    for (int idx = 0; idx < tilem; idx++) {
      get_mtreg(ts3, idx, row, &tmp_reg[0], s->m_eew);
      addr = base_addr + idx * (s->m_width);
      rtl_sm(s, &tmp_reg[0], &addr, 0, s->m_width, MMU_DIRECT);
    }
    base_addr += row_byte_stride;
  }
}

def_EHelper(msbt) {
  uint64_t base_addr = s->src1.val;
  int64_t row_byte_stride = s->src2.val;
  uint64_t ts3 = s->dest.reg;
  uint64_t tilek = mtilek->val;
  uint64_t tilen = mtilen->val;
  
  uint64_t addr = base_addr;
  for (int row = 0; row < tilen; row++) {
    for (int idx = 0; idx < tilek; idx++) {
      get_mtreg(ts3, idx, row, &tmp_reg[0], s->m_eew);
      addr = base_addr + idx * (s->m_width);
      rtl_sm(s, &tmp_reg[0], &addr, 0, s->m_width, MMU_DIRECT);
    }
    base_addr += row_byte_stride;
  }
}

def_EHelper(msct) {
  uint64_t base_addr = s->src1.val;
  int64_t row_byte_stride = s->src2.val;
  uint64_t ts3 = s->dest.reg;
  uint64_t tilem = mtilem->val;
  uint64_t tilen = mtilen->val;
  
  uint64_t addr = base_addr;
  for (int row = 0; row < tilen; row++) {
    for (int idx = 0; idx < tilem; idx++) {
      get_mtreg(ts3, idx, row, &tmp_reg[0], s->m_eew);
      addr = base_addr + idx * (s->m_width);
      rtl_sm(s, &tmp_reg[0], &addr, 0, s->m_width, MMU_DIRECT);
    }
    base_addr += row_byte_stride;
  }
}

#endif // CONFIG_RVMATRIX