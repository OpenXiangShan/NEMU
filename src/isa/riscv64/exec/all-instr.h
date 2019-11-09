#include "cpu/exec.h"

make_EHelper(add);
make_EHelper(sub);
make_EHelper(sll);
make_EHelper(srl);
make_EHelper(sra);
make_EHelper(slt);
make_EHelper(sltu);
make_EHelper(xor);
make_EHelper(or);
make_EHelper(and);
make_EHelper(auipc);
make_EHelper(lui);

make_EHelper(ld);
make_EHelper(lds);
make_EHelper(st);

make_EHelper(jal);
make_EHelper(jalr);
make_EHelper(branch);
make_EHelper(beq);
make_EHelper(bne);

make_EHelper(inv);
make_EHelper(nemu_trap);
make_EHelper(fence);

make_EHelper(csrrw);
make_EHelper(csrrs);
make_EHelper(csrrc);
make_EHelper(priv);

make_EHelper(mul);
make_EHelper(mulh);
make_EHelper(mulhsu);
make_EHelper(mulhu);
make_EHelper(div);
make_EHelper(divu);
make_EHelper(rem);
make_EHelper(remu);

make_EHelper(addw);
make_EHelper(subw);
make_EHelper(sllw);
make_EHelper(mulw);
make_EHelper(divw);
make_EHelper(remw);
make_EHelper(divuw);
make_EHelper(remuw);
make_EHelper(sraw);
make_EHelper(srlw);

make_EHelper(lr);
make_EHelper(sc);
make_EHelper(amoswap);
make_EHelper(amoadd);
make_EHelper(amoor);
