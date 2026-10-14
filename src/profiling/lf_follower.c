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

// Follower side of leader-follower profiling: isolation from the leader,
// slice execution, validation, fragment output and reporting.

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <generated/autoconf.h>

#ifdef CONFIG_LF_PROFILING

#include <isa.h>
#include <cpu/cpu.h>
#include <profiling/lf_record.h>
#include <profiling/lf_fragment.h>
#include "lf_internal.h"
#include <sched.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <unistd.h>

int simpoint_lf_end_slice(const char *path, lf_fragment_header_t *hdr, char *err, size_t err_len);

static struct {
  uint32_t gen;              // generation of the leader that forked this follower
  uint32_t index;            // slice number i
  uint64_t start_icount;     // P_i: g_nr_guest_instr at the fork point
  uint64_t start_profiled;   // instructions profiled before P_i
  uint64_t t_start, startup_us;
} fl;

static uint64_t lf_elapsed(uint64_t t0) { return get_time() - t0; }

_Noreturn void lf_follower_exit(int code) {
  fflush(stdout);
  fflush(stderr);
  // Skip inherited atexit handlers and destructors: they belong to the leader.
  _exit(code);
}

static void lf_send_result(int status, const char *reason) {
  lf_msg_t m = { .type = LF_MSG_RESULT, .gen = fl.gen, .slice = fl.index, .status = status };
  snprintf(m.reason, sizeof(m.reason), "%s", reason ? reason : "");
  lf_msg_send(lf_sock, &m);
}

// Reports an internal error to the coordinator and exits.
__attribute__((format(printf, 1, 2), noreturn))
static void lf_follower_fail(const char *fmt, ...) {
  char reason[160];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(reason, sizeof(reason), fmt, ap);
  va_end(ap);
  fprintf(stdout, "follower %u: %s\n", fl.index, reason);
  lf_send_result(LF_RESULT_ERROR, reason);
  lf_follower_exit(LF_FOLLOWER_EXIT_ERROR);
}

// Follower-side corruption of the published record, for testing each
// comparison path deterministically.
static void lf_inject_record(lf_record_t *rec) {
  switch (lf_inject.kind) {
    case LF_INJ_REC_GPR: rec->h.isa.regs.gpr[1]._64 ^= 1; break;  // ra
    case LF_INJ_REC_CSR: rec->h.isa.csr[0x340] ^= 1; break;       // mscratch
    case LF_INJ_REC_MTIME: rec->h.mtime ^= 1; break;
    case LF_INJ_REC_ICOUNT: rec->h.icount += 1; break;
    case LF_INJ_REC_DEV: rec->h.dev[0] ^= 1; break;
    default: return;
  }
  Log("injected record corruption kind %d", lf_inject.kind);
}

// Publishes R_i, the leader's state at P_i, and tells the coordinator.
static void lf_publish_start_record(uint32_t slice) {
  lf_record_t rec;
  lf_record_capture(&rec, LF_REC_START, slice, false);
  // The inherited list holds the pages the leader wrote in the previous slice.
  if (lf_mem_tracking()) lf_record_set_pages(&rec, &lf_written);
  if (lf_inject.where == slice) lf_inject_record(&rec);
  char path[PATH_MAX];
  lf_slice_path(path, sizeof(path), slice, "start.rec");
  int rc = lf_record_write(path, &rec);
  lf_record_free(&rec);
  if (rc != 0) lf_follower_fail("cannot write %s: %s", path, strerror(-rc));
  lf_msg_t m = { .type = LF_MSG_RECORD, .gen = fl.gen, .slice = slice };
  lf_msg_send(lf_sock, &m);
}

// Followers and the merger may be placed on other host CPUs than the leader
// (--lf-follower-cpus), so that the leader keeps a last-level cache to itself.
static cpu_set_t follower_cpus;  // empty unless the option is given

// Parses a list such as "8-31" or "1,4-7"; returns false on malformed input.
bool lf_set_follower_cpus(const char *list) {
  cpu_set_t set;
  CPU_ZERO(&set);
  const char *p = list;
  for (;;) {
    char *end;
    long lo = strtol(p, &end, 10), hi = lo;
    if (end == p || lo < 0) return false;
    if (*end == '-') {
      p = end + 1;
      hi = strtol(p, &end, 10);
      if (end == p || hi < lo) return false;
    }
    if (hi >= CPU_SETSIZE) return false;
    for (long c = lo; c <= hi; c++) CPU_SET(c, &set);
    if (*end == '\0') break;
    if (*end != ',') return false;
    p = end + 1;
  }
  follower_cpus = set;
  return true;
}

