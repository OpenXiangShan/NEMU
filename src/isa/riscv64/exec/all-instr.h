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
make_EHelper(amoand);
make_EHelper(amomaxu);
make_EHelper(amoxor);

make_EHelper(fp_ld);
make_EHelper(fp_st);
make_EHelper(fadd);
make_EHelper(fsub);
make_EHelper(fmul);
make_EHelper(fdiv);
make_EHelper(fsgnj);
make_EHelper(fmin_max);
make_EHelper(fmv_F_to_G); // fmv.x.w, fmv.x.d fclass
make_EHelper(fmv_G_to_F);
make_EHelper(fcmp);
make_EHelper(fsqrt);
make_EHelper(fcvt_F_to_G);
make_EHelper(fcvt_G_to_F);
make_EHelper(fcvt_F_to_F);
make_EHelper(fmadd);
make_EHelper(fnmadd);
make_EHelper(fmsub);
make_EHelper(fnmsub);