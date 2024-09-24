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

#endif