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


def_EHelper(vlduu) { //unit-strided
  /*
  switch (s->v_width) {
    case 0 : print_asm_template3(vle.v);
    case 1 : print_asm_template3(vlbu.v);
    case 2 : print_asm_template3(vlhu.v);
    case 4 : print_asm_template3(vlwu.v);
  }
  */
  VLD(MODE_UNIT, UNSIGNED, s, MMU_DIRECT)
  //print_asm_template3(vlduu);
}

def_EHelper(vldsu) { //strided unsigned
  /*
  switch (s->v_width) {
    case 0 : print_asm_template3(vlse.v);
    case 1 : print_asm_template3(vlsbu.v);
    case 2 : print_asm_template3(vlshu.v);
    case 4 : print_asm_template3(vlswu.v);
  }*/
  s->src2.reg = s->isa.instr.fp.rs2;
  rtl_lr(s, &(s->src2.val), s->src2.reg, 4);
  VLD(MODE_STRIDED, UNSIGNED, s, MMU_DIRECT)
  //print_asm_template3(vldsu);
}

def_EHelper(vldxu) {
  /*
  switch (s->v_width) {
    case 0 : print_asm_template3(vlxe.v);
    case 1 : print_asm_template3(vlxbu.v);
    case 2 : print_asm_template3(vlxhu.v);
    case 4 : print_asm_template3(vlxwu.v);
  } */
  s->src2.reg = s->isa.instr.fp.rs2;
  rtl_lr(s, &(s->src2.val), s->src2.reg, 4);
  VLD(MODE_INDEXED, UNSIGNED, s, MMU_DIRECT)
  //print_asm_template3(vldxu);
}

def_EHelper(vldus) {
  /*
  switch (s->v_width) {
    case 0 : print_asm_template3(vle.v);
    case 1 : print_asm_template3(vlb.v);
    case 2 : print_asm_template3(vlh.v);
    case 4 : print_asm_template3(vlw.v);
  }*/
  VLD(MODE_UNIT, SIGNED, s, MMU_DIRECT)
  //print_asm_template3(vldus);
}

def_EHelper(vldss) {
  /*
  switch (s->v_width) {
    case 0 : print_asm_template3(vlse.v);
    case 1 : print_asm_template3(vlsb.v);
    case 2 : print_asm_template3(vlsh.v);
    case 4 : print_asm_template3(vlsw.v);
  }*/
  s->src2.reg = s->isa.instr.fp.rs2;
  rtl_lr(s, &(s->src2.val), s->src2.reg, 4);
  VLD(MODE_STRIDED, SIGNED, s, MMU_DIRECT)
  //print_asm_template3(vldss);
}

def_EHelper(vldxs) {
  /*
  switch (s->v_width) {
    case 0 : print_asm_template3(vlxe.v);
    case 1 : print_asm_template3(vlxb.v);
    case 2 : print_asm_template3(vlxh.v);
    case 4 : print_asm_template3(vlxw.v);
  }*/
  s->src2.reg = s->isa.instr.fp.rs2;
  rtl_lr(s, &(s->src2.val), s->src2.reg, 4);
  VLD(MODE_INDEXED, SIGNED, s, MMU_DIRECT)
  //print_asm_template3(vldxs);
}




def_EHelper(vstu) {
  /*
  switch (s->v_width) {
    case 0 : print_asm_template3(vse.v);
    case 1 : print_asm_template3(vsb.v);
    case 2 : print_asm_template3(vsh.v);
    case 4 : print_asm_template3(vsw.v);
  }*/
  VST(MODE_UNIT, MMU_DIRECT)
  //print_asm_template3(vstu);
}

def_EHelper(vsts) {
  /*
  switch (s->v_width) {
    case 0 : print_asm_template3(vsse.v);
    case 1 : print_asm_template3(vssb.v);
    case 2 : print_asm_template3(vssh.v);
    case 4 : print_asm_template3(vssw.v);
  }*/
  s->src2.reg = s->isa.instr.fp.rs2;
  rtl_lr(s, &(s->src2.val), s->src2.reg, 4);
  VST(MODE_STRIDED, MMU_DIRECT)
  //print_asm_template3(vsts);
}

def_EHelper(vstx) {
  /*
  switch (s->v_width) {
    case 0 : print_asm_template3(vsxe.v);
    case 1 : print_asm_template3(vsxb.v);
    case 2 : print_asm_template3(vsxh.v);
    case 4 : print_asm_template3(vsxw.v);
  }*/
  s->src2.reg = s->isa.instr.fp.rs2;
  rtl_lr(s, &(s->src2.val), s->src2.reg, 4);
  VST(MODE_INDEXED, MMU_DIRECT)
  //print_asm_template3(vstx);
}

def_EHelper(vstxu) {
  /*
  switch (s->v_width) {
    case 0 : print_asm_template3(vsuxe.v);
    case 1 : print_asm_template3(vsuxb.v);
    case 2 : print_asm_template3(vsuxh.v);
    case 4 : print_asm_template3(vsuxw.v);
  }*/
  s->src2.reg = s->isa.instr.fp.rs2;
  rtl_lr(s, &(s->src2.val), s->src2.reg, 4);
  VST(MODE_INDEXED, MMU_DIRECT)
  //print_asm_template3(vstxu);
}

