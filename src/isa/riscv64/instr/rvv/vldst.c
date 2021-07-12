#include "cpu/exec.h"
#include "vreg.h"
#include "../local-include/csr.h"
#include "../local-include/rtl.h"
#include <stdio.h>
#include "../local-include/intr.h"
#include <setjmp.h>

#define id_src (&s->src1)
#define id_src2 (&s->src2)
#define id_dest (&s->dest)

// vector load
#define MODE_UNIT    0
#define MODE_STRIDED 1
#define MODE_INDEXED 2

#define VLD(mode, is_signed, s) vld(mode, is_signed, s);
static void vld(int mode, int is_signed, Decode *s) {
  
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
  rtl_mv(s, &(s->tmp_reg[0]), &(s->src1.val));
  for(idx = vstart->val; idx < vl->val; idx ++) {
    //TODO: SEW now only supports LE 64bit
    //TODO: need special rtl function, but here ignore it
    if(mode == MODE_INDEXED) {
      rtl_mv(s, &(s->tmp_reg[0]), &(s->src1.val));
      get_vreg(id_src2->reg, idx, t0, vtype->vsew, vtype->vlmul, 1, 1);
      rtl_add(s, s0, s0, t0);
    }
    
    // mask
    rtlreg_t mask = get_mask(0, idx, vtype->vsew, vtype->vlmul);
    
    // op
    if(s->vm != 0 || mask != 0) {
      rtl_lm(s, s1, s0, 0, s->v_width);
      if (is_signed) rtl_sext(s, s1, s1, s->v_width);
      
      set_vreg(id_dest->reg, idx, *s1, vtype->vsew, vtype->vlmul, 1);
    }
    
    switch (mode) {
      case MODE_UNIT   : rtl_addi(s, s0, s0, s->v_width); break;
      case MODE_STRIDED: rtl_add(s, s0, s0, &id_src2->val) ; break;
    }
  }

  // TODO: the idx larger than vl need reset to zero.
  rtl_li(s, s0, 0);
  vcsr_write(IDXVSTART, s0);
}

def_EHelper(vlduu) { //unit-strided
  switch (s->v_width) {
    case 0 : print_asm_template3(vle.v);
    case 1 : print_asm_template3(vlbu.v);
    case 2 : print_asm_template3(vlhu.v);
    case 4 : print_asm_template3(vlwu.v);
  }
  VLD(MODE_UNIT, UNSIGNED, s)
  //print_asm_template3(vlduu);
}

def_EHelper(vldsu) { //strided unsigned
  switch (s->v_width) {
    case 0 : print_asm_template3(vlse.v);
    case 1 : print_asm_template3(vlsbu.v);
    case 2 : print_asm_template3(vlshu.v);
    case 4 : print_asm_template3(vlswu.v);
  }
  VLD(MODE_STRIDED, UNSIGNED, s)
  //print_asm_template3(vldsu);
}

def_EHelper(vldxu) {
  switch (s->v_width) {
    case 0 : print_asm_template3(vlxe.v);
    case 1 : print_asm_template3(vlxbu.v);
    case 2 : print_asm_template3(vlxhu.v);
    case 4 : print_asm_template3(vlxwu.v);
  }
  VLD(MODE_INDEXED, UNSIGNED, s)
  //print_asm_template3(vldxu);
}

def_EHelper(vldus) {
  switch (s->v_width) {
    case 0 : print_asm_template3(vle.v);
    case 1 : print_asm_template3(vlb.v);
    case 2 : print_asm_template3(vlh.v);
    case 4 : print_asm_template3(vlw.v);
  }
  VLD(MODE_UNIT, SIGNED, s)
  //print_asm_template3(vldus);
}

def_EHelper(vldss) {
  switch (s->v_width) {
    case 0 : print_asm_template3(vlse.v);
    case 1 : print_asm_template3(vlsb.v);
    case 2 : print_asm_template3(vlsh.v);
    case 4 : print_asm_template3(vlsw.v);
  }
  VLD(MODE_STRIDED, SIGNED, s)
  //print_asm_template3(vldss);
}

