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

// Leader worker of leader-follower profiling: executes with profiling
// inactive, forks one follower per slice at eligible outer-loop tops, and
// reports to the coordinator. Should the leader diverge, its followers
// detect that and the coordinator rolls the run back.

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <generated/autoconf.h>

#ifdef CONFIG_LF_PROFILING

#include <isa.h>
#include <cpu/cpu.h>
#include <memory/paddr.h>
#include <profiling/lf_record.h>
#include "lf_internal.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

void set_mtime(uint64_t new_value);
void update_riscv_timer();
uint64_t clint_get_mtime();

static struct {
  uint32_t gen;
  uint32_t next_slice;       // slice of the next follower to fork
  int executing;             // forked followers that have not stopped yet
  uint64_t t_start;
  long minflt_start;
  uint64_t fork_us, fork_max_us, scan_us, scan_max_us, nr_scans, scanned_pages;
} ld;

__attribute__((format(printf, 1, 2), noreturn))
static void lf_leader_fail(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr, "leader-follower leader: ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  lf_follower_exit(LF_FOLLOWER_EXIT_ERROR);
}

// Handles the coordinator's messages; with block set, waits for at least one.
// Returns true when FINAL_OK arrived.
static bool lf_leader_poll(bool block) {
  bool final = false;
  lf_msg_t m;
  int r;
  while ((r = lf_msg_recv(lf_sock, &m, block)) != -1) {
    if (r == 0) lf_follower_exit(LF_FOLLOWER_EXIT_ORPHAN);  // the coordinator is gone
    switch (m.type) {
      case LF_MSG_SLOT: ld.executing--; break;
      case LF_MSG_FINAL_OK: final = true; break;
      case LF_MSG_EXIT: lf_follower_exit(LF_FOLLOWER_EXIT_OK);
      default: lf_leader_fail("unexpected message %u", m.type);
    }
    block = false;
  }
  return final;
}

// Collects the pages written since the previous boundary and re-protects them.
static void lf_leader_scan(void) {
  if (!lf_mem_tracking()) return;
  uint64_t t0 = get_time();
  int rc = lf_memtrack_collect(&lf_track, &lf_written, true);
  if (rc != 0) lf_leader_fail("memory scan failed: %s", strerror(-rc));
  uint64_t dt = get_time() - t0;
  ld.scan_us += dt;
  if (dt > ld.scan_max_us) ld.scan_max_us = dt;
  ld.nr_scans++;
  ld.scanned_pages += lf_written.n;
}

uint64_t lf_slice_end;

// Every boundary makes each page the leader writes afterwards cost a write
// fault (copy-on-write after the fork, and write tracking). A slice gets this
// many instructions per page written in the previous one, within the
// configured lengths, so that the faults stay a small share of the leader's
// time; lengths are multiples of the minimum.
#define LF_SLICE_INSTRS_PER_PAGE (1ull << 17)
// With an instruction limit, a slice is at most this fraction of what
// remains (but not shorter than this fraction of the minimum), so that the
// last followers finish soon after the leader.
#define LF_SLICE_REMAIN_DIV 16

uint64_t cpu_exec_lf_n_remain_total();

static uint64_t lf_next_slice_len(void) {
  uint64_t min = lf_config.slice_len, max = lf_config.slice_max;
  uint64_t len = lf_mem_tracking() ? lf_written.n * LF_SLICE_INSTRS_PER_PAGE : min;
  len = MIN_OF(MAX_OF(len, min), max) / min * min;
  uint64_t remain = cpu_exec_lf_n_remain_total();
  if (remain != (uint64_t)-1)
    len = MIN_OF(len, MAX_OF(remain / LF_SLICE_REMAIN_DIV, min / LF_SLICE_REMAIN_DIV));
  return len;
}

static void lf_fork_follower(uint64_t icount) {
  uint32_t slice = ld.next_slice++;
  lf_slice_end += lf_next_slice_len();
  fflush(NULL);
  uint64_t t0 = get_time();
  pid_t pid = fork();
  if (pid < 0) lf_leader_fail("fork: %s", strerror(errno));
  if (pid == 0) {
    lf_follower_start(ld.gen, slice, icount);
    return;  // the follower continues the guest from here
  }
  uint64_t dt = get_time() - t0;
  ld.fork_us += dt;
  if (dt > ld.fork_max_us) ld.fork_max_us = dt;
  ld.executing++;
  Log("leader-follower boundary %u at icount %lu (generation %u), next at %lu", slice, icount, ld.gen,
      checkpoint_icount_base + lf_slice_end);
}

