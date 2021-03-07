#include <cpu/decode.h>
#include <rtl/rtl.h>

#define INSTR_NULLARY(f) \
  f(inv) f(nemu_trap) f(ecall) f(sret) f(sfence_vma) f(ret)
#define INSTR_UNARY(f) \
  f(li_0) f(li_1)
#define INSTR_BINARY(f) \
  f(lui) f(auipc) f(jal) \
  f(lw) f(sw) f(lh) f(lb) f(lhu) f(lbu) f(sh) f(sb) \
  f(mv) f(j) f(jal_ra) f(jr)
#define INSTR_TERNARY(f) \
  f(add) f(sll) f(srl) f(slt) f(sltu) f(xor) f(or) f(sub) f(sra) f(and) \
  f(addi) f(slli) f(srli) f(slti) f(sltui) f(xori) f(ori) f(srai) f(andi) \
  f(jalr) f(jr_imm) f(beq) f(bne) f(blt) f(bge) f(bltu) f(bgeu) \
  f(mul) f(mulh) f(mulhu) f(mulhsu) f(div) f(divu) f(rem) f(remu) \
  f(csrrw) f(csrrs) \
  f(li) f(beqz) f(bnez) f(blez) f(bgez) f(bltz) f(bgtz)

def_all_EXEC_ID();
