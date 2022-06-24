/***************************************************************************************
* Copyright (c) 2014-2021 Zihao Yu, Nanjing University
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

#include <cpu/decode.h>
#include "../local-include/rtl.h"

#define INSTR_NULLARY(f) \
  f(inv) f(nemu_trap) f(syscall) f(eret) f(tlbwr) f(tlbwi) f(tlbp) f(ret)
#define INSTR_UNARY(f) \
  f(j) f(jal) f(mfhi) f(mflo) f(mthi) f(mtlo)
#define INSTR_BINARY(f) \
  f(lui) f(jr) f(jalr) f(clz) \
  f(lw) f(sw) f(lh) f(lb) f(lhu) f(lbu) f(sh) f(sb) f(swl) f(swr) f(lwl) f(lwr)
#define INSTR_TERNARY(f) \
  f(add) f(sub) f(slt) f(sltu) f(and) f(or) f(xor) f(nor) f(sll) f(srl) f(sra) \
  f(addi) f(slti) f(sltui) f(andi) f(ori) f(xori) f(slli) f(srli) f(srai) f(movz) f(movn) \
  f(beq) f(bne) f(blez) f(bltz) f(bgtz) f(bgez) \
  f(mul) f(mult) f(multu) f(div) f(divu) \
  f(mfc0) f(mtc0)

def_all_EXEC_ID();
