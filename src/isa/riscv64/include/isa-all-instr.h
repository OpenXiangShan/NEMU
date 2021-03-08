#include <cpu/decode.h>
#include <rtl/rtl.h>

#define INSTR_NULLARY(f) \
  f(inv) f(nemu_trap) f(ecall) f(mret) f(sret) f(sfence_vma) f(wfi) f(fence) f(ret)
#define INSTR_UNARY(f) \
  f(li_0) f(li_1)
#define INSTR_BINARY(f) \
  f(lui) f(auipc) f(jal) \
  f(ld) f(lw) f(lh) f(lb) f(lwu) f(lhu) f(lbu) f(sd) f(sw) f(sh) f(sb) \
  f(mv) f(sext_w) f(j) f(jal_ra) f(jr)
#define INSTR_TERNARY(f) \
  f(add) f(sll) f(srl) f(slt) f(sltu) f(xor) f(or) f(sub) f(sra) f(and) \
  f(addi) f(slli) f(srli) f(slti) f(sltui) f(xori) f(ori) f(srai) f(andi) \
  f(addw) f(sllw) f(srlw) f(subw) f(sraw) \
  f(addiw) f(slliw) f(srliw) f(sraiw) \
  f(jalr) f(jr_imm) f(beq) f(bne) f(blt) f(bge) f(bltu) f(bgeu) \
  f(mul) f(mulh) f(mulhu) f(mulhsu) f(div) f(divu) f(rem) f(remu) \
  f(mulw) f(divw) f(divuw) f(remw) f(remuw) \
  f(csrrw) f(csrrs) f(csrrc) f(csrrwi) f(csrrsi) f(csrrci) \
  f(li) f(beqz) f(bnez) f(blez) f(bgez) f(bltz) f(bgtz)

def_all_EXEC_ID();
