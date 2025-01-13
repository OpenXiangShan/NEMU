#include "ideal_model/ideal_model.h"
#include "debug.h"
#include "cpu/decode.h"

#ifdef CONFIG_DEBUG_IDEAL_MODEL
#define BOOL_TO_STRING(b) ((b)? "true" : "false")
#define ASK_MODE_STR(b) ((b)? "iter mode" : "fetch ask")
#endif

// ** All register file copies for the ideal model
// are made under xs-gem5 **

// extern method
extern void update_fm(uint64_t seq_no, uint64_t pc);
extern void add_state_to_fm(const uint64_t seq_no, const CPU_state *const cpu_state);
extern void recover_from_fm(CPU_state *const true_state, uint64_t seq_no, uint64_t pc, int changeType);
extern void remove_state_from_fm(const uint64_t seq_no);
extern void recover_only_memtrace(uint64_t seq_no);
extern void eliminate_useless_state(uint64_t seq_no);

extern void csr_writeback();
extern void csr_prepare();

// extern variable
extern Decode *prev_s;

// ideal model status
static int work_state = IM_WORK; 
static uint64_t seq_no_cause_stop = 0;

// intr_happen can coexist with other status
static bool intr_happen = false;

// only use for exception
static bool exception_pending = false;
static bool squash_after_pending = false;

// dinger
// static bool finish_iter_mode = false;
// static FILE *exception_stream = NULL;

void ideal_model_init(){
  extern void difftest_init();
  difftest_init();
  work_state = IM_WORK;
  Logim("ideal model init\n");
}

void ideal_model_memcpy(paddr_t nemu_addr, void *dut_buf, size_t n, bool direction){
  extern void difftest_memcpy(paddr_t, void *, size_t, bool);
  difftest_memcpy(nemu_addr, dut_buf, n, direction);
}

void ideal_model_regcpy(void *gem5_reg, bool direction){
  extern void difftest_regcpy(void *, bool);
  difftest_regcpy(gem5_reg, direction);
#ifdef CONFIG_DEBUG_IDEAL_MODEL
  Logim("pc: %lX \n", cpu.pc);
#endif
}

void ideal_model_exec(uint64_t n){
  extern void difftest_exec(uint64_t);
  difftest_exec(n);
}

void ideal_model_guide_exec(void * guide){
  extern void difftest_guided_exec(void *);
  difftest_guided_exec(guide);
}

