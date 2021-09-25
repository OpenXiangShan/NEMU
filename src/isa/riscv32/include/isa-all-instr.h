#include <cpu/decode.h>
#include "../local-include/rtl.h"

#ifndef __ICS_EXPORT

#define INSTR_RVI(f) \
  f(lui) f(auipc) f(jal) \
  f(add) f(sll) f(srl) f(slt) f(sltu) f(xor) f(or) f(sub) f(sra) f(and) \
  f(addi) f(slli) f(srli) f(slti) f(sltui) f(xori) f(ori) f(srai) f(andi) \
  f(lw) f(sw) f(lh) f(lb) f(lhu) f(lbu) f(sh) f(sb) \
  f(lw_mmu) f(sw_mmu) f(lh_mmu) f(lb_mmu) f(lhu_mmu) f(lbu_mmu) f(sh_mmu) f(sb_mmu) \
  f(jalr) f(beq) f(bne) f(blt) f(bge) f(bltu) f(bgeu) \

#define INSTR_RVM(f) \
  f(mul) f(mulh) f(mulhu) f(mulhsu) f(div) f(divu) f(rem) f(remu) \

#define INSTR_RVC(f) \
  f(c_li) f(c_addi) f(c_slli) f(c_srli) f(c_srai) f(c_andi) \
  f(c_add) f(c_and) f(c_or) f(c_xor) f(c_sub) f(c_mv) \
  f(c_j) f(c_jal) f(c_jr) \
  f(c_beqz) f(c_bnez) \
  f(p_blez) f(p_bgez) f(p_bltz) f(p_bgtz) \
  f(p_li_0) f(p_li_1) f(p_inc) f(p_dec) \
  f(p_ret)

#define INSTR_PRIV(f) f(csrrw) f(csrrs) f(ecall) f(mret) f(sfence_vma)
#define INSTR_SPECIAL(f) f(inv) f(nemu_trap)

#define INSTR_LIST(f) \
  INSTR_RVI(f) \
  INSTR_RVM(f) \
  INSTR_RVC(f) \
  INSTR_PRIV(f) \
  INSTR_SPECIAL(f)

#else
#define INSTR_LIST(f) f(lui) f(lw) f(sw) f(inv) f(nemu_trap)
#endif

def_all_EXEC_ID();
