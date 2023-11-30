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

// reference: v_ext_macros.h in riscv-isa-sim

void vld(int mode, int is_signed, Decode *s, int mmu_mode) {
  if(check_vstart_ignore(s)) return;
  word_t idx;
  uint64_t nf, fn, vl_val, base_addr, vd, addr;
  int eew, emul, stride, is_stride;

  eew = 0;
  switch(s->v_width) {
    case 1: eew = 0; break;
    case 2: eew = 1; break;
    case 4: eew = 2; break;
    case 8: eew = 3; break;
    default: break;
  }
  emul = vtype->vlmul > 4 ? vtype->vlmul - 8 + eew - vtype->vsew : vtype->vlmul + eew - vtype->vsew;
  emul = emul < 0 ? 0 : emul;
  emul = 1 << emul;

  if (mode == MODE_STRIDED) {
    stride = id_src2->val;
    is_stride = 0;
  } else {
    stride = 0;
    is_stride = 1;
  }
  // previous decode does not load vals for us
  rtl_lr(s, &(s->src1.val), s->src1.reg, 4);
  rtl_mv(s, &(tmp_reg[0]), &(s->src1.val));

  nf = s->v_nf + 1;
  vl_val = mode == MODE_MASK ? (vl->val + 7) / 8 : vl->val;
  base_addr = tmp_reg[0];
  vd = id_dest->reg;
  for (idx = vstart->val; idx < vl_val; idx++) {
    rtlreg_t mask = get_mask(0, idx, vtype->vsew, vtype->vlmul);
    if (s->vm == 0 && mask == 0) {
      if (RVV_AGNOSTIC && vtype->vma) {
        tmp_reg[1] = (uint64_t) -1;
        for (fn = 0; fn < nf; fn++) {
          set_vreg(vd + fn * emul, idx, tmp_reg[1], eew, vtype->vlmul, 1);
        }
      }
      continue;
    }
    for (fn = 0; fn < nf; fn++) {
      addr = base_addr + idx * stride + (idx * nf * is_stride + fn) * s->v_width;
      rtl_lm(s, &tmp_reg[1], &addr, 0, s->v_width, mmu_mode);
      set_vreg(vd + fn * emul, idx, tmp_reg[1], eew, vtype->vlmul, 1);
    }
  }

  if (RVV_AGNOSTIC && vtype->vta) {   // set tail of vector register to 1
    int vlmax = get_vlen_max(eew, vtype->vlmul);
    for(idx = vl->val; idx < vlmax; idx++) {
      tmp_reg[1] = (uint64_t) -1;
      for (fn = 0; fn < nf; fn++) {
        set_vreg(vd + fn * emul, idx, tmp_reg[1], eew, vtype->vlmul, 1);
      }
    }
  }

  vstart->val = 0;
  set_mstatus_dirt();
}

void vldx(int mode, int is_signed, Decode *s, int mmu_mode) {
  //TODO: raise instr when decinfo.v_width > SEW
  //v_width   0  -> none    SEW   0  ->  8
  //        1  ->  8            1  ->  16
  //        2  ->  16           2  ->  32
  //        4  ->  32           3  ->  64
  //        8  ->  64
  if(check_vstart_ignore(s)) return;
  word_t idx;
  uint64_t nf, fn, vl_val, base_addr, vd, index, addr;
  int eew, lmul, index_width;

  index_width = 0;
  eew = vtype->vsew;
  switch(s->v_width) {
    case 1: index_width = 0; break;
    case 2: index_width = 1; break;
    case 4: index_width = 2; break;
    case 8: index_width = 3; break;
    default: break;
  }
  switch (vtype->vsew) {
    case 0: s->v_width = 1; break;
    case 1: s->v_width = 2; break;
    case 2: s->v_width = 4; break;
    case 3: s->v_width = 8; break;
    default: break;
  }
  lmul = vtype->vlmul > 4 ? vtype->vlmul - 8 : vtype->vlmul;
  lmul = lmul < 0 ? 0 : lmul;
  lmul = 1 << lmul;

  // previous decode does not load vals for us
  rtl_lr(s, &(s->src1.val), s->src1.reg, 4);
  rtl_mv(s, &(tmp_reg[0]), &(s->src1.val));

  nf = s->v_nf + 1;
  vl_val = vl->val;
  base_addr = tmp_reg[0];
  vd = id_dest->reg;
  for (idx = vstart->val; idx < vl_val; idx++) {
    rtlreg_t mask = get_mask(0, idx, vtype->vsew, vtype->vlmul);
    if (s->vm == 0 && mask == 0) {
      if (RVV_AGNOSTIC && vtype->vma) {
        tmp_reg[1] = (uint64_t) -1;
        for (fn = 0; fn < nf; fn++) {
          set_vreg(vd + fn * lmul, idx, tmp_reg[1], eew, vtype->vlmul, 1);
        }
      }
      continue;
    }
    for (fn = 0; fn < nf; fn++) {
      // read index
      get_vreg(id_src2->reg, idx, &tmp_reg[2], index_width, vtype->vlmul, 0, 1);
      index = tmp_reg[2];

      // read data in memory
      addr = base_addr + index + fn * s->v_width;
      rtl_lm(s, &tmp_reg[1], &addr, 0, s->v_width, mmu_mode);
      set_vreg(vd + fn * lmul, idx, tmp_reg[1], eew, vtype->vlmul, 1);
    }
  }

  if (RVV_AGNOSTIC && vtype->vta) {   // set tail of vector register to 1
    int vlmax = get_vlen_max(eew, vtype->vlmul);
    for(idx = vl->val; idx < vlmax; idx++) {
      tmp_reg[1] = (uint64_t) -1;
      for (fn = 0; fn < nf; fn++) {
        set_vreg(vd + fn * lmul, idx, tmp_reg[1], eew, vtype->vlmul, 1);
      }
    }
  }

  // TODO: the idx larger than vl need reset to zero.
  vstart->val = 0;
  set_mstatus_dirt();
}

