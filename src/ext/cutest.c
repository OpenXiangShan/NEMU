#include <cpu/decode.h>
#include <ext/amu_ctrl_queue_wrapper.h>
#include <ext/amuctrl.h>

#ifdef CONFIG_SHARE_CTRL

extern void (*amu_ctrl_callback_)(amu_ctrl_event_t);
extern Decode *prev_s;

void cutest_mma_emplace(uint8_t md, bool sat, bool isfp, bool issigned,
                                uint8_t ms1, uint8_t ms2, uint16_t mtilem,
                                uint16_t mtilen, uint16_t mtilek, uint8_t types,
                                uint8_t typed) {
  amu_ctrl_event_t event;
  event.valid = true;
  event.op = 0;
  event.md = md;
  event.sat = sat;
  event.isfp = isfp;
  event.issigned = issigned;
  event.ms1 = ms1;
  event.ms2 = ms2;
  event.mtilem = mtilem;
  event.mtilen = mtilen;
  event.mtilek = mtilek;
  event.types = types;
  event.typed = typed;
  event.pc = prev_s->pc;
  amu_ctrl_callback_(event);
}

void cutest_mls_emplace(uint8_t ms, bool ls, bool transpose, bool isacc,
                                uint64_t base, uint64_t stride, uint16_t row,
                                uint16_t column, uint8_t msew) {
  amu_ctrl_event_t event;
  event.valid = true;
  event.op = 1;
  event.md = ms;
  event.sat = ls;
  event.base = base;
  event.stride = stride;
  event.isfp = transpose;
  event.issigned = isacc;
  event.mtilem = row;
  event.mtilen = column;
  event.types = msew;
  event.pc = prev_s->pc;
  amu_ctrl_callback_(event);
}

void cutest_mrelease_emplace(uint8_t tokenRd) {
  amu_ctrl_event_t event;
  event.op = 2;
  event.mtilem = tokenRd;
  event.pc = prev_s->pc;
  amu_ctrl_callback_(event);
}

void cutest_mzero_emplace(bool isacc, uint8_t md) {
  amu_ctrl_event_t event;
  event.op = 3;
  event.md = md;
  if (isacc) {
    event.base = 0x1bc;
  } else {
    event.base = 0x1b8;
  }
  event.pc = prev_s->pc;
  amu_ctrl_callback_(event);
}

#endif // CONFIG_SHARE_CTRL