void ask_ideal_model(void *gem5_info, void *nemu_info, bool iter_mode){
  struct AskFromGEM5 *ask_info = (struct AskFromGEM5*)gem5_info;
  struct AnswerFromNemu *answer_info = (struct AnswerFromNemu*)nemu_info;
  // at each ask, reset the answer
  memset(answer_info, 0, sizeof(struct AnswerFromNemu));

  // at each ask, update seq_no and pc in flowManager.
  update_fm(ask_info->seq_no, ask_info->pc);

#ifdef CONFIG_DEBUG_IDEAL_MODEL
  Logim("[nemu: %lu] %s ask, work state: %s \n", ask_info->seq_no, ASK_MODE_STR(iter_mode), IdealModelWorkStateStr[work_state]);
  Logim("[nemu: %lu] askinfo => pc: %lX npc: %lX sysop: %s nospec: %s load: %s \n", ask_info->seq_no,
      ask_info->pc, ask_info->next_pc, 
      BOOL_TO_STRING(ask_info->is_systemop), BOOL_TO_STRING(ask_info->is_nonspec),
      BOOL_TO_STRING(ask_info->is_load));
#endif

  // if ideal model not work, do nothing. just tell gem5
  // ideal model is not work. When ideal model not work,
  // each ask will not make influence in ideal model.
  if(work_state != IM_WORK){
    if(work_state == IM_EXCEPTION_STOP){
      IFDEF(CONFIG_DEBUG_IDEAL_MODEL, Logim("exception stop: %lu", seq_no_cause_stop));
    }
    answer_info->ideal_model_work_state = work_state;
    return;
  }

  // at each effective ask, reset the helper.
  // The helper, which mark some message down in decode or
  // execute time. Will be use in after.
  memset(&cpu.im_helper, 0, sizeof(struct IdealModelAnswerHelper));

  // each mode fully guide by gem5
  cpu.pc = ask_info->pc;

  // system op cause the stop.
  if(ask_info->is_systemop){
    work_state = IM_SYSTEMOP_STOP;
#ifdef CONFIG_DEBUG_IDEAL_MODEL
    Logim("[nemu: %lu] %s system op cause stop \n", ask_info->seq_no, ASK_MODE_STR(iter_mode));
#endif
    seq_no_cause_stop = ask_info->seq_no;
    answer_info->ideal_model_work_state = work_state;
    return;
  }

  // non spec operation cause the stop.
  if(ask_info->is_nonspec){
    work_state = IM_NONSPEC_STOP;
#ifdef CONFIG_DEBUG_IDEAL_MODEL
    Logim("[nemu: %lu] %s nonspec cause stop \n", ask_info->seq_no, ASK_MODE_STR(iter_mode));
#endif
    seq_no_cause_stop = ask_info->seq_no;
    answer_info->ideal_model_work_state = work_state;
    return;
  }

  // each load do the snapshot, if it is mmio,
  // it will be move at after.
  if(ask_info->is_load){
    cpu.im_helper.mem_access_is_load = true;
  }

  // do nothing, just mark this is store.
  if(ask_info->is_store){
    cpu.im_helper.mem_access_is_store = true;
  }

  // run-ahead exec
  ideal_model_exec(1);

  // after decode and run, we get each addr,
  // we set return here.
  if(ask_info->is_store || ask_info->is_load){
    answer_info->is_mmio = cpu.im_helper.mem_is_mmio;
    answer_info->mem_vaddr = cpu.im_helper.mem_access_vaddr;
    answer_info->mem_paddr = cpu.im_helper.mem_access_paddr;
  }

  // runtime exception happen, change the status.
  // prior then mmio, also need know mmio may cause exception.
  if(cpu.im_helper.runtime_exception_happen){
    work_state = IM_EXCEPTION_STOP;
#ifdef CONFIG_DEBUG_IDEAL_MODEL
    Logim("[nemu: %lu] %s exception cause stop, NO: %lu\n", ask_info->seq_no, ASK_MODE_STR(iter_mode), cpu.im_helper.ex_cause_copy);
    if(cpu.im_helper.mem_is_mmio){
      Logim("[nemu: %lu] this exception cause by mmio \n", ask_info->seq_no);
    }
#endif
    seq_no_cause_stop = ask_info->seq_no;
    answer_info->ideal_model_work_state = work_state;
    return;
  }

  // todo: atomic mmio?
  if(cpu.im_helper.mem_is_mmio){
    work_state = IM_NONSPEC_STOP;
#ifdef CONFIG_DEBUG_IDEAL_MODEL
    Logim("[nemu: %lu] %s mmio cause stop \n", ask_info->seq_no, ASK_MODE_STR(iter_mode));
#endif
    seq_no_cause_stop = ask_info->seq_no;
    answer_info->ideal_model_work_state = work_state;
    return;
  }


  // normal instruction do the snapshot
  csr_prepare();
  add_state_to_fm(ask_info->seq_no, &cpu);

#ifdef CONFIG_DEBUG_IDEAL_MODEL
  Logim("snapshot_seq_no: %lu pc:%lx \n", ask_info->seq_no, cpu.pc);
  Logim("[nemu: %lu] %s normal instruction take snapshot \n", ask_info->seq_no, ASK_MODE_STR(iter_mode));
#endif


  // the normal situation, return the value.
  // fill the answer, return the runahead value
  Assert(work_state == IM_WORK, "invalid ideal model work state\n");
  answer_info->ideal_model_work_state = work_state;
  if(ask_info->need_provide_dest_value){
    answer_info->dest_value = *(prev_s->dest.preg);
  }

  // each instruction in gem5 do the pred, so cpu.pc guide by gem5.
  cpu.pc = ask_info->next_pc;


#ifdef CONFIG_DEBUG_IDEAL_MODEL
  Logim("[nemu: %lu] %s normal return \n", ask_info->seq_no, ASK_MODE_STR(iter_mode));
#endif

  return;
  
}

