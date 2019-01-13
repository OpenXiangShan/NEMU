#include "cpu/exec.h"

make_EHelper(lui);
make_EHelper(add);
make_EHelper(sub);
make_EHelper(slt);
make_EHelper(sltu);
make_EHelper(and);
make_EHelper(or);
make_EHelper(xor);
make_EHelper(sll);
make_EHelper(srl);
make_EHelper(sra);
make_EHelper(movn);
make_EHelper(movz);

make_EHelper(load);
make_EHelper(store);

make_EHelper(mfhi);
make_EHelper(mflo);
make_EHelper(mul);
make_EHelper(mult);
make_EHelper(div);

make_EHelper(j);
make_EHelper(jal);
make_EHelper(jr);
make_EHelper(jalr);
make_EHelper(bne);
make_EHelper(beq);
make_EHelper(blez);

make_EHelper(inv);
make_EHelper(nemu_trap);
