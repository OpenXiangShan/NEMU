#ifndef __FLOW_MANAGER_HH__
#define __FLOW_MANAGER_HH__


//#include "ideal_model/ideal_model.h"
#include "isa.h"

#include <deque>

class FlowManager{
  public:
    // trace the memory write
    struct MemTrace{
      uint64_t seq_no;
      uint64_t pc;
      paddr_t addr;
      int len;
      word_t data;
      int cross_page_store;
    };
    

    struct CPUStateTrace{
      uint64_t seq_no;
      CPU_state cpu_state;
    };


  private:
    enum class MemTraceWorkState{
      NORMAL,
      RECOVER
    };


    std::deque<CPUStateTrace> cpuStateTraceVec;
    std::deque<MemTrace> memTraceVec;
    MemTraceWorkState memTraceWorkState = MemTraceWorkState::NORMAL;

  public:
    // flow message
    uint64_t seq_no = 0u;
    vaddr_t pc = 0u;

  public:
    FlowManager();
    void recoverMemory(uint64_t seq_no);
    void updateFlow(uint64_t seq_no, uint64_t pc);
    void addToManager(const uint64_t seq_no, const CPU_state *const cpu_state);
    void removeFromManager(const uint64_t seq_no);
    void recoverFromManager(CPU_state *const true_state, uint64_t seq_no, uint64_t pc, int changeType);
    // this just use for physical mem
    void markMemTrace(paddr_t addr, int len, word_t data, int cross_page_store);
    void eliminateUselessState(uint64_t seq_no);
};

extern FlowManager flowManager;

#endif