// recover state from seq_no.
// we also should care about that in the:
//   IM_NONSPEC_STOP: branch out of ordered execute squash and decode squash
//   IM_SYSTEMOP_STOP: decode squash
//   IM_EXCEPTION_STOP: branch out of ordered execute squash and decode squash
// will cause this function can't find state.
void adapt_flow_change(uint64_t seq_no, uint64_t pc, int changeType){
  // now this func only handle mispredict and load violation
  if(work_state != IM_WORK){
    if(seq_no < seq_no_cause_stop){
#ifdef CONFIG_DEBUG_IDEAL_MODEL
      Logim("[nemu: %lu] flow change make [%s:%lu] be cancel \n", seq_no, 
          IdealModelWorkStateStr[work_state], seq_no_cause_stop);
#endif
      seq_no_cause_stop = 0;
      work_state = IM_WORK;

    }else if(seq_no == seq_no_cause_stop){

#ifdef CONFIG_DEBUG_IDEAL_MODEL
      if(work_state == IM_EXCEPTION_STOP || work_state == IM_ITER_STOP){
        Logim("[nemu: %lu] flow change make [%s:%lu] be cancel \n", seq_no, 
            IdealModelWorkStateStr[work_state], seq_no_cause_stop);
      }else{
        Assert(0, "adapt should't in this case\n");
      }
#endif
      int prev_work_state = work_state;
      seq_no_cause_stop = 0;
      work_state = IM_WORK;

      if(prev_work_state == IM_ITER_STOP){
        return;
      }
    }else{
      // this handle the situation mark before.
      // In this situation, we can't find CPU_state
      // we just recover the memtrace.
      // Assert(seq_no > seq_no_cause_stop, "specical case, nonspec not commit, gem5 run ahead and squash\n");
      recover_only_memtrace(seq_no);
#ifdef CONFIG_DEBUG_IDEAL_MODEL
      Logim("[nemu: %lu] a special case in recover, may happen in iter instList, only recover memtrace\n", seq_no);
#endif
      return;
    }
  }

  Assert(work_state == IM_WORK, "only IM_WORK state can recover\n");
  // try to recover from flow change
  // only the IM_WORK state can find the CPU_state and recover.
#ifdef CONFIG_DEBUG_IDEAL_MODEL
  Logim("[nemu: %lu] recover from flow manager \n", seq_no);
#endif
  recover_from_fm(&cpu, seq_no, pc, changeType);
  csr_writeback();
  // need to clear the cached mmu states as well
  extern void update_mmu_state();
  update_mmu_state();
}

void set_intr_happen(){
  Assert(!intr_happen, "intr_happen shouldn't be set\n");
  intr_happen = true;
}


void clear_intr_happen(){
  // because have more clear
  // Assert(intr_happen, "intr_happen should be set\n");
  intr_happen = false;
}

// clear the squash after pending, back to IM_WORK.
void clear_squash_after(){
  Assert(squash_after_pending, "squash after pending must be true\n");
  squash_after_pending = false;
  work_state = IM_WORK;
}

void gem5_raise_intr(void *gem5_reg, word_t no){
  Assert(intr_happen, "before gem5 raise intr, intr_happen must be set\n");

  // do not set intr_happen to false, gem5 will call 
  // clear_intr_happen to set.
  CPU_state* reg_from_gem5 = (CPU_state *)gem5_reg;
  Assert(reg_from_gem5->pc == prev_s->pc, "intr pc not equals \n");
  
  // todo: the question is, should copy reg from gem5?
  // maybe the reg from nemu is valid to use.
  ideal_model_regcpy(gem5_reg, GEM5_TO_IDEAL_MODEL);

  extern void difftest_raise_intr(word_t NO);
  difftest_raise_intr(no);
  // note: set state here may cause error?
#ifdef CONFIG_DEBUG_IDEAL_MODEL
  Logim("[nemu] nemu guided by gem5 interrupt\n");
#endif
  work_state = IM_WORK;
}

