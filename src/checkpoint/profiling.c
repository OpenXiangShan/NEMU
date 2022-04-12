#include <checkpoint/profiling.h>

int profiling_state = NoProfiling;
bool checkpoint_taking = false;
bool checkpoint_restoring = false;
uint64_t checkpoint_interval = 0;

bool wait_manual_oneshot_cpt = false;
bool wait_manual_uniform_cpt = false;
bool recvd_manual_oneshot_cpt = false;
bool recvd_manual_uniform_cpt = false;

bool profiling_started = false;
bool force_cpt_mmode = false;

void reset_inst_counters() {
  extern uint64_t g_nr_guest_instr;
  extern bool profiling_started;
  Log("Start profiling, resetting inst count from %lu to 1, (n_remain_total will not be cleared)\n", g_nr_guest_instr);
  g_nr_guest_instr = 1;
  profiling_started = true;
}

#ifdef CONFIG_SHARE
// empty definition on share
void simpoint_profiling(uint64_t pc, bool is_control, uint64_t abs_instr_count) {}
#endif 