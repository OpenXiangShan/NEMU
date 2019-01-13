#include "cpu/exec.h"

make_EHelper(lui);
make_EHelper(add);
make_EHelper(sltu);
make_EHelper(or);
make_EHelper(xor);
make_EHelper(sll);

make_EHelper(load);
make_EHelper(store);

make_EHelper(j);
make_EHelper(jal);
make_EHelper(jr);
make_EHelper(bne);
make_EHelper(beq);

make_EHelper(inv);
make_EHelper(nemu_trap);