// which time use gem5 pc?
void exception_handle(bool use_gem5_pc){
  // handle the exception
  // this code reference from cpu-exec.c
  // the second param use cpu.pc is ok?
#ifdef CONFIG_DEBUG_IDEAL_MODEL
  Logim("[nemu] exception be handled \n");
#endif
  if(use_gem5_pc){
    cpu.pc = raise_intr(cpu.im_helper.ex_cause_copy, cpu.pc);
  }else{
    cpu.pc = raise_intr(cpu.im_helper.ex_cause_copy, prev_s->pc);
  }
  cpu.amo = false; // clean up
}

void commit_inst(int inst_type, uint64_t seq_no, bool is_squash_after){
  switch (inst_type) {
    case GEM5_SYSTEMOP:
      Assert(work_state == IM_SYSTEMOP_STOP, "work state must be sysop stop \n");
      ideal_model_exec(1);
      csr_prepare();
      add_state_to_fm(seq_no, &cpu);
#ifdef CONFIG_DEBUG_IDEAL_MODEL
      Logim("[nemu: %lu] commit sysop, also snapshot \n", seq_no);
#endif
      break;
    case GEM5_NOSPEC:
      Assert(work_state == IM_NONSPEC_STOP, "work state must be nonspec stop \n");
      // gem5 copy back status
      // copy back will done in gem5
      // the reg-status we need is after status
      csr_prepare();
      add_state_to_fm(seq_no, &cpu);
#ifdef CONFIG_DEBUG_IDEAL_MODEL
      Logim("[nemu: %lu] commit nonspec, also snapshot \n", seq_no);
#endif
      break;
    default:
      // no thing should do in signle commit
      // way be we can clear some snapshot
      // also should clear memory trace
#ifdef CONFIG_DEBUG_IDEAL_MODEL
      Logim("[nemu]in ideal model commit inst\n");
#endif
      eliminate_useless_state(seq_no);
      return;
  }
  if(is_squash_after){
#ifdef CONFIG_DEBUG_IDEAL_MODEL
    Logim("[nemu: %lu] commit nonspec squash after, wait to work \n", seq_no);
#endif
    // wait for squash after, fuck!
    Assert(!squash_after_pending, "squash after pending must be false\n");
    squash_after_pending = true;
  }else{
    work_state = IM_WORK;
  }
}


void raise_runtime_exception(int inst_type, uint64_t seq_no, word_t exception_no, bool is_ecall_ebreak){
  // clear memtrace and snapshot 
  Assert(!exception_pending, "in exception pending state but want set again \n");
  exception_pending = true;
  switch (work_state) {
    case IM_SYSTEMOP_STOP:
#ifdef CONFIG_DEBUG_IDEAL_MODEL
      Logim("[nemu: %lu] begin handle sysop run exception \n", seq_no);
#endif
      Assert(inst_type == GEM5_SYSTEMOP, "inst_type must be system op\n");
      ideal_model_exec(1);
      // verify is that turly have run exception with system op?
      Assert(cpu.im_helper.runtime_exception_happen, "verify system op run exception error\n");
      Assert(cpu.im_helper.ex_cause_copy == exception_no, "nemu exception not equals to gem5 \n");
#ifdef CONFIG_DEBUG_IDEAL_MODEL
      Logim("[nemu: %lu] system op actually raise exception \n", seq_no);
#endif
      work_state = IM_EXCEPTION_STOP;
      // the ecall/ebreak direction itself handles exceptions, so we
      // don't need to do anything here.
      if(!is_ecall_ebreak){
        exception_handle(false);
      }
      break;
    case IM_NONSPEC_STOP:
      Assert(inst_type == GEM5_NOSPEC, "inst_type must be nonspec\n");
#ifdef CONFIG_DEBUG_IDEAL_MODEL
      Logim("[nemu: %lu] nonspec cant handle exception, need gem5 copy \n", seq_no);
#endif
      // nemu cant handle no spec instruction
      // gem5 should copy all reg state
      exception_handle(true);
      break;
    case IM_EXCEPTION_STOP:
      Assert(cpu.im_helper.ex_cause_copy == exception_no, "nemu exception not equals to gem5 \n");
      // ** normal exception **
      // because some instruction should guide
      // so the normol exception handle done gem5
      // just change work state here
#ifdef CONFIG_DEBUG_IDEAL_MODEL
      Logim("[nemu: %lu] normal exception can be nemu handle itself \n", seq_no);
#endif
      exception_handle(false);
      break;
    default:
      panic("ideal model work state should't raise some intr\n");
  }
}

