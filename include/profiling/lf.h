/***************************************************************************************
* Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

// Leader-follower parallel SimPoint profiling.
//
// A leader process executes the guest with profiling inactive and forks a
// follower at eligible outer-loop tops at or after fixed instruction-count
// targets. Each follower profiles one slice, validates its end state against
// the state published by the next follower (or by the leader at the end of the
// run), writes a BBV fragment and exits. The leader merges the fragments into
// the serial simpoint_bbv.gz.

#ifndef __PROFILING_LF_H__
#define __PROFILING_LF_H__

#include <common.h>

#ifdef __cplusplus
extern "C" {
#endif

// getopt codes of the --lf-* flags. The flags are parsed in every build so that
// a build without the feature reports a clear error.
enum {
  LF_OPT_PROFILE = 40,
  LF_OPT_SLICE,
  LF_OPT_FOLLOWERS,
  LF_OPT_DIR,
  LF_OPT_KEEP,
  LF_OPT_MEM_TRACK,
  LF_OPT_INJECT,
  LF_OPT_SELFTEST,
  LF_OPT_FOLLOWER_CPUS,
  LF_OPT_SLICE_MAX,
  LF_OPT_END,
};

#ifdef CONFIG_LF_PROFILING

typedef enum { LF_ROLE_NONE = 0, LF_ROLE_LEADER, LF_ROLE_FOLLOWER } lf_role_t;
typedef enum { LF_MEM_TRACK_UFFD = 0, LF_MEM_TRACK_OFF } lf_mem_track_t;

typedef struct {
  bool requested;           // --lf-profile
  uint64_t slice_len;       // target slice length in instructions (the minimum)
  uint64_t slice_max;       // maximum slice length; 0 selects slice_len
  int max_exec;             // maximum number of executing followers
  const char *work_dir;     // NULL selects <output dir>/lf
  bool keep_files;          // keep records, fragments and slice logs after success
  lf_mem_track_t mem_track;
  const char *inject;       // fault-injection spec, NULL when unused
  const char *selftest;     // self-test name, NULL when unused
} lf_config_t;

extern lf_config_t lf_config;
extern lf_role_t lf_role;   // LF_ROLE_NONE unless the feature is active

void lf_set_option(int code, const char *arg);
// Validates the configuration and makes this process the leader. Called once
// after the image is loaded and before the guest starts.
void lf_init(bool difftest_enabled);

// Hooks called by cpu_exec(). The instruction count passed is g_nr_guest_instr,
// which is settled at every hook point.
void lf_on_run_entry(uint64_t icount);
// Every iteration of the outer execution loop; canonical tells whether the
// previous execute() call ended where the published block ends at the
// current pc, where slices may start and end.
void lf_on_loop_top(bool canonical, uint64_t icount);
void lf_on_run_end(uint64_t icount);

#endif // CONFIG_LF_PROFILING

#ifdef __cplusplus
}
#endif

#endif // __PROFILING_LF_H__
