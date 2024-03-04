#include "isa.h"
#include "cpu/cpu.h"
#include "../local-include/trigger.h"
#include "../local-include/csr.h"
#include "../local-include/intr.h"

#ifdef CONFIG_RVSDTRIG

void tm_update_timings(struct TriggerModule* TM) {
  TM->check_timings.val = 0;
  for (int i = 0; i < CONFIG_TRIGGER_NUM; i++) {
    tdata1_t* tdata1 = (tdata1_t*)&TM->triggers[i].tdata1.val;
    if (tdata1->type != TRIG_TYPE_MCONTROL)
      continue;
    trig_mcontrol_t* mcontrol = (trig_mcontrol_t*)tdata1;
    TM->check_timings.bf |= mcontrol->execute  && !mcontrol->timing;
    TM->check_timings.af |= mcontrol->execute  && mcontrol->timing;
    TM->check_timings.br |= mcontrol->load     && !mcontrol->timing;
    TM->check_timings.ar |= mcontrol->load     && mcontrol->timing;
    TM->check_timings.bw |= mcontrol->store    && !mcontrol->timing;
  }
}

trig_action_t tm_check_hit(
  struct TriggerModule* TM,
  trig_op_t op,
  vaddr_t addr,
  word_t data
) {
#ifdef CONFIG_RVSDEXT
  // do nothing in debug mode
  if (cpu.debug_mode)
    return TRIG_ACTION_NONE;
#endif
  // check mcontrol
  // Action can be taken only when all triggers on the chain are hit.
  const int trigger_num = CONFIG_TRIGGER_NUM;
  bool chain_ok[trigger_num];
  bool timing_ok[trigger_num];
  bool can_fire[trigger_num];
  memset(chain_ok, true, sizeof(chain_ok));
  memset(timing_ok, true, sizeof(chain_ok));

  for (int i = 0; i < trigger_num; i++) {
    if (TM->triggers[i].tdata1.common.type != TRIG_TYPE_MCONTROL)
      continue;
    bool match = trigger_match(&TM->triggers[i], op, addr, data);
    TM->triggers[i].tdata1.mcontrol.hit = match;
  }

  bool last_timing =  TM->triggers[0].tdata1.mcontrol.timing;
  for (int i = 1; i < trigger_num; i++) {
    bool last_hit = TM->triggers[i - 1].tdata1.mcontrol.hit;
    bool last_chain = TM->triggers[i - 1].tdata1.mcontrol.chain;
    bool this_timing = TM->triggers[i].tdata1.mcontrol.timing;
    chain_ok[i] = last_hit || (!last_hit && !last_chain);
    timing_ok[i] = last_timing == this_timing;
    last_timing = this_timing;
  }

  for (int i = 0; i < trigger_num; i++) {
    bool this_chain = TM->triggers[i].tdata1.mcontrol.chain;
    bool this_hit = TM->triggers[i].tdata1.mcontrol.hit;
    can_fire[i] = chain_ok[i] && timing_ok[i] && this_hit && !this_chain;
    if (can_fire[i])
      return TM->triggers[i].tdata1.mcontrol.action;
  }

  return TRIG_ACTION_NONE;
}

bool trigger_match(Trigger* trig, trig_op_t op, vaddr_t addr, word_t data) {
  // not meet trigger condition
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

bool trigger_check_chain_legal(const struct TriggerModule* TM, const int max_chain_len) {
  int chain_len = 0;
  int i = 0;
  while (i < CONFIG_TRIGGER_NUM && chain_len < max_chain_len) {
    if (TM->triggers[i].tdata1.mcontrol.chain) {
      chain_len++;
    } else {
      chain_len = 0;
    }
    i++;
  }
  if (!(chain_len < max_chain_len))
    printf("trigger_check_chain_legal returns false\n");
  return chain_len < max_chain_len;
}

void mcontrol_checked_write(trig_mcontrol_t* mcontrol, word_t* wdata, const struct TriggerModule* TM) {
  trig_mcontrol_t* wdata_mcontrol = (trig_mcontrol_t*)wdata;
  mcontrol->type = TRIG_TYPE_MCONTROL;
  mcontrol->dmode = 0;
  mcontrol->maskmax = 0;
  mcontrol->pad1 = 0;
  mcontrol->sizehi = 0;
  mcontrol->hit = 0;
  mcontrol->select = wdata_mcontrol->select;
  mcontrol->timing = wdata_mcontrol->timing;
  mcontrol->sizelo = 0;
  mcontrol->action = wdata_mcontrol->action <= TRIG_ACTION_DEBUG_MODE ? wdata_mcontrol->action : TRIG_ACTION_BKPT_EXCPT;
  mcontrol->chain = wdata_mcontrol->chain; // chain length will be checked later
  mcontrol->match = (wdata_mcontrol->match == TRIG_MATCH_EQ || wdata_mcontrol->match == TRIG_MATCH_GE || wdata_mcontrol->match == TRIG_MATCH_LT)
    ? wdata_mcontrol->match : TRIG_MATCH_EQ;
  mcontrol->m = wdata_mcontrol->m;
  mcontrol->pad0 = 0;
  mcontrol->s = wdata_mcontrol->s;
  mcontrol->u = wdata_mcontrol->u;
  mcontrol->execute = wdata_mcontrol->execute;
  mcontrol->store = wdata_mcontrol->store;
  mcontrol->load = wdata_mcontrol->load;
  mcontrol->chain = trigger_check_chain_legal(TM, 2) && wdata_mcontrol->chain;
}

void trigger_handler(const trig_action_t action) {
  switch (action) {
    case TRIG_ACTION_NONE: /* no trigger hit, do nothing */; break;
    case TRIG_ACTION_BKPT_EXCPT: longjmp_exception(EX_BP); break;
    default: panic("Unsupported trigger action %d", action);  break;
  }
}

#endif //CONFIG_RVSDTRIG