#include <common.h>
#ifdef CONFIG_RVV

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

#endif