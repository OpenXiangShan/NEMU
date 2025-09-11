#ifndef __EXT_MSYNC_H__
#define __EXT_MSYNC_H__

#include <common.h>

#ifdef CONFIG_RVMATRIX

// Attention: the order of fields in this class must be the same as the order
//            in the corresponding struct in XiangShan(or XSAI)'s diffstate.h.
typedef struct  {
  uint8_t  valid;
  uint8_t  op;
  uint8_t  token;
  uint64_t pc;
} msync_event_t;

int check_msync(msync_event_t *cmp);
msync_event_t get_msync_info();

#endif // CONFIG_RVMATRIX
#endif // __EXT_MSYNC_H__