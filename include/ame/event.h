#ifndef __AME_EVENT_H__
#define __AME_EVENT_H__

#include <common.h>

#ifdef CONFIG_RV_AME

// The field order is part of the AMU control ABI shared with XSAI.
typedef struct {
  uint8_t  valid;
  uint8_t  op;
  uint8_t  rm;
  uint8_t  md;       // also used as ms
  uint8_t  sat;      // also used as ls
  uint8_t  ms1;
  uint8_t  ms2;
  uint16_t mtilem;   // also used as row
  uint16_t mtilen;   // also used as column
  uint16_t mtilek;
  uint8_t  types1;   // also used as isacc
  uint8_t  types2;
  uint8_t  typed;    // also used as widths
  uint8_t  isfp;     // also used as transpose
  uint64_t base;
  uint64_t stride;
  uint64_t pc;
} amu_ctrl_event_t;

#endif // CONFIG_RV_AME

#endif // __AME_EVENT_H__
