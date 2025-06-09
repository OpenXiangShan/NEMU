#include <common.h>
#ifdef CONFIG_RVV

#include <math.h>
#include "vcommon.h"
#include <cpu/cpu.h>

uint8_t check_vstart_ignore(Decode *s) {
  if(vstart->val >= vl->val) {
    if(vstart->val > 0) {
      rtl_li(s, s0, 0);
      vcsr_write(IDXVSTART, s0);
      vp_set_dirty();
    }
    return 1;
  }
  return 0;
}

void check_vstart_exception(Decode *s) {
  if(vstart->val > 0) {
    longjmp_exception(EX_II);
  }
}

bool check_vlmul_sew_illegal(rtlreg_t vtype_req) {
  vtype_t vt;
  vt.val = vtype_req;
  int vlmul = vt.vlmul;
  if (vlmul > 4) vlmul -= 8;
  int vsew = 8 << vt.vsew;
  float vflmul = vlmul >= 0 ? 1 << vlmul : 1.0 / (1 << -vlmul);
  float min_vflmul = vflmul < 1.0f ? vflmul : 1.0f;
  int vill = !(vflmul >= 0.125 && vflmul <= 8)
           || vsew > min_vflmul * 64
           || (vtype_req >> 8) != 0
           || vsew > 64;
  return vill == 1;
}

void set_NAN(rtlreg_t* fpreg, uint64_t vsew){
  switch (vsew) {
    case 0:
      *fpreg = (*fpreg & 0xffffffffffffff00) | 0x78;
      break;
    case 1:
      *fpreg = (*fpreg & 0xffffffffffff0000) | 0x7e00;
      break;
    case 2:
      *fpreg = (*fpreg & 0xffffffff00000000) | 0x7fc00000;
      break;
    case 3:
      break;
    default:
      break;
  }
}

bool check_isFpCanonicalNAN(rtlreg_t* fpreg, uint64_t vsew){
  int isFpCanonicalNAN = 0;
  switch (vsew) {
    case 0:
      isFpCanonicalNAN = ~(*fpreg | 0xff) != 0;
      if(isFpCanonicalNAN) set_NAN(fpreg, vsew);
      break;
    case 1:
      isFpCanonicalNAN = ~(*fpreg | 0xffff) != 0;
      if(isFpCanonicalNAN) set_NAN(fpreg, vsew);
      break;
    case 2:
      isFpCanonicalNAN = ~(*fpreg | 0xffffffff) != 0;
      if(isFpCanonicalNAN) set_NAN(fpreg, vsew);
      break;
    case 3:
      break;
    default:
      break;
  }
  return isFpCanonicalNAN;
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

// if we decode some information in decode stage
// when running in opt mode, these information will not be generated because
// it only runs the exec functions
void predecode_vls(Decode *s) {
#ifdef CONFIG_RVV
  cpu.isVldst = true;
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

#endif