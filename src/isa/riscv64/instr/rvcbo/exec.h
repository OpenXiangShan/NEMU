/***************************************************************************************
* Copyright (c) 2020-2024 Institute of Computing Technology, Chinese Academy of Sciences
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

#include "cbo_impl.h"
#include <common.h>

def_EHelper(cbo_zero) {
  // check illegal instruction exception
  if((cpu.mode != MODE_M && !menvcfg->cbze) || (!cpu.v && cpu.mode == MODE_U && !senvcfg->cbze)){
    longjmp_exception(EX_II);
  } else if(cpu.v && ((cpu.mode == MODE_S && !henvcfg->cbze) || (cpu.mode == MODE_U && !(henvcfg->cbze && senvcfg->cbze)))){
    longjmp_exception(EX_VI);
  }

  cbo_zero(s);
}

def_EHelper(cbo_inval) {
  // check illegal instruction exception
  if((cpu.mode != MODE_M && menvcfg->cbie == 0) || (!cpu.v && cpu.mode == MODE_U && senvcfg->cbie == 0)){
    longjmp_exception(EX_II);
  } else if(cpu.v && ((cpu.mode == MODE_S && henvcfg->cbie == 0) || (cpu.mode == MODE_U && (henvcfg->cbie == 0 || senvcfg->cbie == 0)))){
    longjmp_exception(EX_VI);
  }

  cbo_inval(s);
}

def_EHelper(cbo_flush) {
  // check illegal instruction exception
  if((cpu.mode != MODE_M && !menvcfg->cbcfe) || (!cpu.v && cpu.mode == MODE_U && !senvcfg->cbcfe)){
    longjmp_exception(EX_II);
  } else if(cpu.v && ((cpu.mode == MODE_S && !henvcfg->cbcfe) || (cpu.mode == MODE_U && !(henvcfg->cbcfe && senvcfg->cbcfe)))){
    longjmp_exception(EX_VI);
  }

  cbo_flush(s);
}

def_EHelper(cbo_clean) {
  // check illegal instruction exception
  if((cpu.mode != MODE_M && !menvcfg->cbcfe) || (!cpu.v && cpu.mode == MODE_U && !senvcfg->cbcfe)){
    longjmp_exception(EX_II);
  } else if(cpu.v && ((cpu.mode == MODE_S && !henvcfg->cbcfe) || (cpu.mode == MODE_U && !(henvcfg->cbcfe && senvcfg->cbcfe)))){
    longjmp_exception(EX_VI);
  }

  cbo_clean(s);
}

/*
 * MMU Pattern
 */

def_EHelper(cbo_zero_mmu) {
  // check illegal instruction exception
  if((cpu.mode != MODE_M && !menvcfg->cbze) || (!cpu.v && cpu.mode == MODE_U && !senvcfg->cbze)){
    longjmp_exception(EX_II);
  } else if(cpu.v && ((cpu.mode == MODE_S && !henvcfg->cbze) || (cpu.mode == MODE_U && !(henvcfg->cbze && senvcfg->cbze)))){
    longjmp_exception(EX_VI);
  }

  cbo_zero_mmu(s);
}

def_EHelper(cbo_inval_mmu) {
  // check illegal instruction exception
  if((cpu.mode != MODE_M && menvcfg->cbie == 0) || (!cpu.v && cpu.mode == MODE_U && senvcfg->cbie == 0)){
    longjmp_exception(EX_II);
  } else if(cpu.v && ((cpu.mode == MODE_S && henvcfg->cbie == 0) || (cpu.mode == MODE_U && (henvcfg->cbie == 0 || senvcfg->cbie == 0)))){
    longjmp_exception(EX_VI);
  }

  cbo_inval_mmu(s);
}

def_EHelper(cbo_flush_mmu) {
  // check illegal instruction exception
  if((cpu.mode != MODE_M && !menvcfg->cbcfe) || (!cpu.v && cpu.mode == MODE_U && !senvcfg->cbcfe)){
    longjmp_exception(EX_II);
  } else if(cpu.v && ((cpu.mode == MODE_S && !henvcfg->cbcfe) || (cpu.mode == MODE_U && !(henvcfg->cbcfe && senvcfg->cbcfe)))){
    longjmp_exception(EX_VI);
  }

  cbo_flush_mmu(s);
}

def_EHelper(cbo_clean_mmu) {
  // check illegal instruction exception
  if((cpu.mode != MODE_M && !menvcfg->cbcfe) || (!cpu.v && cpu.mode == MODE_U && !senvcfg->cbcfe)){
    longjmp_exception(EX_II);
  } else if(cpu.v && ((cpu.mode == MODE_S && !henvcfg->cbcfe) || (cpu.mode == MODE_U && !(henvcfg->cbcfe && senvcfg->cbcfe)))){
    longjmp_exception(EX_VI);
  }

  cbo_clean_mmu(s);
}