def_EHelper(vlduu_mmu) { //unit-strided
  /*
  switch (s->v_width) {
    case 0 : print_asm_template3(vle.v);
    case 1 : print_asm_template3(vlbu.v);
    case 2 : print_asm_template3(vlhu.v);
    case 4 : print_asm_template3(vlwu.v);
  }
  */
  VLD(MODE_UNIT, UNSIGNED, s, MMU_TRANSLATE)
  //print_asm_template3(vlduu);
}

def_EHelper(vldsu_mmu) { //strided unsigned
  /*
  switch (s->v_width) {
    case 0 : print_asm_template3(vlse.v);
    case 1 : print_asm_template3(vlsbu.v);
    case 2 : print_asm_template3(vlshu.v);
    case 4 : print_asm_template3(vlswu.v);
  }*/
  s->src2.reg = s->isa.instr.fp.rs2;
  rtl_lr(s, &(s->src2.val), s->src2.reg, 4);
  VLD(MODE_STRIDED, UNSIGNED, s, MMU_TRANSLATE)
  //print_asm_template3(vldsu);
}

def_EHelper(vldxu_mmu) {
  /*
  switch (s->v_width) {
    case 0 : print_asm_template3(vlxe.v);
    case 1 : print_asm_template3(vlxbu.v);
    case 2 : print_asm_template3(vlxhu.v);
    case 4 : print_asm_template3(vlxwu.v);
  } */
  s->src2.reg = s->isa.instr.fp.rs2;
  rtl_lr(s, &(s->src2.val), s->src2.reg, 4);
  VLD(MODE_INDEXED, UNSIGNED, s, MMU_TRANSLATE)
  //print_asm_template3(vldxu);
}

def_EHelper(vldus_mmu) {
  /*
  switch (s->v_width) {
    case 0 : print_asm_template3(vle.v);
    case 1 : print_asm_template3(vlb.v);
    case 2 : print_asm_template3(vlh.v);
    case 4 : print_asm_template3(vlw.v);
  }*/
  VLD(MODE_UNIT, SIGNED, s, MMU_TRANSLATE)
  //print_asm_template3(vldus);
}

def_EHelper(vldss_mmu) {
  /*
  switch (s->v_width) {
    case 0 : print_asm_template3(vlse.v);
    case 1 : print_asm_template3(vlsb.v);
    case 2 : print_asm_template3(vlsh.v);
    case 4 : print_asm_template3(vlsw.v);
  }*/
  VLD(MODE_STRIDED, SIGNED, s, MMU_TRANSLATE)
  //print_asm_template3(vldss);
}

def_EHelper(vldxs_mmu) {
  /*
  switch (s->v_width) {
    case 0 : print_asm_template3(vlxe.v);
    case 1 : print_asm_template3(vlxb.v);
    case 2 : print_asm_template3(vlxh.v);
    case 4 : print_asm_template3(vlxw.v);
  }*/
  s->src2.reg = s->isa.instr.fp.rs2;
  rtl_lr(s, &(s->src2.val), s->src2.reg, 4);
  VLD(MODE_INDEXED, SIGNED, s, MMU_TRANSLATE)
  //print_asm_template3(vldxs);
}




def_EHelper(vstu_mmu) {
  /*
  switch (s->v_width) {
    case 0 : print_asm_template3(vse.v);
    case 1 : print_asm_template3(vsb.v);
    case 2 : print_asm_template3(vsh.v);
    case 4 : print_asm_template3(vsw.v);
  }*/
  VST(MODE_UNIT, MMU_TRANSLATE)
  //print_asm_template3(vstu);
}

def_EHelper(vsts_mmu) {
  /*
  switch (s->v_width) {
    case 0 : print_asm_template3(vsse.v);
    case 1 : print_asm_template3(vssb.v);
    case 2 : print_asm_template3(vssh.v);
    case 4 : print_asm_template3(vssw.v);
  }*/
  VST(MODE_STRIDED, MMU_TRANSLATE)
  //print_asm_template3(vsts);
}

def_EHelper(vstx_mmu) {
  /*
  switch (s->v_width) {
    case 0 : print_asm_template3(vsxe.v);
    case 1 : print_asm_template3(vsxb.v);
    case 2 : print_asm_template3(vsxh.v);
    case 4 : print_asm_template3(vsxw.v);
  }*/
  s->src2.reg = s->isa.instr.fp.rs2;
  rtl_lr(s, &(s->src2.val), s->src2.reg, 4);
  VST(MODE_INDEXED, MMU_TRANSLATE)
  //print_asm_template3(vstx);
}

def_EHelper(vstxu_mmu) {
  /*
  switch (s->v_width) {
    case 0 : print_asm_template3(vsuxe.v);
    case 1 : print_asm_template3(vsuxb.v);
    case 2 : print_asm_template3(vsuxh.v);
    case 4 : print_asm_template3(vsuxw.v);
  }*/
  s->src2.reg = s->isa.instr.fp.rs2;
  rtl_lr(s, &(s->src2.val), s->src2.reg, 4);
  VST(MODE_INDEXED, MMU_TRANSLATE)
  //print_asm_template3(vstxu);
}

#endif // CONFIG_RVV