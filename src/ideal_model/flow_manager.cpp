#include "ideal_model/flow_manager.h"

#include <algorithm>

extern "C" {
  #include "debug.h"
  // pmem provide interface to direct write pmem
  extern void fm_recover_pmem_write(paddr_t addr, int len, word_t data, int cross_page_store); 
}

FlowManager flowManager;

FlowManager::FlowManager(){

}

// ask ideal model call this to update seq_no and pc provide by fetch
void FlowManager::updateFlow(uint64_t seq_no, uint64_t pc){
  this->seq_no = seq_no;
  this->pc = pc;
}

// snapshot CPU_state in flowManager
void FlowManager::addToManager(const uint64_t seq_no, const CPU_state *const cpu_state){
  cpuStateTraceVec.emplace_back(CPUStateTrace{seq_no, *cpu_state});
}

// remove the back member from cpuStateTraceVec
void FlowManager::removeFromManager(const uint64_t seq_no){
  assert(cpuStateTraceVec.back().seq_no == seq_no);
  cpuStateTraceVec.pop_back();
};

// recover the seq_no's cpu_state in cpuStateTraceVec
void FlowManager::recoverFromManager(CPU_state *const true_state, uint64_t seq_no, uint64_t pc, int changeType){
  while(!cpuStateTraceVec.empty()){
    CPUStateTrace &trace = cpuStateTraceVec.back();
    if(trace.seq_no <= seq_no){
      break;
    }
    cpuStateTraceVec.pop_back();
  }
  assert(!cpuStateTraceVec.empty());
  CPUStateTrace &trace = cpuStateTraceVec.back();

  memcpy(true_state, &(trace.cpu_state), sizeof(CPU_state));
  recoverMemory(seq_no);
  return;
}

// when nemu do the pmem write, mark the memory trace.
void FlowManager::markMemTrace(paddr_t addr, int len, word_t data, int cross_page_store){
  if(memTraceWorkState == MemTraceWorkState::RECOVER){
    return;
  }
  memTraceVec.emplace_front(MemTrace{seq_no, pc, addr, len, data, cross_page_store});
}

// iterating the trace in memTraceVec, recover the pmem.
void FlowManager::recoverMemory(uint64_t seq_no){

  memTraceWorkState = MemTraceWorkState::RECOVER;

  while(!memTraceVec.empty()){
    MemTrace &trace = memTraceVec.front();
    if(trace.seq_no <= seq_no){
      break;
    }
    fm_recover_pmem_write(trace.addr, trace.len, trace.data, trace.cross_page_store);
    memTraceVec.pop_front();
  }
  memTraceWorkState = MemTraceWorkState::NORMAL;

  return;
}

// when gem5 commit, we can eliminate some useless info.
void FlowManager::eliminateUselessState(uint64_t seq_no){
  while(!cpuStateTraceVec.empty() && cpuStateTraceVec.size() > 1000 && cpuStateTraceVec.front().seq_no < seq_no){
    cpuStateTraceVec.pop_front();
  }
  while(!memTraceVec.empty() && memTraceVec.size() > 1000 && memTraceVec.back().seq_no < seq_no){
    memTraceVec.pop_back();
  }
  return;
}


extern "C" {
  uint64_t *const ideal_flow_seq_no_ptr = &flowManager.seq_no;
  uint64_t *const ideal_flow_pc_ptr = &flowManager.pc;

  void update_fm(uint64_t seq_no, uint64_t pc){
    flowManager.updateFlow(seq_no, pc); 
  }

  void add_state_to_fm(const uint64_t seq_no, const CPU_state *const cpu_state){
    flowManager.addToManager(seq_no, cpu_state);    
  }

  void remove_state_from_fm(const uint64_t seq_no){
    flowManager.removeFromManager(seq_no);
  }

  void recover_from_fm(CPU_state *const true_state, uint64_t seq_no, uint64_t pc, int changeType){
    flowManager.recoverFromManager(true_state, seq_no, pc, changeType);
  }

  void mark_memtrace_to_fm(paddr_t addr, int len, word_t data, int cross_page_store){
    flowManager.markMemTrace(addr, len, data, cross_page_store);
  }

  void recover_only_memtrace(uint64_t seq_no){
    flowManager.recoverMemory(seq_no);
  }

  void eliminate_useless_state(uint64_t seq_no){
    flowManager.eliminateUselessState(seq_no);
  }
}
