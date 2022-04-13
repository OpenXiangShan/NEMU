#include <checkpoint/profiling.h>

int profiling_state = NoProfiling;
bool checkpoint_taking = false;
bool checkpoint_restoring = false;
uint64_t checkpoint_interval = 0;

bool profiling_started = false;
bool force_cpt_mmode = false;

#ifdef CONFIG_SHARE
// empty definition on share
void simpoint_profiling(uint64_t pc, bool is_control, uint64_t abs_instr_count) {}
#endif 