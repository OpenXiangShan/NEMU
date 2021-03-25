#include "../local-include/rtl.h"
#include <cpu/ifetch.h>
#include <cpu/decode.h>
#include <isa-all-instr.h>

def_all_THelper();

// decode operand helper
#define def_DopHelper(name) \
  void concat(decode_op_, name) (Decode *s, Operand *op, uint64_t val, bool flag)

#include "rvi/decode.h"
#include "rvf/decode.h"
#include "rvm/decode.h"
#include "rva/decode.h"
#include "rvc/decode.h"
#include "rvd/decode.h"
#include "priv/decode.h"

def_THelper(main) {
  switch (s->isa.instr.i.opcode6_2) {
    IDTAB(000, I, load)   IDTAB(001, fload, fload)                         IDTAB(003, I, mem_fence)
    IDTAB(004, I, op_imm) IDTAB(005, auipc, auipc)      IDTAB(006, I, op_imm32)
    IDTAB(010, S, store)  IDTAB(011, fstore, fstore)                       IDTAB(013, R, atomic)
    IDTAB(014, R, op)     IDTAB(015, U, lui)            IDTAB(016, R, op32)
    IDTAB(020, R4, fmadd_dispatch) IDTAB(021, R4, fmadd_dispatch)
    IDTAB(022, R4, fmadd_dispatch) IDTAB(023, R4, fmadd_dispatch)
    TAB  (024, op_fp)
    IDTAB(030, B, branch) IDTAB(031, I, jalr_dispatch)  TAB  (032, nemu_trap)  IDTAB(033, J, jal_dispatch)
    IDTAB(034, csr, system)
    //IDTAB(036, R, rocc3)
    IDTAB(036, R, rt_inv)
  }
  return table_inv(s);
};

int isa_fetch_decode(Decode *s) {
  int idx = EXEC_ID_inv;

  if ((s->pc & 0xfff) == 0xffe) {
    // instruction may accross page boundary
    uint32_t lo = instr_fetch(&s->snpc, 2);
    s->isa.instr.val = lo & 0xffff;
    if (s->isa.instr.r.opcode1_0 != 0x3) {
      // this is an RVC instruction
      goto rvc;
    }
    // this is a 4-byte instruction, should fetch the MSB part
    // NOTE: The fetch here may cause IPF.
    // If it is the case, we should have mepc = xxxffe and mtval = yyy000.
    // Refer to `mtval` in the privileged manual for more details.
    uint32_t hi = instr_fetch(&s->snpc, 2);
    s->isa.instr.val |= ((hi & 0xffff) << 16);
  } else {
    // in-page instructions, fetch 4 byte and
    // see whether it is an RVC instruction later
    s->isa.instr.val = instr_fetch(&s->snpc, 4);
  }

  if (s->isa.instr.r.opcode1_0 == 0x3) {
    idx = table_main(s);
  } else {
    // RVC instructions are only 2-byte
    s->snpc -= 2;
rvc: idx = table_rvc(s);
  }

  s->type = INSTR_TYPE_N;
  switch (idx) {
    case EXEC_ID_c_j: case EXEC_ID_p_jal: case EXEC_ID_jal:
      s->jnpc = id_src1->imm; s->type = INSTR_TYPE_J; break;

    case EXEC_ID_beq: case EXEC_ID_bne: case EXEC_ID_blt: case EXEC_ID_bge:
    case EXEC_ID_bltu: case EXEC_ID_bgeu:
    case EXEC_ID_c_beqz: case EXEC_ID_c_bnez:
    case EXEC_ID_p_bltz: case EXEC_ID_p_bgez: case EXEC_ID_p_blez: case EXEC_ID_p_bgtz:
      s->jnpc = id_dest->imm; s->type = INSTR_TYPE_B; break;

    case EXEC_ID_p_ret: case EXEC_ID_c_jr: case EXEC_ID_c_jalr: case EXEC_ID_jalr:
    IFDEF(CONFIG_DEBUG, case EXEC_ID_mret: case EXEC_ID_sret: case EXEC_ID_ecall:)
      s->type = INSTR_TYPE_I; break;

#ifndef CONFIG_DEBUG
    case EXEC_ID_system:
      if (s->isa.instr.i.funct3 == 0) {
        switch (s->isa.instr.csr.csr) {
          case 0:     // ecall
          case 0x102: // sret
          case 0x302: // mret
            s->type = INSTR_TYPE_I;
        }
      }
      break;
#endif
  }

  return idx;
}
