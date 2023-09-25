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
  s->v_width = s->v_width == 0 ? 1 << vtype->vsew : s->v_width;
  bool error = (s->v_width * 8) > (8 << vtype->vsew);
  if(error) {
    printf("vld encounter an instr: v_width > SEW: mode::%d is_signed:%d\n", mode, is_signed);
    longjmp_raise_intr(EX_II);
  }
  // previous decode does not load vals for us 
  rtl_lr(s, &(s->src1.val), s->src1.reg, 4);

  word_t idx;
  rtl_mv(s, &(tmp_reg[0]), &(s->src1.val));
  for(idx = vstart->val; idx < vl->val; idx ++) {
    //TODO: SEW now only supports LE 64bit
    //TODO: need special rtl function, but here ignore it
    if(mode == MODE_INDEXED) {
      rtl_mv(s, &(tmp_reg[0]), &(s->src1.val));
      get_vreg(id_src2->reg, idx, &tmp_reg[3], vtype->vsew, vtype->vlmul, 1, 1);
      rtl_add(s, &tmp_reg[0], &tmp_reg[0], &tmp_reg[3]);
    }
    
    // mask
    rtlreg_t mask = get_mask(0, idx, vtype->vsew, vtype->vlmul);
    
    // op
    if(s->vm != 0 || mask != 0) {
      rtl_lm(s, &tmp_reg[1], &tmp_reg[0], 0, s->v_width, mmu_mode);
      if (is_signed) rtl_sext(s, &tmp_reg[1], &tmp_reg[1], s->v_width);
      
      set_vreg(id_dest->reg, idx, *&tmp_reg[1], vtype->vsew, vtype->vlmul, 1);
    }
    
    switch (mode) {
      case MODE_UNIT   : rtl_addi(s, &tmp_reg[0], &tmp_reg[0], s->v_width); break;
      case MODE_STRIDED: rtl_add(s, &tmp_reg[0], &tmp_reg[0], &id_src2->val) ; break;
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
  s->v_width = s->v_width == 0 ? 1 << vtype->vsew : s->v_width;
  bool error = (s->v_width * 8) < (8 << vtype->vsew);
  if(error) {
    printf("vst encounter an instr: v_width < SEW: mode::%d\n", mode);
    longjmp_raise_intr(EX_II);
  }

  rtl_lr(s, &(s->src1.val), s->src1.reg, 4);

  word_t idx;
  rtl_mv(s, &(tmp_reg[0]), &(s->src1.val));
  for(idx = vstart->val; idx < vl->val; idx ++) {
    //TODO: SEW now only supports LE 64bit
    //TODO: need special rtl function, but here ignore it
    if(mode == MODE_INDEXED) {
      rtl_mv(s, &tmp_reg[0], &id_src->val);
      get_vreg(id_src2->reg, idx, &tmp_reg[3], vtype->vsew, vtype->vlmul, 1, 1);
      rtl_add(s, &tmp_reg[0], &tmp_reg[0], &tmp_reg[3]);
      // switch(vtype->vsew) {
      //   case 0 : rtl_addi(&&tmp_reg[0], &&tmp_reg[0], vreg_b(id_src2->reg, idx)); break;
      //   case 1 : rtl_addi(&&tmp_reg[0], &&tmp_reg[0], vreg_s(id_src2->reg, idx)); break;
      //   case 2 : rtl_addi(&&tmp_reg[0], &&tmp_reg[0], vreg_i(id_src2->reg, idx)); break;
      //   case 3 : rtl_addi(&&tmp_reg[0], &&tmp_reg[0], vreg_l(id_src2->reg, idx)); break;
      // }
    }
    
    // mask
    // uint8_t mask;
    // switch (vtype->vsew) {
    //   case 0 : mask = (uint8_t)(vreg_b(0, idx) & 0x1); break;
    //   case 1 : mask = (uint8_t)(vreg_s(0, idx) & 0x1); break;
    //   case 2 : mask = (uint8_t)(vreg_i(0, idx) & 0x1); break;
    //   case 3 : mask = (uint8_t)(vreg_l(0, idx) & 0x1); break;
    //   default: mask = 0;
    // }
    rtlreg_t mask = get_mask(0, idx, vtype->vsew, vtype->vlmul);

    // op
    if(s->vm != 0 || mask != 0) {
      // switch (vtype->vsew) {
      //   case 0 : rtl_li(&&tmp_reg[1], vreg_b(id_dest->reg, idx)); break;
      //   case 1 : rtl_li(&&tmp_reg[1], vreg_s(id_dest->reg, idx)); break;
      //   case 2 : rtl_li(&&tmp_reg[1], vreg_i(id_dest->reg, idx)); break;
      //   case 3 : rtl_li(&&tmp_reg[1], vreg_l(id_dest->reg, idx)); break;
      // }
      get_vreg(id_dest->reg, idx, &tmp_reg[1], vtype->vsew, vtype->vlmul, 0, 1);
      rtl_sm(s, &tmp_reg[1], &tmp_reg[0], 0, s->v_width, mmu_mode);
    }

    switch (mode) {
      case MODE_UNIT   : rtl_addi(s, &tmp_reg[0], &tmp_reg[0], s->v_width); break;
      case MODE_STRIDED: rtl_add(s, &tmp_reg[0], &tmp_reg[0], &id_src2->val) ; break;
    }
  }
  // TODO: the idx larger than vl need reset to zero.
  vstart->val = 0;
  set_mstatus_dirt();
}

#endif // CONFIG_RVV