void vst(int mode, Decode *s, int mmu_mode) {
  if(check_vstart_ignore(s)) return;
  word_t idx;
  uint64_t nf, fn, vl_val, base_addr, vd, addr;
  int eew, emul, stride, is_stride;

  eew = 0;
  switch(s->v_width) {
    case 1: eew = 0; break;
    case 2: eew = 1; break;
    case 4: eew = 2; break;
    case 8: eew = 3; break;
    default: break;
  }
  emul = vtype->vlmul > 4 ? vtype->vlmul - 8 + eew - vtype->vsew : vtype->vlmul + eew - vtype->vsew;
  emul = emul < 0 ? 0 : emul;
  emul = 1 << emul;

  if (mode == MODE_STRIDED) {
    stride = id_src2->val;
    is_stride = 0;
  } else {
    stride = 0;
    is_stride = 1;
  }
  // previous decode does not load vals for us
  rtl_lr(s, &(s->src1.val), s->src1.reg, 4);
  rtl_mv(s, &(tmp_reg[0]), &(s->src1.val));

  nf = s->v_nf + 1;
  vl_val = mode == MODE_MASK ? (vl->val + 7) / 8 : vl->val;
  base_addr = tmp_reg[0];
  vd = id_dest->reg;
  for (idx = vstart->val; idx < vl_val; idx++) {
    rtlreg_t mask = get_mask(0, idx, vtype->vsew, vtype->vlmul);
    if (s->vm == 0 && mask == 0) {
      continue;
    }
    for (fn = 0; fn < nf; fn++) {
      get_vreg(vd + fn * emul, idx, &tmp_reg[1], eew, vtype->vlmul, 0, 1);
      addr = base_addr + idx * stride + (idx * nf * is_stride + fn) * s->v_width;
      rtl_sm(s, &tmp_reg[1], &addr, 0, s->v_width, mmu_mode);
    }
  }

  vstart->val = 0;
  set_mstatus_dirt();
}

void vstx(int mode, Decode *s, int mmu_mode) {
  if(check_vstart_ignore(s)) return;
  word_t idx;
  uint64_t nf, fn, vl_val, base_addr, vd, index, addr;
  int eew, lmul, index_width;

  index_width = 0;
  eew = vtype->vsew;
  switch(s->v_width) {
    case 1: index_width = 0; break;
    case 2: index_width = 1; break;
    case 4: index_width = 2; break;
    case 8: index_width = 3; break;
    default: break;
  }
  switch (vtype->vsew) {
    case 0: s->v_width = 1; break;
    case 1: s->v_width = 2; break;
    case 2: s->v_width = 4; break;
    case 3: s->v_width = 8; break;
    default: break;
  }
  lmul = vtype->vlmul > 4 ? vtype->vlmul - 8 : vtype->vlmul;
  lmul = lmul < 0 ? 0 : lmul;
  lmul = 1 << lmul;

  // previous decode does not load vals for us
  rtl_lr(s, &(s->src1.val), s->src1.reg, 4);
  rtl_mv(s, &(tmp_reg[0]), &(s->src1.val));

  nf = s->v_nf + 1;
  vl_val = vl->val;
  base_addr = tmp_reg[0];
  vd = id_dest->reg;
  for (idx = vstart->val; idx < vl_val; idx++) {
    rtlreg_t mask = get_mask(0, idx, vtype->vsew, vtype->vlmul);
    if (s->vm == 0 && mask == 0) {
      continue;
    }
    for (fn = 0; fn < nf; fn++) {
      // read index
      get_vreg(id_src2->reg, idx, &tmp_reg[2], index_width, vtype->vlmul, 0, 1);
      index = tmp_reg[2];

      // read data in vector register
      get_vreg(vd + fn * lmul, idx, &tmp_reg[1], eew, vtype->vlmul, 0, 1);
      addr = base_addr + index + fn * s->v_width;
      rtl_sm(s, &tmp_reg[1], &addr, 0, s->v_width, mmu_mode);
    }
  }

  // TODO: the idx larger than vl need reset to zero.
  vstart->val = 0;
  set_mstatus_dirt();
}

