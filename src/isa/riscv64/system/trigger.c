#include "isa.h"
#include "cpu/cpu.h"
#include "../local-include/trigger.h"
#include "../local-include/csr.h"
#include "../local-include/intr.h"

#ifdef CONFIG_RV_SDTRIG

trig_action_t trigger_action = TRIG_ACTION_NONE;
word_t triggered_tval;

void trigger_mark_state_dirty(TriggerModule* TM) {
  TM->mcontrol6_state_dirty = true;
}

static inline void refresh_mcontrol6_state(TriggerModule* TM) {
  uint8_t active_count = 0;

  for (int i = 0; i < CONFIG_TRIGGER_NUM; i++) {
    active_count += TM->triggers[i].tdata1.common.type == TRIG_TYPE_MCONTROL6;
  }

  TM->mcontrol6_active_count = active_count;
  TM->mcontrol6_state_dirty = false;
}

#ifdef CONFIG_RV_SDTRIG_TDATA3
static bool tdata3_smatch(Trigger* trig) {
  switch (trig->tdata3.sselect) {
    case 0:
      return true;
    case 1: {
      uint32_t scontext_val = (uint32_t)(scontext->val & 0xffffffffULL);
      uint32_t svalue = trig->tdata3.svalue;
      uint32_t sbytemask = trig->tdata3.sbytemask;
      for (int i = 0; i < 4; i++) {
        if ((sbytemask >> i) & 0x1) {
          continue;
        }
        if (((scontext_val >> (i * 8)) & 0xff) != ((svalue >> (i * 8)) & 0xff)) {
          return false;
        }
      }
      return true;
    }
    case 2: {
      word_t current_asid =
#ifdef CONFIG_RVH
        cpu.v ? vsatp->asid :
#endif
        satp->asid;
      return current_asid == (trig->tdata3.svalue & BITMASK(CONFIG_RV_ASID_LEN));
    }
    default:
      return false;
  }
}

static bool tdata3_mhmatch(Trigger* trig) {
#ifdef CONFIG_RVH
  uint64_t high_sel_value = ((uint64_t)trig->tdata3.mhvalue << 1) |
    ((trig->tdata3.mhselect >> 2) & 0x1);
#endif

  switch (trig->tdata3.mhselect) {
    case 0:
      return true;
    case 4:
      return (mcontext->val & BITMASK(13)) == trig->tdata3.mhvalue;
#ifdef CONFIG_RVH
    case 1:
    case 5:
      return (mcontext->val & BITMASK(14)) == high_sel_value;
    case 2:
    case 6:
      return hgatp->vmid == high_sel_value;
#endif
    default:
      return false;
  }
}

static bool tdata3_match(Trigger* trig) {
  return tdata3_smatch(trig) && tdata3_mhmatch(trig);
}
#endif // CONFIG_RV_SDTRIG_TDATA3

