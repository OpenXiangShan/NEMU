#ifndef __CHECKPOINT_PROFILING_H__
#define __CHECKPOINT_PROFILING_H__

#include <common.h>

enum ProfilingState {
    NoProfiling = 0,
    SimpointProfiling,
    SimpointCheckpointing,
    UniformCheckpointing
};

extern int profiling_state;
extern bool checkpoint_taking;
extern bool checkpoint_restoring;
extern uint64_t checkpoint_interval;

extern bool profiling_started;
extern bool force_cpt_mmode;

#endif
