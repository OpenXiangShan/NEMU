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

// State shared by the leader and follower halves of leader-follower profiling.

#ifndef __PROFILING_LF_INTERNAL_H__
#define __PROFILING_LF_INTERNAL_H__

#include <profiling/lf.h>
#include <profiling/profiling_control.h>
#include <profiling/lf_memtrack.h>
#include <sys/types.h>

// Worker exit codes.
#define LF_FOLLOWER_EXIT_OK        0
#define LF_FOLLOWER_EXIT_ERROR     11
#define LF_FOLLOWER_EXIT_ORPHAN    12

// Exit code of the run when a follower fails (a leader's divergence is not a
// failure: it is rolled back).
#define LF_RUN_EXIT_ERROR          4

// Processes. The coordinator (the original process) owns the run: slice
// table, commits, rollbacks, the merger and the final exit code. The leader
// executes with profiling inactive and forks one follower per slice; it and
// its followers form one process group. Followers replay their slice with
// profiling and validate its end state. After a leader diverges, the
// follower that validated last becomes the leader.
typedef enum {
  // worker -> coordinator
  LF_MSG_HELLO = 1,         // role, gen, slice, pid, start_icount
  LF_MSG_RECORD,            // follower `slice` published start record `slice`
  LF_MSG_EXEC_DONE,         // follower stopped at end_icount
  LF_MSG_RESULT,            // follower result: status, timings, reason
  LF_MSG_LEADER_END,        // leader: run ended at end_icount; start record `slice` published
  LF_MSG_READY,             // follower moved to its own process group
  // coordinator -> worker
  LF_MSG_SLOT,              // leader: one executing follower finished
  LF_MSG_RECORD_READY,      // follower: the next start record is published
  LF_MSG_NO_RECORD,         // follower: the leader is gone, there is no next record
  LF_MSG_PREPARE,           // follower: leave the leader's process group
  LF_MSG_PROMOTE,           // follower: become the leader of generation `gen`
  LF_MSG_EXIT,              // worker: exit
  LF_MSG_FINAL_OK,          // leader: every slice is committed, finish the run
} lf_msg_type_t;

typedef enum { LF_HELLO_LEADER = 1, LF_HELLO_FOLLOWER } lf_hello_role_t;

// Follower results.
typedef enum {
  LF_RESULT_OK = 1,         // end state equals the next boundary record
  LF_RESULT_DIVERGED,       // end state differs: the leader diverged in this slice
  LF_RESULT_UNVALIDATED,    // no next record (the leader is gone)
  LF_RESULT_ERROR,          // the follower failed; there is no valid state
} lf_result_t;

// Fixed-size message on a worker's connection to the coordinator.
typedef struct {
  uint32_t type;            // lf_msg_type_t
  uint32_t gen;             // leader generation of the sender or target
  uint32_t slice;
  uint32_t role;            // HELLO: lf_hello_role_t
  int32_t pid;
  int32_t status;           // RESULT: lf_result_t
  uint64_t start_icount;    // HELLO
  uint64_t end_icount;      // EXEC_DONE, LEADER_END
  uint64_t exec_us;         // RESULT: guest execution time of the slice
  uint64_t startup_us;      // start record, arming
  uint64_t finish_us;       // end record, scan, comparison and fragment
  uint64_t handoff_wait_us; // time spent waiting for the next start record
  // LEADER_END: statistics of the final leader
  uint64_t fork_us, fork_max_us, scan_us, scan_max_us, nr_scans, scanned_pages;
  int64_t minflt;
  char reason[160];         // RESULT: failure description
} lf_msg_t;

// Connection to the coordinator (abstract AF_UNIX socket, one per worker).
extern char lf_coord_addr[];
int lf_coord_connect(void);
void lf_msg_send(int fd, const lf_msg_t *m);
// Receives one message; returns 1, 0 at EOF, or -1 when none is available
// (nonblocking) or on error.
int lf_msg_recv(int fd, lf_msg_t *m, bool block);


// End of the slice that the leader started last, in instructions after the
// profiling base: the leader's next boundary, and the stop of the follower
// that inherited it at the fork. Zero until the first slice starts.
extern uint64_t lf_slice_end;

