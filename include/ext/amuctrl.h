#ifndef __EXT_AMUCTRL_H__
#define __EXT_AMUCTRL_H__

#include <common.h>

#ifdef CONFIG_RVMATRIX

// Attention: the order of fields in this class must be the same as the order
//            in the corresponding struct in NEMU.
typedef struct  {
  uint8_t  valid;
  uint8_t  op;
  uint8_t  md;     // also used as ms
  uint8_t  sat;    // also used as ls
  uint8_t  ms1;
  uint8_t  ms2;
  uint16_t mtilem; // also used as row
  uint16_t mtilen; // also used as column
  uint16_t mtilek;
  uint8_t  types;
  uint8_t  typed;
  uint8_t  transpose;
  uint8_t  isacc;
  uint64_t base;
  uint64_t stride;
  uint64_t pc;
} amu_ctrl_event_t;

int check_amu_ctrl(amu_ctrl_event_t *cmp);
amu_ctrl_event_t get_amu_ctrl_info();

#endif // CONFIG_RVMATRIX
#endif // __EXT_AMUCTRL_H__