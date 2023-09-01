#ifndef __PROFILING_CONTROL_H__
#define __PROFILING_CONTROL_H__

#include <common.h>

enum ProfilingState{
    NoProfiling =0,
    SimpointProfiling,
};

enum CheckpointState{
    NoCheckpoint=0,
    SimpointCheckpointing,
    UniformCheckpointing,
    ManualOneShotCheckpointing,
    ManualUniformCheckpointing,
};

extern int profiling_state;
extern int checkpoint_state;
extern bool checkpoint_restoring;
extern uint64_t checkpoint_interval;

extern bool recvd_manual_oneshot_cpt;
extern bool recvd_manual_uniform_cpt;

extern bool force_cpt_mmode;

extern bool workload_loaded;
extern bool donot_skip_boot;

void reset_inst_counters();

#endif // __PROFILING_CONTROL_H__
