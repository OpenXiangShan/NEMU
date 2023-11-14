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
  uint64_t nf, fn, vl_val, base_addr, vd;
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
  vl_val = vl->val;
  base_addr = tmp_reg[0];
  vd = id_dest->reg;
  for (idx = vstart->val; idx < vl_val; idx++) {
    rtlreg_t mask = get_mask(0, idx, vtype->vsew, vtype->vlmul);
    if (s->vm == 0 && mask == 0) {
      if (RVV_AGNOSTIC && vtype->vma) {
        tmp_reg[1] = (uint64_t) -1;
        for (fn = 0; fn < nf; fn++) {
          set_vreg(vd + fn * emul, idx, *&tmp_reg[1], eew, vtype->vlmul, 1);
        }
      }
      continue;
    }
    for (fn = 0; fn < nf; fn++) {
      uint64_t addr = base_addr + idx * stride + (idx * nf * is_stride + fn) * s->v_width;
      rtl_lm(s, &tmp_reg[1], &addr, 0, s->v_width, mmu_mode);
      set_vreg(vd + fn * emul, idx, *&tmp_reg[1], eew, vtype->vlmul, 1);
    }
  }

  if (RVV_AGNOSTIC) {   // set start of vector register to 1
    for (idx = 0; idx < vstart->val; idx++) {
      tmp_reg[1] = (uint64_t) -1;
      for (fn = 0; fn < nf; fn++) {
        set_vreg(vd + fn * emul, idx, *&tmp_reg[1], eew, vtype->vlmul, 1);
      }
    }
  }

  if (RVV_AGNOSTIC && vtype->vta) {   // set tail of vector register to 1
    int vlmax = get_vlen_max(vtype->vsew, vtype->vlmul);
    for(idx = vl->val; idx < vlmax; idx++) {
      tmp_reg[1] = (uint64_t) -1;
      for (fn = 0; fn < nf; fn++) {
        set_vreg(vd + fn * emul, idx, *&tmp_reg[1], eew, vtype->vlmul, 1);
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
  int index_width = 0;
  int eew = 0;
  switch(s->v_width) {
    case 1: eew = 0; break;
    case 2: eew = 1; break;
    case 4: eew = 2; break;
    case 8: eew = 3; break;
    default: break;
  }
  if(mode == MODE_INDEXED) {
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
  }
  // previous decode does not load vals for us 
  rtl_lr(s, &(s->src1.val), s->src1.reg, 4);

  word_t idx, nf_idx, vl_idx;
  rtl_mv(s, &(tmp_reg[0]), &(s->src1.val));

  uint64_t load_vl = vl->val;

  int emul = vtype->vlmul << (eew - vtype->vsew);

  for(vl_idx = vstart->val; vl_idx < load_vl; vl_idx ++) {
    for (nf_idx = 0; nf_idx <= s->v_nf; nf_idx++) {
      idx = vl_idx + nf_idx * emul;
      //TODO: SEW now only supports LE 64bit
      //TODO: need special rtl function, but here ignore it
      if(mode == MODE_INDEXED) {
        rtl_mv(s, &(tmp_reg[0]), &(s->src1.val));
        get_vreg(id_src2->reg, idx, &tmp_reg[3], index_width, vtype->vlmul, 1, 1);
        rtl_add(s, &tmp_reg[0], &tmp_reg[0], &tmp_reg[3]);
      }
      
      // mask
      rtlreg_t mask = get_mask(0, idx, vtype->vsew, vtype->vlmul);
      
      // op
      if(s->vm != 0 || mask != 0) {
        rtl_lm(s, &tmp_reg[1], &tmp_reg[0], 0, s->v_width, mmu_mode);
        if (is_signed) rtl_sext(s, &tmp_reg[1], &tmp_reg[1], s->v_width);
        set_vreg(id_dest->reg, idx, *&tmp_reg[1], eew, vtype->vlmul, 1);
      } else if (s->vm == 0 && mask==0) {
          if (RVV_AGNOSTIC && vtype->vma) {
            tmp_reg[1] = (uint64_t) -1;
            set_vreg(id_dest->reg, idx, *&tmp_reg[1], eew, vtype->vlmul, 1);
          }
      }
      
      switch (mode) {
        case MODE_UNIT   : rtl_addi(s, &tmp_reg[0], &tmp_reg[0], s->v_width); break;
        case MODE_STRIDED: rtl_add(s, &tmp_reg[0], &tmp_reg[0], &id_src2->val) ; break;
        //default : assert(0);
      }
    }

    if (RVV_AGNOSTIC && vtype->vta) {
      int vlmax = get_vlen_max(vtype->vsew, vtype->vlmul);
      for(idx = vl->val; idx < vlmax; idx++) {
        tmp_reg[1] = (uint64_t) -1;
        set_vreg(id_dest->reg, idx, *&tmp_reg[1], eew, vtype->vlmul, 1);
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
  uint64_t nf, fn, vl_val, base_addr, vd;
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
  vl_val = vl->val;
  base_addr = tmp_reg[0];
  vd = id_dest->reg;
  for (idx = vstart->val; idx < vl_val; idx++) {
    rtlreg_t mask = get_mask(0, idx, vtype->vsew, vtype->vlmul);
    if (s->vm == 0 && mask == 0) {
      continue;
    }
    for (fn = 0; fn < nf; fn++) {
      get_vreg(vd + fn * emul, idx, &tmp_reg[1], eew, vtype->vlmul, 0, 1);
      uint64_t addr = base_addr + idx * stride + (idx * nf * is_stride + fn) * s->v_width;
      rtl_sm(s, &tmp_reg[1], &addr, 0, s->v_width, mmu_mode);
    }
  }

  vstart->val = 0;
  set_mstatus_dirt();
}

void vstx(int mode, Decode *s, int mmu_mode) {
  //TODO: raise instr when decinfo.v_width > SEW
  //v_width   0  -> none    SEW   0  ->  8
  //        1  ->  8            1  ->  16
  //        2  ->  16           2  ->  32
  //        4  ->  32           3  ->  64
  //        8  ->  64
  if(vstart->val >= vl->val) return;
  int index_width = 0;
  int eew = 0;
  switch(s->v_width) {
    case 1: eew = 0; break;
    case 2: eew = 1; break;
    case 4: eew = 2; break;
    case 8: eew = 3; break;
    default: break;
  }
  if(mode == MODE_INDEXED) {
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
  }

  rtl_lr(s, &(s->src1.val), s->src1.reg, 4);

  word_t idx, nf_idx, vs_idx;
  rtl_mv(s, &(tmp_reg[0]), &(s->src1.val));

  uint64_t store_vl = vl->val;

  int emul = vtype->vlmul << (eew - vtype->vsew);

  for(vs_idx = vstart->val; vs_idx < store_vl; vs_idx ++) {
    for (nf_idx = 0; nf_idx <= s->v_nf; nf_idx++) {
      idx = vs_idx + nf_idx * emul;
      //TODO: SEW now only supports LE 64bit
      //TODO: need special rtl function, but here ignore it
      if(mode == MODE_INDEXED) {
        rtl_mv(s, &tmp_reg[0], &id_src->val);
        get_vreg(id_src2->reg, idx, &tmp_reg[3], index_width, vtype->vlmul, 1, 1);
        rtl_add(s, &tmp_reg[0], &tmp_reg[0], &tmp_reg[3]);
      }
      
      // mask
      rtlreg_t mask = get_mask(0, idx, vtype->vsew, vtype->vlmul);

      // op
      if(s->vm != 0 || mask != 0) {
        get_vreg(id_dest->reg, idx, &tmp_reg[1], eew, vtype->vlmul, 0, 1);
        rtl_sm(s, &tmp_reg[1], &tmp_reg[0], 0, s->v_width, mmu_mode);
      }

      switch (mode) {
        case MODE_UNIT   : rtl_addi(s, &tmp_reg[0], &tmp_reg[0], s->v_width); break;
        case MODE_STRIDED: rtl_add(s, &tmp_reg[0], &tmp_reg[0], &id_src2->val) ; break;
        //default : assert(0);
      }
    }
  }
  // TODO: the idx larger than vl need reset to zero.
  vstart->val = 0;
  set_mstatus_dirt();
}

void vlr(int mode, int is_signed, Decode *s, int mmu_mode) {
  //TODO: raise instr when decinfo.v_width > SEW
  //v_width   0  -> none    SEW   0  ->  8
  //        1  ->  8            1  ->  16
  //        2  ->  16           2  ->  32
  //        4  ->  32           3  ->  64
  //        8  ->  64
  int vtype_vsew = 0;
  switch(s->v_width) {
    case 1: vtype_vsew = 0; break;
    case 2: vtype_vsew = 1; break;
    case 4: vtype_vsew = 2; break;
    case 8: vtype_vsew = 3; break;
    default: break;
  }
  // previous decode does not load vals for us 
  rtl_lr(s, &(s->src1.val), s->src1.reg, 4);

  word_t idx;
  rtl_mv(s, &(tmp_reg[0]), &(s->src1.val));

  uint64_t load_vl = VLEN / (8*s->v_width) * (s->v_nf+1);

  for(idx = vstart->val; idx < load_vl; idx ++) {
    //TODO: SEW now only supports LE 64bit
    //TODO: need special rtl function, but here ignore it
    
    // mask
    rtlreg_t mask = get_mask(0, idx, vtype->vsew, vtype->vlmul);
    
    // op
    if(s->vm != 0 || mask != 0) {
      rtl_lm(s, &tmp_reg[1], &tmp_reg[0], 0, s->v_width, mmu_mode);
      if (is_signed) rtl_sext(s, &tmp_reg[1], &tmp_reg[1], s->v_width);

      set_vreg(id_dest->reg, idx, *&tmp_reg[1], vtype_vsew, vtype->vlmul, 1);
    } else if (s->vm == 0 && mask==0) {
        if (RVV_AGNOSTIC == 1 && vtype->vma) {
          tmp_reg[1] = (uint64_t) -1;
          set_vreg(id_dest->reg, idx, *&tmp_reg[1], vtype_vsew, vtype->vlmul, 1);
        }
    }
    
    rtl_addi(s, &tmp_reg[0], &tmp_reg[0], s->v_width);
  }

  if (RVV_AGNOSTIC == 1 && vtype->vta) {
    int vlmax = get_vlen_max(vtype->vsew, vtype->vlmul);
    for(idx = vl->val; idx < vlmax; idx++) {
      tmp_reg[1] = (uint64_t) -1;
      set_vreg(id_dest->reg, idx, *&tmp_reg[1], vtype_vsew, vtype->vlmul, 1);
    }
  }

  // TODO: the idx larger than vl need reset to zero.
  vstart->val = 0;
  set_mstatus_dirt();
}

void vlm(int mode, int is_signed, Decode *s, int mmu_mode) {
  //TODO: raise instr when decinfo.v_width > SEW
  //v_width   0  -> none    SEW   0  ->  8
  //        1  ->  8            1  ->  16
  //        2  ->  16           2  ->  32
  //        4  ->  32           3  ->  64
  //        8  ->  64
  int vtype_vsew = 0;
  switch(s->v_width) {
    case 1: vtype_vsew = 0; break;
    case 2: vtype_vsew = 1; break;
    case 4: vtype_vsew = 2; break;
    case 8: vtype_vsew = 3; break;
    default: break;
  }
  // previous decode does not load vals for us 
  rtl_lr(s, &(s->src1.val), s->src1.reg, 4);

  word_t idx;
  rtl_mv(s, &(tmp_reg[0]), &(s->src1.val));

  uint64_t load_vl = (vl->val + 8*s->v_width - 1) / (8*s->v_width);

  for(idx = vstart->val; idx < load_vl; idx ++) {
    //TODO: SEW now only supports LE 64bit
    //TODO: need special rtl function, but here ignore it
    // mask
    rtlreg_t mask = get_mask(0, idx, vtype->vsew, vtype->vlmul);
    
    // op
    if(s->vm != 0 || mask != 0) {
      rtl_lm(s, &tmp_reg[1], &tmp_reg[0], 0, s->v_width, mmu_mode);
      if (is_signed) rtl_sext(s, &tmp_reg[1], &tmp_reg[1], s->v_width);
      if (idx == load_vl - 1 && vl->val % (8*s->v_width) != 0) {
        // last bits of the last element
        int remain_len = vl->val % (8*s->v_width);
        uint64_t mask = (1LU << remain_len) - 1;
        tmp_reg[1] = tmp_reg[1] & mask;
      }

      set_vreg(id_dest->reg, idx, *&tmp_reg[1], vtype_vsew, vtype->vlmul, 1);
    } else if (s->vm == 0 && mask==0) {
        if (RVV_AGNOSTIC == 1 && vtype->vma) {
          tmp_reg[1] = (uint64_t) -1;
          set_vreg(id_dest->reg, idx, *&tmp_reg[1], vtype_vsew, vtype->vlmul, 1);
        }
    }
    
    rtl_addi(s, &tmp_reg[0], &tmp_reg[0], s->v_width);
  }

  if (RVV_AGNOSTIC == 1 && vtype->vta) {
    int vlmax = get_vlen_max(vtype->vsew, vtype->vlmul);
    for(idx = vl->val; idx < vlmax; idx++) {
      tmp_reg[1] = (uint64_t) -1;
      set_vreg(id_dest->reg, idx, *&tmp_reg[1], vtype_vsew, vtype->vlmul, 1);
    }
  }

  // TODO: the idx larger than vl need reset to zero.
  vstart->val = 0;
  set_mstatus_dirt();
}

void vsr(int mode, Decode *s, int mmu_mode) {
  //TODO: raise instr when decinfo.v_width > SEW
  //v_width   0  -> none    SEW   0  ->  8
  //        1  ->  8            1  ->  16
  //        2  ->  16           2  ->  32
  //        4  ->  32           3  ->  64
  //        8  ->  64
  int vtype_vsew = 0;
  switch(s->v_width) {
    case 1: vtype_vsew = 0; break;
    case 2: vtype_vsew = 1; break;
    case 4: vtype_vsew = 2; break;
    case 8: vtype_vsew = 3; break;
    default: break;
  }
  rtl_lr(s, &(s->src1.val), s->src1.reg, 4);

  word_t idx;
  rtl_mv(s, &(tmp_reg[0]), &(s->src1.val));

  uint64_t store_vl = VLEN / (8*s->v_width) * (s->v_nf+1);

  for(idx = vstart->val; idx < store_vl; idx ++) {
    //TODO: SEW now only supports LE 64bit
    //TODO: need special rtl function, but here ignore it
    
    // mask
    rtlreg_t mask = get_mask(0, idx, vtype->vsew, vtype->vlmul);

    // op
    if(s->vm != 0 || mask != 0) {
      get_vreg(id_dest->reg, idx, &tmp_reg[1], vtype_vsew, vtype->vlmul, 0, 1);
      rtl_sm(s, &tmp_reg[1], &tmp_reg[0], 0, s->v_width, mmu_mode);
    }

    rtl_addi(s, &tmp_reg[0], &tmp_reg[0], s->v_width);
  }
  // TODO: the idx larger than vl need reset to zero.
  vstart->val = 0;
  set_mstatus_dirt();
}

void vsm(int mode, Decode *s, int mmu_mode) {
  //TODO: raise instr when decinfo.v_width > SEW
  //v_width   0  -> none    SEW   0  ->  8
  //        1  ->  8            1  ->  16
  //        2  ->  16           2  ->  32
  //        4  ->  32           3  ->  64
  //        8  ->  64
  int vtype_vsew = 0;
  switch(s->v_width) {
    case 1: vtype_vsew = 0; break;
    case 2: vtype_vsew = 1; break;
    case 4: vtype_vsew = 2; break;
    case 8: vtype_vsew = 3; break;
    default: break;
  }

  rtl_lr(s, &(s->src1.val), s->src1.reg, 4);

  word_t idx;
  rtl_mv(s, &(tmp_reg[0]), &(s->src1.val));

  uint64_t store_vl = (vl->val + 8*s->v_width - 1) / (8*s->v_width);

  for(idx = vstart->val; idx < store_vl; idx ++) {
    //TODO: SEW now only supports LE 64bit
    //TODO: need special rtl function, but here ignore it
    
    // mask
    rtlreg_t mask = get_mask(0, idx, vtype->vsew, vtype->vlmul);

    // op
    if(s->vm != 0 || mask != 0) {
      get_vreg(id_dest->reg, idx, &tmp_reg[1], vtype_vsew, vtype->vlmul, 0, 1);
      if (idx == store_vl - 1 && vl->val % (8*s->v_width) != 0) {
        // last bits of the last element
        int remain_len = vl->val % (8*s->v_width);
        uint64_t mask = (1LU << remain_len) - 1;
        tmp_reg[1] = tmp_reg[1] & mask;
      }
      rtl_sm(s, &tmp_reg[1], &tmp_reg[0], 0, s->v_width, mmu_mode);
    }

    rtl_addi(s, &tmp_reg[0], &tmp_reg[0], s->v_width);
  }
  // TODO: the idx larger than vl need reset to zero.
  vstart->val = 0;
  set_mstatus_dirt();
}

#endif // CONFIG_RVV
