#ifndef __MONITOR_MONITOR_H__
#define __MONITOR_MONITOR_H__

#include <common.h>

enum { NEMU_STOP, NEMU_RUNNING, NEMU_END, NEMU_ABORT };

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
    CheckpointRestoring
};

extern char *cpt_file;
extern char *stats_base_dir;
extern char *workload_name;
extern char *config_name;
extern char *simpoints_file;
extern int simpoint_state;
extern int cpt_id;

#endif
