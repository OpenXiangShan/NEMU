#include <cpu/decode.h>
#include <ext/amu_ctrl_queue_wrapper.h>
#include <ext/amuctrl.h>

#ifdef CONFIG_SHARE_CTRL

extern void (*amu_ctrl_callback_)(amu_ctrl_event_t);
extern Decode *prev_s;

void cutest_mma_emplace(uint8_t md, bool sat, bool isfp,
                                uint8_t ms1, uint8_t ms2, uint16_t mtilem,
                                uint16_t mtilen, uint16_t mtilek, uint8_t types1,
                                uint8_t types2, uint8_t typed) {
  amu_ctrl_event_t event = {0};
  event.valid = true;
  event.op = 0;
  event.md = md;
  event.sat = sat;
  event.isfp = isfp;
  event.ms1 = ms1;
  event.ms2 = ms2;
  event.mtilem = mtilem;
  event.mtilen = mtilen;
  event.mtilek = mtilek;
  event.types1 = types1;
  event.types2 = types2;
  event.typed = typed;
  event.pc = prev_s->pc;
  amu_ctrl_callback_(event);
}

void cutest_mls_emplace(uint8_t ms, bool ls, bool transpose, bool isacc,
                                bool isA, uint64_t base, uint64_t stride, uint16_t row,
                                uint16_t column, uint8_t msew) {
  amu_ctrl_event_t event = {0};
  event.valid = true;
  event.op = 1;
  event.md = ms;
  event.sat = ls;
  event.base = base;
  event.stride = stride;
  event.isfp = transpose;
  event.types1 = isacc;
  event.types2 = isA;
  event.mtilem = row;
  event.mtilen = column;
  event.typed = msew;
  event.pc = prev_s->pc;
  amu_ctrl_callback_(event);
}

void cutest_mrelease_emplace(uint8_t msyncRd) {
  amu_ctrl_event_t event = {0};
  event.valid = true;
  event.op = 2;
  event.mtilem = msyncRd;
  event.pc = prev_s->pc;
  amu_ctrl_callback_(event);
}

void cutest_mzero_emplace(bool isacc, uint8_t md) {
  amu_ctrl_event_t event = {0};
  event.valid = true;
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
