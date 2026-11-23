#include <profiling/profiling_control.h>

int profiling_state = NoProfiling;
int checkpoint_state = NoCheckpoint;
bool checkpoint_taking = false;
uint64_t checkpoint_interval = 0;
uint64_t warmup_interval = 0;
uint64_t checkpoint_icount_base = 0;

bool recvd_manual_oneshot_cpt = false;
bool recvd_manual_uniform_cpt = false;

bool force_cpt_mmode = false;

bool donot_skip_boot=false;
bool workload_loaded=false;

void start_profiling() {
  workload_loaded=true;
  uint64_t get_abs_instr_count();
  checkpoint_icount_base = get_abs_instr_count();
  Log("Start profiling. Setting inst count base to Current inst count %lu.", checkpoint_icount_base);
}

void stop_profiling() {
  // The ROI end marker does not terminate the workload.  Stop both normal
  // ROI sampling and --dont-skip-boot sampling before workload teardown.
  workload_loaded = false;
  donot_skip_boot = false;
  Log("Stop profiling.");
}

#ifdef CONFIG_SHARE
// empty definition on share
void simpoint_profiling(uint64_t pc, bool is_control, uint64_t abs_instr_count) {}
#endif
