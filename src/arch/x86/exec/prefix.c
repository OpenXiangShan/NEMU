#include "cpu/exec.h"

make_EHelper(arch);

make_EHelper(operand_size) {
  decinfo.arch.is_operand_size_16 = true;
  exec_arch(eip);
  decinfo.arch.is_operand_size_16 = false;
}
