#include <common.h>
#ifdef CONFIG_RVV

#include <math.h>
#include "vcommon.h"
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

bool check_vlmul_sew_illegal(rtlreg_t vtype_req){
  vtype_t vt = (vtype_t )vtype_req;
  int vlmul = vt.vlmul;
  int vsew = vt.vsew;
  if (vlmul > 4) vlmul -= 8;
  if((vlmul < vsew + 3 - log2(MAXELEN)) || vlmul == 4) return true; // vmul < sew/ELEN || vlmul == 100
  return false;
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