void vlr(int mode, int is_signed, Decode *s, int mmu_mode) {
  word_t idx, vreg_idx, offset, pos;
  uint64_t len, base_addr, vd, addr, elt_per_reg, size;
  int eew;

  eew = 0;
  switch(s->v_width) {
    case 1: eew = 0; break;
    case 2: eew = 1; break;
    case 4: eew = 2; break;
    case 8: eew = 3; break;
    default: break;
  }

  // previous decode does not load vals for us
  rtl_lr(s, &(s->src1.val), s->src1.reg, 4);
  rtl_mv(s, &(tmp_reg[0]), &(s->src1.val));

  len = s->v_nf + 1;
  elt_per_reg = VLEN / (8*s->v_width);
  size = len * elt_per_reg;
  base_addr = tmp_reg[0];
  vd = id_dest->reg;
  idx = vstart->val;

  if (vstart->val < size) {
    vreg_idx = vstart->val / elt_per_reg;
    offset = vstart->val % elt_per_reg;
    if (offset) {
      // first vreg
      for (pos = offset; pos < elt_per_reg; pos++) {
        addr = base_addr + idx * s->v_width;
        rtl_lm(s, &tmp_reg[1], &addr, 0, s->v_width, mmu_mode);
        set_vreg(vd + vreg_idx, pos, tmp_reg[1], eew, 0, 1);
        idx++;
      }
      vreg_idx++;
    }
    for (; vreg_idx < len; vreg_idx++) {
      for (pos = 0; pos < elt_per_reg; pos++) {
        addr = base_addr + idx * s->v_width;
        rtl_lm(s, &tmp_reg[1], &addr, 0, s->v_width, mmu_mode);
        set_vreg(vd + vreg_idx, pos, tmp_reg[1], eew, 0, 1);
        idx++;
      }
    }
  }

  vstart->val = 0;
  set_mstatus_dirt();
}

void vsr(int mode, Decode *s, int mmu_mode) {
  word_t idx, vreg_idx, offset, pos;
  uint64_t len, base_addr, vd, addr, elt_per_reg, size;

  // previous decode does not load vals for us
  rtl_lr(s, &(s->src1.val), s->src1.reg, 4);
  rtl_mv(s, &(tmp_reg[0]), &(s->src1.val));

  len = s->v_nf + 1;
  elt_per_reg = vlenb->val;
  size = len * elt_per_reg;
  base_addr = tmp_reg[0];
  vd = id_dest->reg;
  idx = vstart->val;

  if (vstart->val < size) {
    vreg_idx = vstart->val / elt_per_reg;
    offset = vstart->val % elt_per_reg;
    if (offset) {
      // first vreg
      for (pos = offset; pos < elt_per_reg; pos++) {
        // read 1 byte and store 1 byte to memory
        get_vreg(vd + vreg_idx, pos, &tmp_reg[1], 0, 0, 0, 1);
        addr = base_addr + idx;
        rtl_sm(s, &tmp_reg[1], &addr, 0, 1, mmu_mode);
        idx++;
      }
      vreg_idx++;
    }
    for (; vreg_idx < len; vreg_idx++) {
      for (pos = 0; pos < elt_per_reg; pos++) {
        // read 1 byte and store 1 byte to memory
        get_vreg(vd + vreg_idx, pos, &tmp_reg[1], 0, 0, 0, 1);
        addr = base_addr + idx;
        rtl_sm(s, &tmp_reg[1], &addr, 0, 1, mmu_mode);
        idx++;
      }
    }
  }

  vstart->val = 0;
  set_mstatus_dirt();
}

#endif // CONFIG_RVV
