#include "cpu/cpu.h"
#include "cpu/decode.h"

#include <isa/riscv64/instr/rvv/vreg_impl.h>
#include "isa/riscv64/instr/rvv/vcommon.h"


// if we decode some information in decode stage
// when running in opt mode, these information will not be generated because
// it only runs the exec functions
void predecode_vls(Decode *s) {
#ifdef CONFIG_RVV
  const int table [8] = {1, 0, 0, 0, 0, 2, 4, 8};
  s->vm = s->isa.instr.v_opv.v_vm; //1 for without mask; 0 for with mask
  s->v_width = table[s->isa.instr.vldfp.v_width];
  if (s->v_width == 0) {
    // reserved
    longjmp_exception(EX_II);
  }
  s->v_nf = s->isa.instr.vldfp.v_nf;
  s->v_lsumop = s->isa.instr.vldfp.v_lsumop;
#endif
}

int get_mode(Decode *s) {
  /*
   * mode 0: rs1 != 0, Normal stripmining
   * mode 1: rd != 0, rs1 == 0, Set vl to VLMAX
   * mode 2: rd == 0, rs1 == 0, Keep existing vl
   */
  int mode = 0;
  if (id_src1->reg == 0 && id_dest->reg != 0) {
    mode = 1;
  }
  else if (id_src1->reg == 0 && id_dest->reg == 0) {
    mode = 2;
  }
  return mode;
}

void set_vtype_vl(Decode *s, int mode) {
  rtlreg_t vl_num = check_vsetvl(id_src2->val, id_src1->val, mode);
  rtlreg_t error = 1ul << 63;
  
  if(vl_num == (uint64_t)-1 || check_vlmul_sew_illegal(id_src2->val)) {
    vtype->val = error;
    // if vtype illegal, set vl = 0, vd = 0
    vl->val = 0;
  }
  else {
    vtype->val = id_src2->val;
    vl->val = vl_num;
  }

  rtl_sr(s, id_dest->reg, &vl->val, 8);

  vstart->val = 0;
}
