#ifndef __PROFILING_CONTROL_H__
#define __PROFILING_CONTROL_H__

#include <common.h>

enum ProfilingState {
    NoProfiling = 0,
    SimpointProfiling,
    SimpointCheckpointing,
//    BetapointProfiling,
    UniformCheckpointing
};

extern int profiling_state;
extern bool checkpoint_taking;
extern bool checkpoint_restoring;
extern uint64_t checkpoint_interval;

extern bool wait_manual_oneshot_cpt;
extern bool wait_manual_uniform_cpt;
extern bool recvd_manual_oneshot_cpt;
extern bool recvd_manual_uniform_cpt;

extern bool profiling_started;
extern bool force_cpt_mmode;

void reset_inst_counters();

#endif // __PROFILING_CONTROL_H__
