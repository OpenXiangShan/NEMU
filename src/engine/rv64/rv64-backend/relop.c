#include <common.h>
#include <rtl/rtl.h>
#include "rv_ins_def.h"

uint8_t reg_ptr2idx(DecodeExecState *s, const rtlreg_t* dest);

void rv64_relop(DecodeExecState *s, uint32_t relop, 
    const rtlreg_t* dest, const rtlreg_t* src1, const rtlreg_t* src2) {
  uint8_t idx_dest = reg_ptr2idx(s, dest);
  uint8_t idx_src1 = reg_ptr2idx(s, src1);
  uint8_t idx_src2 = reg_ptr2idx(s, src2);
  switch (relop) {
    case RELOP_FALSE: rv64_addi(idx_dest, x0, 0); return;
    case RELOP_TRUE:  rv64_addi(idx_dest, x0, 1); return;
    case RELOP_EQ:
      rv64_xor(x31, idx_src1, idx_src2);
      rv64_sltu(idx_dest, x0, x31);
      rv64_xori(idx_dest, idx_dest, 1);
      return;
    case RELOP_NE:
      rv64_xor(x31, idx_src1, idx_src2);
      rv64_sltu(idx_dest, x0, x31);
      return;
    case RELOP_LT: rv64_slt(idx_dest, idx_src1, idx_src2); return;
    case RELOP_LE:
      rv64_slt(idx_dest, idx_src2, idx_src1);
      rv64_xori(idx_dest, idx_dest, 1);
      return;
    case RELOP_GT: rv64_slt(idx_dest, idx_src2, idx_src1); return;
    case RELOP_GE:
      rv64_slt(idx_dest, idx_src1, idx_src2);
      rv64_xori(idx_dest, idx_dest, 1);
      return;
    case RELOP_LTU: rv64_sltu(idx_dest, idx_src1, idx_src2); return;
    case RELOP_LEU:
      rv64_sltu(idx_dest, idx_src2, idx_src1);
      rv64_xori(idx_dest, idx_dest, 1);
      return;
    case RELOP_GTU: rv64_sltu(idx_dest, idx_src2, idx_src1); return;
    case RELOP_GEU:
      rv64_sltu(idx_dest, idx_src1, idx_src2);
      rv64_xori(idx_dest, idx_dest, 1);
      return;
    default: panic("unsupport relop = %d", relop);
  }
}
