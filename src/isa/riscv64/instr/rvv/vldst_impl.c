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

void vld(int mode, int is_signed, Decode *s, int mmu_mode) {
  //TODO: raise instr when decinfo.v_width > SEW
  //v_width   0  -> none    SEW   0  ->  8
  //        1  ->  8            1  ->  16
  //        2  ->  16           2  ->  32
  //        4  ->  32           3  ->  64
  //        8  ->  64
  int index_width = 0;
  if(mode == MODE_INDEXED) {
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

  word_t idx;
  rtl_mv(s, &(tmp_reg[0]), &(s->src1.val));

  uint64_t load_vl = vl->val * (s->v_nf+1);
  if (mode == MODE_UNIT) {
    switch (s->v_lsumop) {
      case 0b01000: load_vl = VLEN / (8*s->v_width) * (s->v_nf+1); break;
      case 0b01011: load_vl = (vl->val + 8*s->v_width - 1) / (8*s->v_width); break;
    }
  }

  for(idx = vstart->val; idx < load_vl; idx ++) {
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
      if (mode == MODE_UNIT && s->v_lsumop == 0b01011 && idx == load_vl - 1 && vl->val % (8*s->v_width) != 0) {
        // last bits of the last element
        int remain_len = vl->val % (8*s->v_width);
        uint64_t mask = (1LU << remain_len) - 1;
        tmp_reg[1] = tmp_reg[1] & mask;
      }

      set_vreg(id_dest->reg, idx, *&tmp_reg[1], vtype->vsew, vtype->vlmul, 1);
    } else if (s->vm == 0 && mask==0) {
        if (AGNOSTIC == 1 && vtype->vma) {
          tmp_reg[1] = (uint64_t) -1;
          set_vreg(id_dest->reg, idx, *&tmp_reg[1], vtype->vsew, vtype->vlmul, 1);
        }
    }
    
    switch (mode) {
      case MODE_UNIT   : rtl_addi(s, &tmp_reg[0], &tmp_reg[0], s->v_width); break;
      case MODE_STRIDED: rtl_add(s, &tmp_reg[0], &tmp_reg[0], &id_src2->val) ; break;
      //default : assert(0);
    }
  }

  if (AGNOSTIC == 1 && vtype->vta) {
    int vlmax = get_vlen_max(vtype->vsew, vtype->vlmul);
    for(idx = vl->val; idx < vlmax; idx++) {
      tmp_reg[1] = (uint64_t) -1;
      set_vreg(id_dest->reg, idx, *&tmp_reg[1], vtype->vsew, vtype->vlmul, 1);
    }
  }

  // TODO: the idx larger than vl need reset to zero.
  vstart->val = 0;
  set_mstatus_dirt();
}

void vst(int mode, Decode *s, int mmu_mode) {
  //TODO: raise instr when decinfo.v_width > SEW
  //v_width   0  -> none    SEW   0  ->  8
  //        1  ->  8            1  ->  16
  //        2  ->  16           2  ->  32
  //        4  ->  32           3  ->  64
  //        8  ->  64
  int index_width = 0;
  if(mode == MODE_INDEXED) {
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

  word_t idx;
  rtl_mv(s, &(tmp_reg[0]), &(s->src1.val));

  uint64_t store_vl = vl->val * (s->v_nf+1);
  if (mode == MODE_UNIT) {
    switch (s->v_lsumop) {
      case 0b01000: store_vl = VLEN / (8*s->v_width) * (s->v_nf+1); break;
      case 0b01011: store_vl = (vl->val + 8*s->v_width - 1) / (8*s->v_width); break;
    }
  }

  for(idx = vstart->val; idx < store_vl; idx ++) {
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
      get_vreg(id_dest->reg, idx, &tmp_reg[1], vtype->vsew, vtype->vlmul, 0, 1);
      if (mode == MODE_UNIT && s->v_lsumop == 0b01011 && idx == store_vl - 1 && vl->val % (8*s->v_width) != 0) {
        // last bits of the last element
        int remain_len = vl->val % (8*s->v_width);
        uint64_t mask = (1LU << remain_len) - 1;
        tmp_reg[1] = tmp_reg[1] & mask;
      }
      rtl_sm(s, &tmp_reg[1], &tmp_reg[0], 0, s->v_width, mmu_mode);
    }

    switch (mode) {
      case MODE_UNIT   : rtl_addi(s, &tmp_reg[0], &tmp_reg[0], s->v_width); break;
      case MODE_STRIDED: rtl_add(s, &tmp_reg[0], &tmp_reg[0], &id_src2->val) ; break;
      //default : assert(0);
    }
  }
  // TODO: the idx larger than vl need reset to zero.
  vstart->val = 0;
  set_mstatus_dirt();
}

#endif // CONFIG_RVV