// Leader-side fault injection at a loop top, after any fork there. Fires once
// per run: promoted leaders never inject.
static void lf_inject_leader(uint64_t icount) {
  if (lf_inject.fired || lf_inject.kind == LF_INJ_NONE || lf_inject_follower_kind(lf_inject.kind) ||
      icount < lf_inject.where)
    return;
  lf_inject.fired = true;
  switch (lf_inject.kind) {
    case LF_INJ_CSR: csr_array[0x33f] ^= 1; break;  // mhpmevent31, unused by the guests
    case LF_INJ_TIMER:  // moves the mtime correction
      update_riscv_timer();
      set_mtime(clint_get_mtime() + 1000);
      break;
    case LF_INJ_GPR: cpu.gpr[3]._64 ^= 1; break;    // gp
    case LF_INJ_MEM_HASH: {
      // Flip one byte of a page the leader already wrote in this slice.
      lf_page_list_t cur = {0};
      if (!lf_mem_tracking() || lf_memtrack_collect(&lf_track, &cur, false) != 0 || cur.n == 0)
        panic("mem-hash injection needs memory tracking and a written page");
      guest_to_host(CONFIG_MBASE)[cur.pages[0] << LF_PAGE_SHIFT] ^= 1;
      lf_page_list_free(&cur);
      break;
    }
    case LF_INJ_MEM_SET: guest_to_host(CONFIG_MBASE)[MEMORY_SIZE - LF_PAGE_SIZE] ^= 1; break;
    case LF_INJ_LEADER_CRASH:
      Log("leader-follower injected leader crash at icount %lu", icount);
      fflush(stdout);
      abort();
    default: panic("unexpected injection kind %d", lf_inject.kind);
  }
  Log("leader-follower injected fault kind %d at icount %lu", lf_inject.kind, icount);
}

void lf_leader_start(uint32_t gen, uint32_t next_slice, uint64_t icount, bool run_ended) {
  lf_role = LF_ROLE_LEADER;
  // The leader runs the fast path: the reference block cache with host
  // floating point. Followers disable host floating point for themselves.
  cpu_enter_leader_mode();
  ld = (typeof(ld)){ .gen = gen, .next_slice = next_slice, .t_start = get_time() };
  if (gen > 0) {
    // Promoted: the run's stdout, the leader's own error log, and write
    // tracking from this boundary on.
    if (dup2(lf_orig_stdout, STDOUT_FILENO) < 0) lf_follower_exit(LF_FOLLOWER_EXIT_ERROR);
    lf_redirect_leader_stderr(gen);
    profiling_state = NoProfiling;
    lf_inject.fired = true;
    if (lf_mem_tracking()) {
      lf_page_list_t discard = {0};
      if (lf_memtrack_collect(&lf_track, &discard, true) != 0) lf_leader_fail("memory scan failed");
      lf_page_list_free(&discard);
    }
  }
  lf_msg_t m = { .type = LF_MSG_HELLO, .role = LF_HELLO_LEADER, .gen = gen, .slice = next_slice,
                 .pid = getpid(), .start_icount = icount };
  lf_msg_send(lf_sock, &m);
  struct rusage ru;
  getrusage(RUSAGE_SELF, &ru);
  ld.minflt_start = ru.ru_minflt;
  Log("leader-follower leader of generation %u starts at icount %lu", gen, icount);
  if (!run_ended) lf_fork_follower(icount);
}

void lf_leader_loop_top(bool canonical, uint64_t icount) {
  if (lf_slice_ended(canonical, icount)) {
    lf_leader_poll(false);
    while (ld.executing >= lf_config.max_exec) lf_leader_poll(true);
    lf_leader_scan();
    // Followers that finished are reaped here; the coordinator learns about
    // them through their connections.
    while (waitpid(-1, NULL, WNOHANG) > 0) {}
    lf_fork_follower(icount);
    if (lf_role == LF_ROLE_FOLLOWER) return;
  }
  lf_inject_leader(icount);
}

// Publishes the record of the final boundary, reports the end of the run
// and waits until every slice is committed: only then may the run's final
// output be printed. A leader rolled back meanwhile is killed here.
void lf_leader_run_end(uint64_t icount) {
  lf_record_t rec;
  lf_record_capture(&rec, LF_REC_START, ld.next_slice, true);
  if (lf_mem_tracking()) {
    lf_leader_scan();
    lf_record_set_pages(&rec, &lf_written);
  }
  char path[PATH_MAX];
  lf_slice_path(path, sizeof(path), ld.next_slice, "start.rec");
  int rc = lf_record_write(path, &rec);
  lf_record_free(&rec);
  if (rc != 0) lf_leader_fail("cannot write %s: %s", path, strerror(-rc));
  struct rusage ru;
  getrusage(RUSAGE_SELF, &ru);
  lf_msg_t m = {
    .type = LF_MSG_LEADER_END, .gen = ld.gen, .slice = ld.next_slice, .end_icount = icount,
    .fork_us = ld.fork_us, .fork_max_us = ld.fork_max_us,
    .scan_us = ld.scan_us, .scan_max_us = ld.scan_max_us, .nr_scans = ld.nr_scans,
    .scanned_pages = ld.scanned_pages, .minflt = ru.ru_minflt - ld.minflt_start,
  };
  lf_msg_send(lf_sock, &m);
  Log("leader-follower run ended at icount %lu after %u slices", icount, ld.next_slice);
  while (!lf_leader_poll(true)) {}
}

#endif // CONFIG_LF_PROFILING
