#include <profiling/profiling_control.h>

int profiling_state = NoProfiling;
int checkpoint_state = NoCheckpoint;
bool checkpoint_taking = false;
bool checkpoint_restoring = false;
uint64_t checkpoint_interval = 0;

bool recvd_manual_oneshot_cpt = false;
bool recvd_manual_uniform_cpt = false;

bool force_cpt_mmode = false;

bool donot_skip_boot=false;
bool workload_loaded=false;

void reset_inst_counters() {
  extern uint64_t g_nr_guest_instr;
  extern uint64_t g_nr_guest_instr_old;
  extern bool workload_loaded;
  Log("Start profiling, resetting inst count from %lu to 1, (n_remain_total will not be cleared)\n", g_nr_guest_instr);
  g_nr_guest_instr_old = g_nr_guest_instr;
  g_nr_guest_instr = 1;
  workload_loaded=true;
}

#ifdef CONFIG_SHARE
// empty definition on share
void simpoint_profiling(uint64_t pc, bool is_control, uint64_t abs_instr_count) {}
#endif 