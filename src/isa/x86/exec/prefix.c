#include "cpu/exec.h"

make_EHelper(isa);

make_EHelper(operand_size) {
  decinfo.isa.is_operand_size_16 = true;
  exec_isa(eip);
  decinfo.isa.is_operand_size_16 = false;
}
