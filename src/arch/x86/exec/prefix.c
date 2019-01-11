#include "cpu/exec.h"

make_EHelper(real);

make_EHelper(operand_size) {
  decinfo.arch.is_operand_size_16 = true;
  exec_real(eip);
  decinfo.arch.is_operand_size_16 = false;
}
