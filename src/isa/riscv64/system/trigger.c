#include "isa.h"
#include "cpu/cpu.h"
#include "../local-include/trigger.h"
#include "../local-include/csr.h"
#include "../local-include/intr.h"

#ifdef CONFIG_RV_SDTRIG

void tm_update_timings(struct TriggerModule* TM) {
  TM->check_timings.val = 0;
  for (int i = 0; i < CONFIG_TRIGGER_NUM; i++) {
    tdata1_t* tdata1 = (tdata1_t*)&TM->triggers[i].tdata1.val;
    if (tdata1->type != TRIG_TYPE_MCONTROL6)
      continue;
    trig_mcontrol6_t* mcontrol6 = (trig_mcontrol6_t*)tdata1;


    /* Table 15. Suggested Trigger Timing in debug specification. Don't care data(select = true) match. 
    *  | Match   | Type Suggested Trigger Timing |
    *  | Execute | Address Before                |
    *  | Execute | Instruction Before            |
    *  | Execute | Address + Instruction Before  |
    *  | Load    | Address Before                |
    *  | Load    | Data After                    |
    *  | Load    | Address + Data After          |
    *  | Store   | Address Before                |
    *  | Store   | Data Before                   |
    *  | Store   | Address + Data Before         |
    * 
    * For mcontrol6 you can't request a timing. Default to before since that's
    * most useful to the user.
    */
    TM->check_timings.bf |= mcontrol6->execute;
    TM->check_timings.br |= mcontrol6->load && !mcontrol6->select;
    TM->check_timings.ar |= mcontrol6->load && mcontrol6->select;
    TM->check_timings.bw |= mcontrol6->store;
  }
}

trig_action_t tm_check_hit(
  struct TriggerModule* TM,
  trig_op_t op,
  vaddr_t addr,
  word_t data
) {
#ifdef CONFIG_RV_SDEXT
  // do nothing in debug mode
  if (cpu.debug_mode)
    return TRIG_ACTION_NONE;
#endif
  // check mcontrol6
  // Action can be taken only when all triggers on the chain are hit.
  /* GDB doesn't support setting triggers in a way that combines a data load trigger
   * with an address trigger to trigger on a load of a value at a given address.
   * The default timing legalization on mcontrol6 assumes no such trigger setting. */
  const int trigger_num = CONFIG_TRIGGER_NUM;
  bool chain_ok[trigger_num];
  bool hit[trigger_num];
  bool can_fire[trigger_num];
  memset(chain_ok, true, sizeof(chain_ok));

  for (int i = 0; i < trigger_num; i++) {
    bool match = false;
    if (TM->triggers[i].tdata1.common.type == TRIG_TYPE_MCONTROL6){
      match = trigger_match(&TM->triggers[i], op, addr, data);
    }
    hit[i] = match;
  }

  for (int i = 1; i < trigger_num; i++) {
    bool last_hit = hit[i-1];
    bool last_chain = TM->triggers[i - 1].tdata1.mcontrol6.chain;
    chain_ok[i] = last_hit || (!last_hit && !last_chain);
  }

  for (int i = 0; i < trigger_num; i++) {
    bool this_chain = TM->triggers[i].tdata1.mcontrol6.chain;
    bool this_hit = hit[i];
    can_fire[i] = chain_ok[i] && this_hit && !this_chain;
    if (can_fire[i])
      return (trig_action_t)TM->triggers[i].tdata1.mcontrol6.action;
  }

  return TRIG_ACTION_NONE;
}

bool trigger_match(Trigger* trig, trig_op_t op, vaddr_t addr, word_t data) {
  // not meet trigger condition
  if (((op & TRIG_OP_EXECUTE)       && !trig->tdata1.mcontrol6.execute) ||
      ((op & TRIG_OP_LOAD)          && !trig->tdata1.mcontrol6.load) ||
      ((op & TRIG_OP_STORE)         && !trig->tdata1.mcontrol6.store) ||
      ((op & TRIG_OP_TIMING)        && !false) ||
      (cpu.mode == MODE_M           && !(trig->tdata1.mcontrol6.m && tcontrol->mte)) ||
      (cpu.mode == MODE_S           && !trig->tdata1.mcontrol6.s) ||
      (cpu.mode == MODE_U           && !trig->tdata1.mcontrol6.u) ||
#ifdef CONFIG_RVH
      (cpu.mode == MODE_S && cpu.v  && !trig->tdata1.mcontrol6.vs) ||
      (cpu.mode == MODE_U && cpu.v  && !trig->tdata1.mcontrol6.vu)
#endif // CONFIG_RVH
      ) {
    return false;
  }
  word_t value = trig->tdata1.mcontrol6.select ? data : addr;

  if (trigger_value_match(trig, value)) {
    return true;
  }
  return false;
}

bool trigger_value_match(Trigger* trig, word_t value) {
  word_t tdata2_val = trig->tdata2.val;
  switch (trig->tdata1.mcontrol6.match)
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
    default : panic("Unsupported trigger match id %d", trig->tdata1.mcontrol6.match);
  }
}

bool trigger_check_chain_legal(const struct TriggerModule* TM, const int max_chain_len) {
  int chain_len = 0;
  int i = 0;
  while (i < CONFIG_TRIGGER_NUM && chain_len < max_chain_len) {
    if (TM->triggers[i].tdata1.mcontrol6.chain) {
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

void mcontrol6_checked_write(trig_mcontrol6_t* mcontrol6, word_t* wdata, const struct TriggerModule* TM) {
  trig_mcontrol6_t* wdata_mcontrol6 = (trig_mcontrol6_t*)wdata;
  mcontrol6->type = TRIG_TYPE_MCONTROL6;
  mcontrol6->dmode = 0;
  mcontrol6->pad1 = 0;
  mcontrol6->uncertain = 0;
  mcontrol6->hit1 = 0;
  mcontrol6->vs = wdata_mcontrol6->vs;
  mcontrol6->vu = wdata_mcontrol6->vu;
  mcontrol6->hit0 = 0;
  mcontrol6->select = wdata_mcontrol6->select;
  mcontrol6->pad0 = 0;
  mcontrol6->size = 0;
  mcontrol6->action = wdata_mcontrol6->action <= TRIG_ACTION_DEBUG_MODE ? wdata_mcontrol6->action : TRIG_ACTION_BKPT_EXCPT;
  mcontrol6->chain = wdata_mcontrol6->chain; // chain length will be checked later
  mcontrol6->match = (wdata_mcontrol6->match == TRIG_MATCH_EQ || wdata_mcontrol6->match == TRIG_MATCH_GE || wdata_mcontrol6->match == TRIG_MATCH_LT)
    ? wdata_mcontrol6->match : TRIG_MATCH_EQ;
  mcontrol6->m = wdata_mcontrol6->m;
  mcontrol6->uncertainen = 0;
  mcontrol6->s = wdata_mcontrol6->s;
  mcontrol6->u = wdata_mcontrol6->u;
  mcontrol6->execute = wdata_mcontrol6->execute;
  mcontrol6->store = wdata_mcontrol6->store;
  mcontrol6->load = wdata_mcontrol6->load;
  mcontrol6->chain = trigger_check_chain_legal(TM, 2) && wdata_mcontrol6->chain;
}

void trigger_handler(const trig_action_t action) {
  switch (action) {
    case TRIG_ACTION_NONE: /* no trigger hit, do nothing */; break;
    case TRIG_ACTION_BKPT_EXCPT: longjmp_exception(EX_BP); break;
    default: panic("Unsupported trigger action %d", action);  break;
  }
}

#endif //CONFIG_RV_SDTRIG