bool raise_force_exception(uint64_t seq_no, word_t exception_no){
  Assert(!exception_pending, "in exception pending state but want set again \n");
  exception_pending = true;
  switch (work_state) {
    case IM_EXCEPTION_STOP:
      // can do it selfly
      Assert(cpu.im_helper.ex_cause_copy == exception_no, "nemu exception not equals to gem5 \n");
#ifdef CONFIG_DEBUG_IDEAL_MODEL
      Logim("[nemu: %lu] nemu can do force itself \n", seq_no);
#endif
      exception_handle(false);
      return false;
    default:
      // gem5 guide force raise
      // just recover memory
      // gem5 also should prepare regs
      // gem5 will guide, this maybe do nothing
      Logim("force raise interrupt will run-exec once in may be not IM_WORK state\n");
#ifdef CONFIG_DEBUG_IDEAL_MODEL
      Logim("[nemu: %lu] force intr(%lu) can't handle by nemu itself \n", seq_no, exception_no);
#endif
      recover_only_memtrace(seq_no);
      return true;
  }
}

void clear_exception_pending(){
  Assert(exception_pending, "no exception but want clear pending \n");
#ifdef CONFIG_DEBUG_IDEAL_MODEL
  Logim("[nemu] clear interrupt pending state\n");
#endif
  exception_pending = false;
  work_state = IM_WORK;
}


void set_iter_stop_state(uint64_t seq_no){
  Assert(work_state == IM_WORK, "set iter stop must in IM_WORK iter\n");

  // exec the branch
  extern void difftest_exec(uint64_t);
  difftest_exec(1);

  work_state = IM_ITER_STOP;
  seq_no_cause_stop = seq_no;
#ifdef CONFIG_DEBUG_IDEAL_MODEL
  Logim("[nemu: %lu]set work_state to IM_ITER_STOP\n", seq_no);
#endif
}

// use to temp exec and recover back to check some branch
void temp_exec(void *res_work_state, void *res_pc, uint64_t gem5_pc){
  Assert(work_state == IM_WORK, "temp exec must in IM_WORK iter\n");
  
  // only for branch
  int *ret_work_state = (int *)res_work_state;
  uint64_t *ret_pc = (uint64_t *)res_pc;

  csr_prepare();
  CPU_state temp_state;
  int temp_work_state;
  memcpy(&temp_state, &cpu, sizeof(CPU_state));
  temp_work_state = work_state;
  cpu.pc = gem5_pc;

  IFDEF(CONFIG_DEBUG_IDEAL_MODEL, Logim("before temp exec, pc is %lx\n", cpu.pc));

  extern void difftest_exec(uint64_t);
  difftest_exec(1);

  // set return res
  Assert(work_state == IM_WORK, "after exec, we still should in IM_WORK \n");
  *ret_work_state = work_state;
  *ret_pc = cpu.pc;

  work_state = temp_work_state;
  memcpy(&cpu, &temp_state, sizeof(CPU_state));
  csr_writeback();
  extern void update_mmu_state();
  update_mmu_state();

  return;
}
