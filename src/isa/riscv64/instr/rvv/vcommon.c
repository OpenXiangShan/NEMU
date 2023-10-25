#include <common.h>
#ifdef CONFIG_RVV

#include <math.h>
#include "vcommon.h"
uint8_t check_vstart_ignore(Decode *s) {
  if(vstart->val >= vl->val) {
    if(vstart->val > 0) {
      rtl_li(s, s0, 0);
      vcsr_write(IDXVSTART, s0);
      set_mstatus_dirt();
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

#endif