def_EHelper(vldxs) {
  switch (s->v_width) {
    case 0 : print_asm_template3(vlxe.v);
    case 1 : print_asm_template3(vlxb.v);
    case 2 : print_asm_template3(vlxh.v);
    case 4 : print_asm_template3(vlxw.v);
  }
  VLD(MODE_INDEXED, SIGNED, s)
  //print_asm_template3(vldxs);
}

// vector store
#define VST(mode) vst(mode, s);
static void vst(int mode, Decode *s) {
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

  word_t idx;
  rtl_mv(s, s0, &id_src->val);
  for(idx = vstart->val; idx < vl->val; idx ++) {
    //TODO: SEW now only supports LE 64bit
    //TODO: need special rtl function, but here ignore it
    if(mode == MODE_INDEXED) {
      rtl_mv(s, s0, &id_src->val);
      get_vreg(id_src2->reg, idx, t0, vtype->vsew, vtype->vlmul, 1, 1);
      rtl_add(s, s0, s0, t0);
      // switch(vtype->vsew) {
      //   case 0 : rtl_addi(&s0, &s0, vreg_b(id_src2->reg, idx)); break;
      //   case 1 : rtl_addi(&s0, &s0, vreg_s(id_src2->reg, idx)); break;
      //   case 2 : rtl_addi(&s0, &s0, vreg_i(id_src2->reg, idx)); break;
      //   case 3 : rtl_addi(&s0, &s0, vreg_l(id_src2->reg, idx)); break;
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
      //   case 0 : rtl_li(&s1, vreg_b(id_dest->reg, idx)); break;
      //   case 1 : rtl_li(&s1, vreg_s(id_dest->reg, idx)); break;
      //   case 2 : rtl_li(&s1, vreg_i(id_dest->reg, idx)); break;
      //   case 3 : rtl_li(&s1, vreg_l(id_dest->reg, idx)); break;
      // }
      get_vreg(id_dest->reg, idx, s1, vtype->vsew, vtype->vlmul, 0, 1);
      rtl_sm(s, s0, 0, s1, s->v_width);
    }

    switch (mode) {
      case MODE_UNIT   : rtl_addi(s, s0, s0, s->v_width); break;
      case MODE_STRIDED: rtl_add(s, s0, s0, &id_src2->val) ; break;
    }
  }
  // TODO: the idx larger than vl need reset to zero.
  vstart->val = 0;
}

def_EHelper(vstu) {
  switch (s->v_width) {
    case 0 : print_asm_template3(vse.v);
    case 1 : print_asm_template3(vsb.v);
    case 2 : print_asm_template3(vsh.v);
    case 4 : print_asm_template3(vsw.v);
  }
  VST(MODE_UNIT)
  //print_asm_template3(vstu);
}

def_EHelper(vsts) {
  switch (s->v_width) {
    case 0 : print_asm_template3(vsse.v);
    case 1 : print_asm_template3(vssb.v);
    case 2 : print_asm_template3(vssh.v);
    case 4 : print_asm_template3(vssw.v);
  }
  VST(MODE_STRIDED)
  //print_asm_template3(vsts);
}

def_EHelper(vstx) {
  switch (s->v_width) {
    case 0 : print_asm_template3(vsxe.v);
    case 1 : print_asm_template3(vsxb.v);
    case 2 : print_asm_template3(vsxh.v);
    case 4 : print_asm_template3(vsxw.v);
  }
  VST(MODE_INDEXED)
  //print_asm_template3(vstx);
}

def_EHelper(vstxu) {
  switch (s->v_width) {
    case 0 : print_asm_template3(vsuxe.v);
    case 1 : print_asm_template3(vsuxb.v);
    case 2 : print_asm_template3(vsuxh.v);
    case 4 : print_asm_template3(vsuxw.v);
  }
  VST(MODE_INDEXED)
  //print_asm_template3(vstxu);
}
