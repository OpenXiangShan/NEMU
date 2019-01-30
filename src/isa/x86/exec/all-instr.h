#include "cpu/exec.h"

make_EHelper(mov);
make_EHelper(push);
make_EHelper(pop);
make_EHelper(leave);
make_EHelper(lea);
make_EHelper(movzx);
make_EHelper(cltd);
make_EHelper(movsx);
make_EHelper(cwtl);
make_EHelper(pusha);
make_EHelper(popa);

make_EHelper(sub);
make_EHelper(cmp);
make_EHelper(add);
make_EHelper(adc);
make_EHelper(imul2);
make_EHelper(idiv);
make_EHelper(inc);
make_EHelper(dec);
make_EHelper(imul1);
make_EHelper(sbb);
make_EHelper(div);
make_EHelper(neg);
make_EHelper(mul);
make_EHelper(imul3);

make_EHelper(xor);
make_EHelper(and);
make_EHelper(test);
make_EHelper(setcc);
make_EHelper(or);
make_EHelper(shl);
make_EHelper(sar);
make_EHelper(not);
make_EHelper(shr);
make_EHelper(rol);
make_EHelper(shld);
make_EHelper(shrd);

make_EHelper(call);
make_EHelper(ret);
make_EHelper(ret_imm);
make_EHelper(jcc);
make_EHelper(jmp);
make_EHelper(call_rm);
make_EHelper(jmp_rm);

make_EHelper(operand_size);

make_EHelper(nop);
make_EHelper(inv);
make_EHelper(nemu_trap);

make_EHelper(mov_cr2r);
make_EHelper(mov_r2cr);
make_EHelper(lidt);
make_EHelper(int);
make_EHelper(iret);
make_EHelper(out);
make_EHelper(in);

make_EHelper(movs);
