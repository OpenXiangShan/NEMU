#ifdef CONFIG_RVSDTRIG
#include "isa.h"
#include "cpu/cpu.h"
#include "../local-include/trigger.h"
#include "../local-include/csr.h"

void tm_update_timings(struct TriggerModule* TM, tdata1_t tdata1) {
  if (!(tdata1.type == TRIG_TYPE_MCONTROL))
    return;
  trig_mcontrol_t* mcontrol = (trig_mcontrol_t*)&tdata1;
  TM->check_timings.bf = mcontrol->execute  && !mcontrol->timing;
  TM->check_timings.af = mcontrol->execute  && mcontrol->timing;
  TM->check_timings.br = mcontrol->load     && !mcontrol->timing;
  TM->check_timings.ar = mcontrol->load     && mcontrol->timing;
  TM->check_timings.bw = mcontrol->store    && !mcontrol->timing;
}

void tm_check_hit(
  /*out*/ trig_action_t * const action,
  struct TriggerModule* TM,
  trig_op_t op,
  vaddr_t addr,
  word_t data
) {
#ifdef CONFIG_RVSDEXT
  // do nothing in debug mode
  if (cpu.debug_mode)
    return;
#endif

  // check mcontrol
  // Action can be taken only when all triggers on the chain are hit.
  bool chain_ok = true;
  for (int i = 0; i < CONFIG_TRIGGER_NUM; i++) {
    if (TM->triggers[i].tdata1.common.type != TRIG_TYPE_MCONTROL)
      continue;
    if (!chain_ok) {
      chain_ok = chain_ok || !TM->triggers[i].tdata1.mcontrol.chain;
      continue;
    }
    bool match = trigger_match(&TM->triggers[i], op, addr, data);
    TM->triggers[i].tdata1.mcontrol.hit = match;
    if (match && !TM->triggers[i].tdata1.mcontrol.chain) {
      *action = TM->triggers[i].tdata1.mcontrol.action;
      return;
    } 
    chain_ok = match;
  }
}

bool trigger_match(Trigger* trig, trig_op_t op, vaddr_t addr, word_t data) {
  // not neet trigger condition
  if (((op & TRIG_OP_EXECUTE) && !trig->tdata1.mcontrol.execute) ||
      ((op & TRIG_OP_LOAD)    && !trig->tdata1.mcontrol.load) ||
      ((op & TRIG_OP_STORE)   && !trig->tdata1.mcontrol.store) ||
      ((op & TRIG_OP_TIMING)  && !trig->tdata1.mcontrol.timing) ||
      (cpu.mode == MODE_M     && !trig->tdata1.mcontrol.m) ||
      (cpu.mode == MODE_S     && !trig->tdata1.mcontrol.s) ||
      (cpu.mode == MODE_U     && !trig->tdata1.mcontrol.u)) {
    return false;
  }
  word_t value = trig->tdata1.mcontrol.select ? data : addr;

  if (trigger_value_match(trig, value)) {
    return true;
  }
  return false;
}

bool trigger_value_match(Trigger* trig, word_t value) {
  word_t tdata2_val = trig->tdata2.val;
  switch (trig->tdata1.mcontrol.match)
  {
    case TRIG_MATCH_EQ:
      return value == tdata2_val;
    case TRIG_MATCH_LT:
      return value < tdata2_val;
    case TRIG_MATCH_GE:
      return value >= tdata2_val;
    case TRIG_MATCH_NAPOT: {
      int ones = 0;
      word_t val = tdata2_val;
      // Count number of contiguous 1 bits starting from the LSB
      while (val & 1) {
        val >>= 1;
        ones ++;
      }
      word_t mask = ~(BITMASK(ones + 1));
      return (value & mask) == (tdata2_val & mask);
    }
    case TRIG_MATCH_MASK_LO: {
      word_t mask = tdata2_val >> 32; // xlen / 2
      return (value & mask) == (tdata2_val & mask);
    }
    case TRIG_MATCH_MASK_HI: {
      word_t mask = tdata2_val >> 32; // xlen / 2
      return ((value >> 32) & mask) == (tdata2_val & mask);
    }
    default : panic("Unsupported trigger match id %d", trig->tdata1.mcontrol.match);
  }
}

#endif //CONFIG_RVSDTRIG