void lf_apply_follower_cpus(void) {
  if (CPU_COUNT(&follower_cpus) > 0 && sched_setaffinity(0, sizeof(follower_cpus), &follower_cpus) != 0)
    Log("sched_setaffinity for --lf-follower-cpus: %s", strerror(errno));
}

void lf_follower_start(uint32_t gen, uint32_t slice, uint64_t icount) {
  lf_role = LF_ROLE_FOLLOWER;
  lf_apply_follower_cpus();
  signal(SIGPIPE, SIG_IGN);
  char log[PATH_MAX], console[PATH_MAX];
  lf_slice_path(log, sizeof(log), slice, "log");
  lf_slice_path(console, sizeof(console), slice, "console");
  // The guest console of the slice, which the coordinator emits in commit order.
  lf_redirect_child_output(log, console);

  uint64_t t0 = get_time();
  fl.gen = gen;
  fl.index = slice;
  fl.start_icount = icount;
  close(lf_sock);
  // Delays this follower's connection, so that the next one connects first.
  if (lf_inject.kind == LF_INJ_SLOW_HELLO && lf_inject.where == slice) usleep(500000);
  lf_sock = lf_coord_connect();
  if (lf_sock < 0) lf_follower_exit(LF_FOLLOWER_EXIT_ORPHAN);
  lf_msg_t hello = { .type = LF_MSG_HELLO, .role = LF_HELLO_FOLLOWER, .gen = gen, .slice = slice,
                     .pid = getpid(), .start_icount = icount };
  lf_msg_send(lf_sock, &hello);
  Log("follower %u starts at icount %lu (generation %u)%s", slice, icount, gen,
      slice == 0 ? " (run entry)" : "");
  // The inherited tracker belongs to the leader; this copy of the mapping is
  // no longer registered with it.
  if (lf_mem_tracking()) lf_memtrack_forget_inherited(&lf_track);
  if (slice > 0) lf_publish_start_record(slice);
  lf_page_list_free(&lf_written);
  if (lf_mem_tracking()) {
    int rc = lf_open_pmem_tracker();
    if (rc != 0) lf_follower_fail("memory tracking unavailable: %s", strerror(-rc));
  }
  if (lf_inject.kind == LF_INJ_CRASH && lf_inject.where == slice) {
    Log("injected crash in follower %u", slice);
    fflush(stdout);
    abort();
  }
  // Profiling becomes active at the next execute() call.
  fl.start_profiled = profiling_workload_ready() ? icount - checkpoint_icount_base : 0;
  profiling_state = SimpointProfiling;
  simpoint_lf_begin_slice(slice == 0, fl.start_profiled, cpu.pc);
  fl.startup_us = lf_elapsed(t0);
  fl.t_start = get_time();
}

void lf_follower_loop_top(bool canonical, uint64_t icount) {
  if (lf_slice_ended(canonical, icount))
    lf_follower_finish(icount, false);
}

// Waits for the coordinator's next message.
static lf_msg_t lf_follower_wait(void) {
  lf_msg_t m;
  int r = lf_msg_recv(lf_sock, &m, true);
  if (r != 1) lf_follower_exit(LF_FOLLOWER_EXIT_ORPHAN);  // the coordinator is gone
  if (m.type == LF_MSG_EXIT) lf_follower_exit(LF_FOLLOWER_EXIT_OK);
  return m;
}

