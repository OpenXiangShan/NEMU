#include <rtl/rtl.h>
#include "rv_ins_def.h"
#include "../tran.h"

void rv64_relop(uint32_t relop, uint32_t idx_dest, uint32_t idx_src1, uint32_t idx_src2) {
  switch (relop) {
    case RELOP_FALSE: rv64_addi(idx_dest, x0, 0); return;
    case RELOP_TRUE:  rv64_addi(idx_dest, x0, 1); return;
    case RELOP_EQ:
      rv64_xor(idx_dest, idx_src1, idx_src2);
      rv64_sltu(idx_dest, x0, idx_dest);
      rv64_xori(idx_dest, idx_dest, 1);
      return;
    case RELOP_NE:
      rv64_xor(idx_dest, idx_src1, idx_src2);
      rv64_sltu(idx_dest, x0, idx_dest);
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