// True at a canonical loop top at or after the end of the current slice,
// once profiling is ready.
static inline bool lf_slice_ended(bool canonical, uint64_t icount) {
  return canonical && profiling_workload_ready() && icount >= checkpoint_icount_base + lf_slice_end;
}

// Fault injection for testing the validation paths (--lf-inject=KIND@WHERE).
typedef enum {
  LF_INJ_NONE = 0,
  // Leader side, applied at the first loop top at or after an instruction count.
  LF_INJ_CSR, LF_INJ_TIMER, LF_INJ_GPR, LF_INJ_MEM_HASH, LF_INJ_MEM_SET, LF_INJ_LEADER_CRASH,
  // Follower side, applied by the follower of one slice.
  LF_INJ_CRASH, LF_INJ_REC_GPR, LF_INJ_REC_CSR, LF_INJ_REC_MTIME, LF_INJ_REC_ICOUNT,
  LF_INJ_REC_DEV, LF_INJ_SLOW_HELLO, LF_INJ_DESYNC,
} lf_inject_kind_t;

typedef struct {
  lf_inject_kind_t kind;
  uint64_t where;           // instruction count (leader kinds) or slice (follower kinds)
  bool fired;
} lf_inject_t;

extern lf_inject_t lf_inject;

static inline bool lf_inject_follower_kind(lf_inject_kind_t k) { return k >= LF_INJ_CRASH; }

// Guest memory write tracking. The leader collects the pages written in each
// slice at the boundary that ends it; a follower forked there inherits the list
// and hashes those pages for its start record.
extern lf_memtrack_t lf_track;      // tracker owned by this process
extern lf_page_list_t lf_written;   // leader: pages written in the slice that just ended
static inline bool lf_mem_tracking(void) { return lf_config.mem_track != LF_MEM_TRACK_OFF; }
int lf_open_pmem_tracker(void);

_Noreturn void lf_run_selftest(const char *name);

// Work directory, set before the first fork, and paths in it.
extern char lf_work_dir[];
void lf_work_path(char *buf, size_t len, const char *name);
void lf_slice_path(char *buf, size_t len, uint32_t slice, const char *suffix);
// Sends stdout and stderr of a forked child to files (NULL stderr_path:
// the same file as stdout) and detaches it from the parent's log.
void lf_redirect_child_output(const char *path, const char *stderr_path);
void lf_leader_err_path(char *buf, size_t len, uint32_t gen);
void lf_redirect_leader_stderr(uint32_t gen);
// A duplicate of the run's original stdout, kept open in every worker so that
// a promoted follower can take it back.
extern int lf_orig_stdout;

// Follower half (lf_follower.c). Follower 0 is forked at run entry, with the
// profiler in its constructor state.
bool lf_set_follower_cpus(const char *list);
void lf_apply_follower_cpus(void);
void lf_follower_start(uint32_t gen, uint32_t slice, uint64_t icount);
void lf_follower_loop_top(bool canonical, uint64_t icount);
// Stops the slice and validates it. Returns only when the follower was
// promoted to leader; exits otherwise.
void lf_follower_finish(uint64_t icount, bool run_ended);
_Noreturn void lf_follower_exit(int code);

// Leader half (lf_leader.c). A leader starts at run entry (generation 0,
// forking follower 0) or by promotion at a validated boundary.
void lf_leader_start(uint32_t gen, uint32_t next_slice, uint64_t icount, bool run_ended);
void lf_leader_loop_top(bool canonical, uint64_t icount);
void lf_leader_run_end(uint64_t icount);
// This worker's connection to the coordinator. A follower closes the one it
// inherits from the leader and opens its own, which it keeps when promoted.
extern int lf_sock;

// Coordinator (lf_coord.c): runs in the original process from run entry on.
_Noreturn void lf_coordinator_main(pid_t leader_pid);

// SimPoint side (simpoint.cpp, lf_merge.cpp).
int simpoint_lf_output_dir(char *buf, size_t len);
void simpoint_lf_begin_slice(bool pristine, uint64_t profiled_icount, uint64_t next_pc);
void *lf_merger_open(const char *out_path);
int lf_merger_add(void *m, const char *fragment, uint32_t slice, uint64_t start_icount,
                  uint64_t end_icount, char *err, size_t err_len);
int lf_merger_finish(void *m, const char *prior_err, char *err, size_t err_len);

#endif // __PROFILING_LF_INTERNAL_H__