void lf_follower_finish(uint64_t icount, bool run_ended) {
  uint64_t exec_us = lf_elapsed(fl.t_start);
  uint64_t t0 = get_time();
  Log("follower %u stops at icount %lu%s", fl.index, icount, run_ended ? " (end of run)" : "");
  uint32_t next = fl.index + 1;
  lf_record_t mine;
  lf_record_capture(&mine, LF_REC_END, next, run_ended);
  if (lf_mem_tracking()) {
    lf_page_list_t written = {0};
    int rc = lf_memtrack_collect(&lf_track, &written, false);
    if (rc != 0) lf_follower_fail("memory scan failed: %s", strerror(-rc));
    lf_record_set_pages(&mine, &written);
    lf_page_list_free(&written);
  }
  char path[PATH_MAX];
  lf_slice_path(path, sizeof(path), fl.index, "end.rec");
  int rc = lf_record_write(path, &mine);
  if (rc != 0) lf_follower_fail("cannot write %s: %s", path, strerror(-rc));

  // This slice's profile is correct whatever the validation finds: the
  // follower started from a state validated before.
  uint64_t end_profiled = profiling_workload_ready() ? icount - checkpoint_icount_base : 0;
  lf_fragment_header_t hdr = {
    .version = LF_FRAG_VERSION, .slice = fl.index,
    .start_icount = fl.start_icount, .end_icount = icount,
    .start_profiled = fl.start_profiled, .end_profiled = end_profiled, .run_ended = run_ended,
  };
  memcpy(hdr.magic, LF_FRAG_MAGIC, sizeof(hdr.magic));
  char err[128];
  lf_slice_path(path, sizeof(path), fl.index, "bbvf");
  if (simpoint_lf_end_slice(path, &hdr, err, sizeof(err)) != 0) lf_follower_fail("BBV fragment: %s", err);
  uint64_t finish_us = lf_elapsed(t0);

  // Reporting the stop releases the leader's executing slot before waiting,
  // so that it can fork the follower that publishes the next record.
  lf_msg_t done = { .type = LF_MSG_EXEC_DONE, .gen = fl.gen, .slice = fl.index, .end_icount = icount };
  lf_msg_send(lf_sock, &done);

  uint64_t t1 = get_time();
  lf_msg_t m = lf_follower_wait();
  uint64_t handoff_wait_us = lf_elapsed(t1);
  int status;
  if (m.type == LF_MSG_RECORD_READY) {
    t0 = get_time();
    lf_record_t leader;
    lf_slice_path(path, sizeof(path), next, "start.rec");
    rc = lf_record_read(path, &leader);
    if (rc != 0) lf_follower_fail("cannot read %s: %s", path, strerror(-rc));
    status = lf_record_compare(&leader, &mine) ? LF_RESULT_OK : LF_RESULT_DIVERGED;
    lf_record_free(&leader);
    finish_us += lf_elapsed(t0);
  } else if (m.type == LF_MSG_NO_RECORD) {
    status = LF_RESULT_UNVALIDATED;
  } else {
    lf_follower_fail("unexpected message %u", m.type);
  }
  lf_record_free(&mine);
  Log("follower %u validation against boundary %u: %s", fl.index, next,
      status == LF_RESULT_OK ? "ok" : status == LF_RESULT_DIVERGED ? "DIVERGED" : "no record");
  lf_msg_t r = {
    .type = LF_MSG_RESULT, .gen = fl.gen, .slice = fl.index, .status = status,
    .exec_us = exec_us, .startup_us = fl.startup_us, .finish_us = finish_us,
    .handoff_wait_us = handoff_wait_us,
  };
  if (status == LF_RESULT_DIVERGED) snprintf(r.reason, sizeof(r.reason), "end state differs from boundary %u", next);
  lf_msg_send(lf_sock, &r);
  if (status == LF_RESULT_OK) lf_follower_exit(LF_FOLLOWER_EXIT_OK);

  // The leader diverged (or is gone): this follower holds the valid state at
  // boundary next and becomes the leader once the coordinator has committed
  // the slices before it and stopped the old generation.
  for (;;) {
    m = lf_follower_wait();
    if (m.type == LF_MSG_PREPARE) {
      if (setpgid(0, 0) != 0) lf_follower_fail("setpgid: %s", strerror(errno));
      lf_msg_t ready = { .type = LF_MSG_READY, .gen = fl.gen, .slice = fl.index };
      lf_msg_send(lf_sock, &ready);
    } else if (m.type == LF_MSG_PROMOTE) {
      Log("follower %u becomes the leader of generation %u at icount %lu", fl.index, m.gen, icount);
      lf_leader_start(m.gen, next, icount, run_ended);
      return;
    } else {
      lf_follower_fail("unexpected message %u", m.type);
    }
  }
}

#endif // CONFIG_LF_PROFILING
