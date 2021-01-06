#ifndef __MONITOR_MONITOR_H__
#define __MONITOR_MONITOR_H__

#include <common.h>

enum { NEMU_STOP, NEMU_RUNNING, NEMU_END, NEMU_ABORT, NEMU_REACH };

typedef struct {
  int state;
  vaddr_t halt_pc;
  uint32_t halt_ret;
} NEMUState;

extern NEMUState nemu_state;

void display_inv_msg(vaddr_t pc);

enum {
    NoSimpoint = 0,
    SimpointProfiling,
    SimpointCheckpointing,
    CheckpointRestoring,
    BetapointProfiling,
};

extern char *cpt_file;
extern char *stats_base_dir;
extern char *workload_name;
extern char *config_name;
extern char *simpoints_dir;
extern int profiling_state;
extern bool checkpointRestoring;
extern bool checkpointTaking;
extern int cpt_id;
extern unsigned profiling_interval;
extern uint64_t checkpoint_interval;
extern uint64_t max_insts;

#endif