trig_action_t check_triggers_mcontrol6(
  TriggerModule* TM,
  trig_op_t op,
  vaddr_t addr,
  word_t data
) {
  if (TM->mcontrol6_state_dirty) {
    refresh_mcontrol6_state(TM);
  }
  if (TM->mcontrol6_active_count == 0) {
    return TRIG_ACTION_NONE;
  }
#ifdef CONFIG_RV_SDEXT
  // do nothing in debug mode
  if (cpu.debug_mode)
    return TRIG_ACTION_NONE;
#endif
  if (!trigger_reentrancy_check()) return TRIG_ACTION_NONE;
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
      match = mcontrol6_match(&TM->triggers[i], op, addr, data);
    }
    hit[i] = match;
  }

  for (int i = 1; i < trigger_num; i++) {
    bool last_hit = hit[i-1];
    bool last_chain = TM->triggers[i - 1].tdata1.common.type == TRIG_TYPE_MCONTROL6 &&
      TM->triggers[i - 1].tdata1.mcontrol6.chain;
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

bool mcontrol6_match(Trigger* trig, trig_op_t op, vaddr_t addr, word_t data) {
  // not meet trigger condition
  if (((op & TRIG_OP_EXECUTE) && !trig->tdata1.mcontrol6.execute) ||
      ((op & TRIG_OP_LOAD)    && !trig->tdata1.mcontrol6.load)    ||
      ((op & TRIG_OP_STORE)   && !trig->tdata1.mcontrol6.store)   ||
      ((op & TRIG_OP_TIMING))                                     ||
#ifdef CONFIG_RVH
      (cpu.mode == MODE_S && cpu.v && !trig->tdata1.mcontrol6.vs) ||
      (cpu.mode == MODE_U && cpu.v && !trig->tdata1.mcontrol6.vu) ||
#endif // CONFIG_RVH
      (cpu.mode == MODE_S && IFDEF(CONFIG_RVH, !cpu.v &&) !trig->tdata1.mcontrol6.s) ||
      (cpu.mode == MODE_U && IFDEF(CONFIG_RVH, !cpu.v &&) !trig->tdata1.mcontrol6.u) ||
      (cpu.mode == MODE_M && !trig->tdata1.mcontrol6.m)
      ) {
    return false;
  }
  word_t value = trig->tdata1.mcontrol6.select ? data : addr;

  return mcontrol6_value_match(trig, value) &&
         MUXDEF(CONFIG_RV_SDTRIG_TDATA3, tdata3_match(trig), true);
}

bool mcontrol6_value_match(Trigger* trig, word_t value) {
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

bool mcontrol6_check_chain_legal(const TriggerModule* TM, const int max_chain_len) {
  int chain_len = 0;
  int i = 0;
  while (i < CONFIG_TRIGGER_NUM && chain_len < max_chain_len) {
    if (TM->triggers[i].tdata1.common.type == TRIG_TYPE_MCONTROL6 &&
        TM->triggers[i].tdata1.mcontrol6.chain) {
      chain_len++;
    } else {
      chain_len = 0;
    }
    i++;
  }
  if (!(chain_len < max_chain_len))
    printf("mcontrol6_check_chain_legal returns false\n");
  return chain_len < max_chain_len;
}

void mcontrol6_checked_write(trig_mcontrol6_t* mcontrol6, word_t* wdata, const TriggerModule* TM) {
  if (!tinfo->mcontrol6) {
    return ;
  }
  trig_mcontrol6_t* wdata_mcontrol6 = (trig_mcontrol6_t*)wdata;
  ((tdata1_t *)mcontrol6)->val = 0;
  mcontrol6->type = TRIG_TYPE_MCONTROL6;
  mcontrol6->vs = wdata_mcontrol6->vs;
  mcontrol6->vu = wdata_mcontrol6->vu;
  mcontrol6->select = 0;
  mcontrol6->action = TRIG_ACTION_BKPT_EXCPT;
  mcontrol6->chain = wdata_mcontrol6->chain; // chain length will be checked later
  mcontrol6->match = (wdata_mcontrol6->match == TRIG_MATCH_EQ || wdata_mcontrol6->match == TRIG_MATCH_GE || wdata_mcontrol6->match == TRIG_MATCH_LT)
    ? wdata_mcontrol6->match : TRIG_MATCH_EQ;
  mcontrol6->m = wdata_mcontrol6->m;
  mcontrol6->s = wdata_mcontrol6->s;
  mcontrol6->u = wdata_mcontrol6->u;
  mcontrol6->execute = wdata_mcontrol6->execute;
  mcontrol6->store = wdata_mcontrol6->store;
  mcontrol6->load = wdata_mcontrol6->load;
  mcontrol6->chain = mcontrol6_check_chain_legal(TM, 2) && wdata_mcontrol6->chain;
}

static inline bool trigger_mode_match(bool m, bool s, bool u, bool vs, bool vu) {
#ifdef CONFIG_RVH
  if (cpu.v) return cpu.mode == MODE_S ? vs : cpu.mode == MODE_U && vu;
#endif
  return cpu.mode == MODE_M ? m : cpu.mode == MODE_S ? s : cpu.mode == MODE_U && u;
}

trig_action_t check_triggers_etrigger(TriggerModule* TM, uint64_t cause) {
#ifdef CONFIG_RV_SDEXT
  // do nothing in debug mode
  if (cpu.debug_mode)
    return TRIG_ACTION_NONE;
#endif
  if (!trigger_reentrancy_check()) return TRIG_ACTION_NONE;
  for (int i = 0; i < CONFIG_TRIGGER_NUM; i++) {
    if (TM->triggers[i].tdata1.common.type != TRIG_TYPE_ETRIG) {
      continue;
    }
    if (etrigger_match(&TM->triggers[i], cause)) {
      return (trig_action_t)TM->triggers[i].tdata1.etrigger.action;
    }
  }
  return TRIG_ACTION_NONE;
}

bool etrigger_match(Trigger* trig, uint64_t cause) {
  if (cause >= 64 || !((trig->tdata2.val >> cause) & 1)) {
    return false;
  }
  return trigger_mode_match(trig->tdata1.etrigger.m, trig->tdata1.etrigger.s,
      trig->tdata1.etrigger.u, trig->tdata1.etrigger.vs, trig->tdata1.etrigger.vu) &&
    MUXDEF(CONFIG_RV_SDTRIG_TDATA3, tdata3_match(trig), true);
}

void etrigger_checked_write(trig_etrigger_t* etrigger, word_t* wdata) {
  if (!tinfo->etrigger) {
    return ;
  }
  trig_etrigger_t* wdata_etrigger = (trig_etrigger_t*)wdata;
  ((tdata1_t *)etrigger)->val = 0;
  etrigger->type = TRIG_TYPE_ETRIG;
  etrigger->vs = MUXDEF(CONFIG_RVH, wdata_etrigger->vs, 0);
  etrigger->vu = MUXDEF(CONFIG_RVH, wdata_etrigger->vu, 0);
  etrigger->m = wdata_etrigger->m;
  etrigger->s = wdata_etrigger->s;
  etrigger->u = wdata_etrigger->u;
  etrigger->action = TRIG_ACTION_BKPT_EXCPT;
}

trig_action_t check_triggers_itrigger(TriggerModule* TM, uint64_t cause) {
#ifdef CONFIG_RV_SDEXT
  // do nothing in debug mode
  if (cpu.debug_mode)
    return TRIG_ACTION_NONE;
#endif
  if (!trigger_reentrancy_check()) return TRIG_ACTION_NONE;
  for (int i = 0; i < CONFIG_TRIGGER_NUM; i++) {
    if (TM->triggers[i].tdata1.common.type != TRIG_TYPE_ITRIG) {
      continue;
    }
    if (itrigger_match(&TM->triggers[i], cause)) {
      return (trig_action_t)TM->triggers[i].tdata1.itrigger.action;
    }
  }
  return TRIG_ACTION_NONE;
}

bool itrigger_match(Trigger* trig, uint64_t cause) {
  bool nmi = MUXDEF(CONFIG_RV_SMRNMI, cpu.hasNMI && (cause & INTR_BIT), false);
  cause &= ~INTR_BIT;
  bool cause_match = nmi ? trig->tdata1.itrigger.nmi :
    cause < 64 && ((trig->tdata2.val >> cause) & 1);
  if (!cause_match) {
    return false;
  }
  return trigger_mode_match(trig->tdata1.itrigger.m, trig->tdata1.itrigger.s,
      trig->tdata1.itrigger.u, trig->tdata1.itrigger.vs, trig->tdata1.itrigger.vu) &&
    MUXDEF(CONFIG_RV_SDTRIG_TDATA3, tdata3_match(trig), true);
}

void itrigger_checked_write(trig_itrigger_t* itrigger, word_t* wdata) {
  if (!tinfo->itrigger) {
    return ;
  }
  trig_itrigger_t* wdata_itrigger = (trig_itrigger_t*)wdata;
  ((tdata1_t *)itrigger)->val = 0;
  itrigger->type = TRIG_TYPE_ITRIG;
  itrigger->vs = MUXDEF(CONFIG_RVH, wdata_itrigger->vs, 0);
  itrigger->vu = MUXDEF(CONFIG_RVH, wdata_itrigger->vu, 0);
  itrigger->nmi = wdata_itrigger->nmi;
  itrigger->m = wdata_itrigger->m;
  itrigger->s = wdata_itrigger->s;
  itrigger->u = wdata_itrigger->u;
  itrigger->action = TRIG_ACTION_BKPT_EXCPT;
}

trig_action_t check_triggers_trap(TriggerModule* TM, word_t cause) {
  if (cause & INTR_BIT) {
    return MUXDEF(CONFIG_TDATA1_ITRIGGER, check_triggers_itrigger(TM, cause), TRIG_ACTION_NONE);
  }
  return MUXDEF(CONFIG_TDATA1_ETRIGGER, check_triggers_etrigger(TM, cause), TRIG_ACTION_NONE);
}

void trigger_schedule_trap(TriggerModule* TM, trig_action_t action) {
  // Source-mode protection was checked before trap entry. Also check the
  // handler's mode after entry, since its xIE may have just been cleared.
  TM->trap_action = trigger_reentrancy_check() ? action : TRIG_ACTION_NONE;
}

void trigger_take_trap_action(TriggerModule* TM) {
  trig_action_t action = TM->trap_action;
  TM->trap_action = TRIG_ACTION_NONE;
  if (action == TRIG_ACTION_NONE) return;
  assert(action == TRIG_ACTION_BKPT_EXCPT);
  // This event belongs to the handler PC, not the instruction that caused the
  // original trap. Supply a separate context to the longjmp exception path.
  static Decode handler;
  extern Decode *prev_s;
  memset(&handler, 0, sizeof(handler));
  handler.pc = cpu.pc;
  IFDEF(CONFIG_INSTR_CNT_BY_BB, handler.idx_in_bb = 1);
  prev_s = &handler;
  trigger_action = action;
  triggered_tval = 0;
  TM->breakpoint_no_address = true;
  longjmp_exception(EX_BP);
}

trig_action_t trigger_icount_begin_slow(TriggerModule* TM) {
  // A tcache miss decodes and retries the same instruction.
  if (TM->icount_inflight) return TRIG_ACTION_NONE;
  TM->icount_inflight = true;
  TM->icount_matched = icount_match(&TM->triggers[0]);
  trig_icount_t* icount = &TM->triggers[0].tdata1.icount;
  if (TM->icount_matched && icount->pending) {
    icount->pending = 0;
    IFDEF(CONFIG_SHARE, csr_difftest_mark_trigger_dirty());
    return TRIG_ACTION_BKPT_EXCPT;
  }
  return TRIG_ACTION_NONE;
}

void trigger_icount_retire_slow(TriggerModule* TM) {
  trig_icount_t* icount = &TM->triggers[0].tdata1.icount;
  if (TM->icount_inflight && TM->icount_matched && icount->count != 0) {
    if (--icount->count == 0) icount->pending = 1;
    IFDEF(CONFIG_SHARE, csr_difftest_mark_trigger_dirty());
  }
  TM->icount_inflight = false;
  TM->icount_matched = false;
}

void trigger_icount_trap(TriggerModule* TM) {
  // A synchronous trap completes the current instruction once. An interrupt
  // is its own match event, before trap entry changes mode and disables xIE.
  // Interrupts take precedence over action=0: never consume pending here.
  if (!TM->icount_inflight) {
    TM->icount_inflight = true;
    TM->icount_matched = icount_match(&TM->triggers[0]);
  }
  trigger_icount_retire(TM);
}

bool icount_match(Trigger* trig) {
  if (trig->tdata1.common.type != TRIG_TYPE_ICOUNT ||
      (trig->tdata1.icount.count == 0 && !trig->tdata1.icount.pending)) {
    return false;
  }
  return trigger_mode_match(trig->tdata1.icount.m, trig->tdata1.icount.s,
      trig->tdata1.icount.u, trig->tdata1.icount.vs, trig->tdata1.icount.vu) &&
    trigger_reentrancy_check() && MUXDEF(CONFIG_RV_SDTRIG_TDATA3, tdata3_match(trig), true);
}

void icount_checked_write(trig_icount_t* icount, word_t* wdata) {
  if (!tinfo->icount || tselect->val != 0) {
    return ;
  }
  trig_icount_t* wdata_icount = (trig_icount_t*)wdata;
  ((tdata1_t *)icount)->val = 0;
  icount->type = TRIG_TYPE_ICOUNT;
  icount->vs = MUXDEF(CONFIG_RVH, wdata_icount->vs, 0);
  icount->vu = MUXDEF(CONFIG_RVH, wdata_icount->vu, 0);
  icount->count = wdata_icount->count;
  icount->m = wdata_icount->m;
  icount->pending = wdata_icount->pending;
  icount->s = wdata_icount->s;
  icount->u = wdata_icount->u;
  icount->action = TRIG_ACTION_BKPT_EXCPT;
}

bool trigger_reentrancy_check() {
  bool medeleg_bp = BITS(medeleg->val, EX_BP, EX_BP);
#ifdef CONFIG_RVH
  bool hedeleg_bp = BITS(hedeleg->val, EX_BP, EX_BP);
#endif // CONFIG_RVH
  return !(
#ifdef CONFIG_RVH
    (cpu.mode == MODE_S && cpu.v && medeleg_bp && hedeleg_bp && !vsstatus->sie) ||
#endif // CONFIG_RVH
    (cpu.mode == MODE_S && IFDEF(CONFIG_RVH, !cpu.v &&) medeleg_bp && !mstatus->sie) ||
    (cpu.mode == MODE_M && !mstatus->mie)
  );
}

void trigger_handler(const trig_type_t type, const trig_action_t action, word_t tval) {
  switch (action) {
    case TRIG_ACTION_NONE: /* no trigger hit, do nothing */; break;
    case TRIG_ACTION_BKPT_EXCPT:
      switch (type) {
        case TRIG_TYPE_ITRIG:
        case TRIG_TYPE_ETRIG:
          if (!trigger_reentrancy_check()) {
            break;
          }
        {
          extern Decode *prev_s;
          prev_s->pc = cpu.pc;
        }
        case TRIG_TYPE_MCONTROL:
        case TRIG_TYPE_MCONTROL6:
        case TRIG_TYPE_ICOUNT:
          cpu.TM->breakpoint_no_address = type != TRIG_TYPE_MCONTROL && type != TRIG_TYPE_MCONTROL6;
          trigger_action = action;
          triggered_tval = tval;
          longjmp_exception(EX_BP);
          break;
        default : panic("Unsupported trigger type %d", type);  break;
      }
      break;
    default: panic("Unsupported trigger action %d", action);  break;
  }
}

#endif //CONFIG_RV_SDTRIG
