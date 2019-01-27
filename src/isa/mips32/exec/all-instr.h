#include "cpu/exec.h"

make_EHelper(lui);
make_EHelper(add);
make_EHelper(sub);
make_EHelper(slt);
make_EHelper(sltu);
make_EHelper(and);
make_EHelper(or);
make_EHelper(xor);
make_EHelper(nor);
make_EHelper(sll);
make_EHelper(srl);
make_EHelper(sra);
make_EHelper(movn);
make_EHelper(movz);

make_EHelper(ld);
make_EHelper(lds);
make_EHelper(st);
make_EHelper(swl);
make_EHelper(swr);
make_EHelper(lwl);
make_EHelper(lwr);

make_EHelper(mfhi);
make_EHelper(mflo);
make_EHelper(mthi);
make_EHelper(mtlo);
make_EHelper(mul);
make_EHelper(mult);
make_EHelper(multu);
make_EHelper(div);
make_EHelper(divu);

make_EHelper(j);
make_EHelper(jal);
make_EHelper(jr);
make_EHelper(jalr);
make_EHelper(bne);
make_EHelper(beq);
make_EHelper(blez);
make_EHelper(bltz);
make_EHelper(bgtz);
make_EHelper(bgez);

make_EHelper(inv);
make_EHelper(nemu_trap);

make_EHelper(syscall);
make_EHelper(eret);
make_EHelper(mfc0);
make_EHelper(mtc0);
make_EHelper(tlbwr);
make_EHelper(tlbwi);
make_EHelper(tlbp);
