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

// Leader-follower profiling: configuration, the coordinator process (slice
// table, commits, rollbacks, merge) and the connections to it.

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <generated/autoconf.h>

#ifdef CONFIG_LF_PROFILING

#include <isa.h>
#include <memory/paddr.h>
#include <profiling/lf_record.h>
#include "lf_internal.h"
#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <ucontext.h>
#include <unistd.h>

lf_config_t lf_config = {
  .requested = false,
  .slice_len = 1000000000ul,
  .max_exec = 4,
  .work_dir = NULL,
  .keep_files = false,
  .mem_track = LF_MEM_TRACK_UFFD,
  .inject = NULL,
  .selftest = NULL,
};
lf_role_t lf_role = LF_ROLE_NONE;
lf_inject_t lf_inject = { .kind = LF_INJ_NONE };

lf_memtrack_t lf_track = { .uffd = -1, .pagemap_fd = -1 };
lf_page_list_t lf_written;

int lf_open_pmem_tracker(void) {
  return lf_memtrack_open(&lf_track, guest_to_host(CONFIG_MBASE), MEMORY_SIZE);
}

__attribute__((format(printf, 1, 2), noreturn))
static void lf_config_error(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  fflush(stdout);
  fprintf(stderr, "leader-follower: ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  exit(1);
}

// Accepts a decimal integer, optionally in the form <mantissa>e<exponent>.
static uint64_t lf_parse_count(const char *name, const char *arg) {
  char *end;
  errno = 0;
  uint64_t v = strtoull(arg, &end, 10);
  if (end != arg && (*end == 'e' || *end == 'E')) {
    char *exp_end;
    unsigned long e = strtoul(end + 1, &exp_end, 10);
    if (exp_end == end + 1 || e > 19) lf_config_error("invalid value '%s' for --%s", arg, name);
    for (unsigned long i = 0; i < e; i++) {
      if (v > UINT64_MAX / 10) lf_config_error("value '%s' for --%s is too large", arg, name);
      v *= 10;
    }
    end = exp_end;
  }
  if (errno != 0 || end == arg || *end != '\0' || arg[0] == '-')
    lf_config_error("invalid value '%s' for --%s", arg, name);
  return v;
}

void lf_set_option(int code, const char *arg) {
  switch (code) {
    case LF_OPT_PROFILE: lf_config.requested = true; break;
    case LF_OPT_SLICE: lf_config.slice_len = lf_parse_count("lf-slice", arg); break;
    case LF_OPT_SLICE_MAX: lf_config.slice_max = lf_parse_count("lf-slice-max", arg); break;
    case LF_OPT_FOLLOWERS: {
      uint64_t n = lf_parse_count("lf-followers", arg);
      if (n > 4096) lf_config_error("--lf-followers=%s is too large", arg);
      lf_config.max_exec = (int)n;
      break;
    }
    case LF_OPT_DIR: lf_config.work_dir = arg; break;
    case LF_OPT_KEEP: lf_config.keep_files = true; break;
    case LF_OPT_MEM_TRACK:
      if (strcmp(arg, "uffd") == 0) lf_config.mem_track = LF_MEM_TRACK_UFFD;
      else if (strcmp(arg, "off") == 0) lf_config.mem_track = LF_MEM_TRACK_OFF;
      else lf_config_error("--lf-mem-track must be 'uffd' or 'off', not '%s'", arg);
      break;
    case LF_OPT_INJECT: lf_config.inject = arg; break;
    case LF_OPT_SELFTEST: lf_config.selftest = arg; break;
    case LF_OPT_FOLLOWER_CPUS:
      if (!lf_set_follower_cpus(arg)) lf_config_error("invalid CPU list '%s' for --lf-follower-cpus", arg);
      break;
    default: lf_config_error("unknown option code %d", code);
  }
}

static void lf_parse_inject(const char *spec) {
  static const struct { const char *name; lf_inject_kind_t kind; } kinds[] = {
    {"csr", LF_INJ_CSR}, {"timer", LF_INJ_TIMER}, {"gpr", LF_INJ_GPR},
    {"mem-hash", LF_INJ_MEM_HASH}, {"mem-set", LF_INJ_MEM_SET}, {"leader-crash", LF_INJ_LEADER_CRASH},
    {"crash", LF_INJ_CRASH},
    {"rec-gpr", LF_INJ_REC_GPR}, {"rec-csr", LF_INJ_REC_CSR}, {"rec-mtime", LF_INJ_REC_MTIME},
    {"rec-icount", LF_INJ_REC_ICOUNT}, {"rec-dev", LF_INJ_REC_DEV}, {"slow-hello", LF_INJ_SLOW_HELLO},
  };
  const char *at = strchr(spec, '@');
  if (at == NULL) lf_config_error("--lf-inject expects KIND@ICOUNT or KIND@sSLICE, not '%s'", spec);
  size_t len = at - spec;
  for (int i = 0; i < ARRLEN(kinds); i++) {
    if (strlen(kinds[i].name) != len || strncmp(spec, kinds[i].name, len) != 0) continue;
    lf_inject.kind = kinds[i].kind;
    bool follower = lf_inject_follower_kind(lf_inject.kind);
    if (follower != (at[1] == 's'))
      lf_config_error("--lf-inject=%s: use %s", spec, follower ? "KIND@sSLICE" : "KIND@ICOUNT");
    lf_inject.where = lf_parse_count("lf-inject", at + 1 + follower);
    return;
  }
  lf_config_error("--lf-inject: unknown kind in '%s'", spec);
}

static int lf_thread_count(void) {
  FILE *fp = fopen("/proc/self/status", "r");
  if (fp == NULL) return -1;
  char line[256];
  int n = -1;
  while (fgets(line, sizeof(line), fp)) {
    if (sscanf(line, "Threads: %d", &n) == 1) break;
  }
  fclose(fp);
  return n;
}

static void lf_validate(bool difftest_enabled) {
  extern int is_batch_mode();
  extern char *mapped_cpt_file;
  if (!is_batch_mode()) lf_config_error("--lf-profile requires batch mode (-b)");
  if (profiling_state != SimpointProfiling)
    lf_config_error("--lf-profile requires --simpoint-profile");
  if (checkpoint_state != NoCheckpoint)
    lf_config_error("--lf-profile cannot be combined with checkpointing");
  if (checkpoint_interval == 0) lf_config_error("--lf-profile requires --cpt-interval > 0");
  if (difftest_enabled) lf_config_error("--lf-profile cannot be combined with difftest");
  if (mapped_cpt_file != NULL) lf_config_error("--lf-profile cannot be combined with --map-cpt");
  if (lf_config.slice_len == 0) lf_config_error("--lf-slice must be greater than 0");
  if (lf_config.slice_max == 0) lf_config.slice_max = lf_config.slice_len;
  if (lf_config.slice_max < lf_config.slice_len) lf_config_error("--lf-slice-max must be at least --lf-slice");
  if (lf_config.max_exec < 1) lf_config_error("--lf-followers must be at least 1");
  IFDEF(CONFIG_HAS_SDCARD, extern bool sdcard_has_image());
  if (MUXDEF(CONFIG_HAS_SDCARD, sdcard_has_image(), false))
    lf_config_error("--lf-profile cannot be used with an sdcard image");
  if (lf_config.inject) lf_parse_inject(lf_config.inject);
  int threads = lf_thread_count();
  if (threads != 1) lf_config_error("--lf-profile needs a single-threaded process (found %d threads)", threads);
}

// ---------------------------------------------------------------------------
// Paths, output and connections

char lf_work_dir[PATH_MAX];
char lf_coord_addr[108];
int lf_sock = -1;
int lf_orig_stdout = -1;

void lf_work_path(char *buf, size_t len, const char *name) {
  int n = snprintf(buf, len, "%s/%s", lf_work_dir, name);
  Assert(n > 0 && (size_t)n < len, "work file path too long");
}

void lf_slice_path(char *buf, size_t len, uint32_t slice, const char *suffix) {
  char name[64];
  snprintf(name, sizeof(name), "slice-%06u.%s", slice, suffix);
  lf_work_path(buf, len, name);
}

void lf_redirect_child_output(const char *path, const char *stderr_path) {
  int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
  int efd = stderr_path ? open(stderr_path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644) : fd;
  if (fd < 0 || efd < 0 || dup2(fd, STDOUT_FILENO) < 0 || dup2(efd, STDERR_FILENO) < 0)
    _exit(LF_FOLLOWER_EXIT_ERROR);
  close(fd);
  if (efd != fd) close(efd);
  extern void log_detach_for_child();
  log_detach_for_child();
}

void lf_leader_err_path(char *buf, size_t len, uint32_t gen) {
  char name[64];
  snprintf(name, sizeof(name), "leader-%u.err", gen);
  lf_work_path(buf, len, name);
}

// The leader's console output may be speculative (followers provide the
// run's), so its stderr goes to a file of its generation.
void lf_redirect_leader_stderr(uint32_t gen) {
  char err[PATH_MAX];
  lf_leader_err_path(err, sizeof(err), gen);
  int fd = open(err, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
  if (fd < 0 || dup2(fd, STDERR_FILENO) < 0) lf_follower_exit(LF_FOLLOWER_EXIT_ERROR);
  close(fd);
}

// A worker that dies of a fault writes the signal, the faulting address and
// a backtrace to its stdout (the run's stdout for a leader, the slice log for
// a follower); the default action then ends it.
static void lf_crash_report(int sig, siginfo_t *si, void *ctx) {
  static const char *roles[] = { [LF_ROLE_NONE] = "worker", [LF_ROLE_LEADER] = "leader",
                                 [LF_ROLE_FOLLOWER] = "follower" };
  unsigned long pc = 0;
#if defined(__x86_64__)
  pc = ((ucontext_t *)ctx)->uc_mcontext.gregs[REG_RIP];
#endif
  char buf[192];
  int n = snprintf(buf, sizeof(buf), "leader-follower %s %d: signal %d at address %p, host pc %#lx\n",
                   roles[lf_role], getpid(), sig, si->si_addr, pc);
  if (n >= (int)sizeof(buf)) n = sizeof(buf) - 1;
  if (n > 0 && write(STDOUT_FILENO, buf, n) < 0) {}
  void *frames[64];
  backtrace_symbols_fd(frames, backtrace(frames, 64), STDOUT_FILENO);
}

static void lf_install_crash_report(void) {
  static char alt[65536];
  stack_t ss = { .ss_sp = alt, .ss_size = sizeof(alt) };
  sigaltstack(&ss, NULL);
  void *warm[1];
  backtrace(warm, 1);  // loads the unwinder now, not in the handler
  struct sigaction sa = { .sa_sigaction = lf_crash_report, .sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESETHAND };
  sigemptyset(&sa.sa_mask);
  int sigs[] = { SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT };
  for (size_t i = 0; i < sizeof(sigs) / sizeof(sigs[0]); i++) sigaction(sigs[i], &sa, NULL);
}

static socklen_t lf_coord_sockaddr(struct sockaddr_un *a) {
  memset(a, 0, sizeof(*a));
  a->sun_family = AF_UNIX;
  // Abstract name: a leading NUL, no file.
  size_t n = strlen(lf_coord_addr + 1) + 1;
  memcpy(a->sun_path, lf_coord_addr, n);
  return offsetof(struct sockaddr_un, sun_path) + n;
}

int lf_coord_connect(void) {
  int fd = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
  if (fd < 0) return -1;
  struct sockaddr_un a;
  socklen_t len = lf_coord_sockaddr(&a);
  if (connect(fd, (struct sockaddr *)&a, len) != 0) { close(fd); return -1; }
  return fd;
}

void lf_msg_send(int fd, const lf_msg_t *m) {
  ssize_t n;
  do { n = send(fd, m, sizeof(*m), MSG_NOSIGNAL); } while (n < 0 && errno == EINTR);
  // The peer is gone: a worker's coordinator, or a worker being stopped.
  if (n != sizeof(*m) && lf_role != LF_ROLE_NONE) lf_follower_exit(LF_FOLLOWER_EXIT_ORPHAN);
}

int lf_msg_recv(int fd, lf_msg_t *m, bool block) {
  ssize_t n;
  do { n = recv(fd, m, sizeof(*m), block ? 0 : MSG_DONTWAIT); } while (n < 0 && errno == EINTR);
  if (n == 0) return 0;
  if (n != sizeof(*m)) return -1;
  return 1;
}

static void lf_make_work_dir(void) {
  if (lf_config.work_dir != NULL) {
    snprintf(lf_work_dir, PATH_MAX, "%s", lf_config.work_dir);
  } else {
    char out[PATH_MAX];
    if (simpoint_lf_output_dir(out, sizeof(out)) != 0)
      lf_config_error("cannot determine the SimPoint output directory");
    snprintf(lf_work_dir, PATH_MAX, "%s/lf", out);
  }
  if (mkdir(lf_work_dir, 0755) != 0 && errno != EEXIST)
    lf_config_error("cannot create work directory %s: %s", lf_work_dir, strerror(errno));
}

void lf_init(bool difftest_enabled) {
  if (lf_config.selftest) lf_run_selftest(lf_config.selftest);
  if (!lf_config.requested) {
    if (lf_config.inject) lf_config_error("--lf-inject requires --lf-profile");
    return;
  }
  lf_validate(difftest_enabled);
  lf_make_work_dir();
  // Workers execute with profiling inactive until they become followers.
  profiling_state = NoProfiling;
  lf_role = LF_ROLE_LEADER;
  Log("leader-follower mode configured: slice %lu to %lu instructions, %d followers, work dir %s",
      lf_config.slice_len, lf_config.slice_max, lf_config.max_exec, lf_work_dir);
}

// ---------------------------------------------------------------------------
// Merger process
//
// Fragments are merged in slice order by a separate process while the run
// goes on, so that the coordinator only hands them over.

typedef struct {
  uint32_t slice;
  uint32_t finish;           // no more fragments
  uint64_t start_icount, end_icount;
} lf_merge_req_t;

_Noreturn static void lf_merger_main(int req_fd) {
  char dir[PATH_MAX], out[PATH_MAX + 32], err[256] = "", ferr[256];
  if (simpoint_lf_output_dir(dir, sizeof(dir)) != 0) _exit(LF_FOLLOWER_EXIT_ERROR);
  snprintf(out, sizeof(out), "%s/simpoint_bbv.gz", dir);
  void *m = lf_merger_open(out);
  uint32_t merged = 0;
  for (;;) {
    lf_merge_req_t r;
    size_t got = 0;
    while (got < sizeof(r)) {
      ssize_t n = read(req_fd, (char *)&r + got, sizeof(r) - got);
      if (n < 0 && errno == EINTR) continue;
      if (n <= 0) {
        if (err[0] == '\0') snprintf(err, sizeof(err), "the leader closed the request pipe");
        r.finish = 1;
        break;
      }
      got += n;
    }
    if (r.finish) break;
    if (err[0] != '\0') continue;  // keep draining requests after a failure
    char path[PATH_MAX];
    lf_slice_path(path, sizeof(path), r.slice, "bbvf");
    if (lf_merger_add(m, path, r.slice, r.start_icount, r.end_icount, err, sizeof(err)) != 0) continue;
    merged++;
    if (!lf_config.keep_files) unlink(path);
  }
  int rc = lf_merger_finish(m, err[0] ? err : NULL, ferr, sizeof(ferr));
  if (rc != 0) fprintf(stderr, "merge failed: %s\n", ferr);
  else printf("merged %u fragments into %s\n", merged, out);
  fflush(stdout);
  fflush(stderr);
  _exit(rc == 0 ? 0 : LF_FOLLOWER_EXIT_ERROR);
}

static void lf_merger_start(pid_t *pid_out, int *wr_out) {
  int req[2];
  if (pipe2(req, O_CLOEXEC) != 0) panic("pipe: %s", strerror(errno));
  fflush(NULL);
  pid_t pid = fork();
  if (pid < 0) panic("fork: %s", strerror(errno));
  if (pid == 0) {
    prctl(PR_SET_PDEATHSIG, SIGKILL);
    lf_apply_follower_cpus();
    close(req[1]);
    char log[PATH_MAX];
    lf_work_path(log, sizeof(log), "merger.log");
    lf_redirect_child_output(log, NULL);
    lf_merger_main(req[0]);
  }
  close(req[0]);
  *pid_out = pid;
  *wr_out = req[1];
}

// ---------------------------------------------------------------------------
// Coordinator
//
// Slices are committed in order. A slice whose follower validated its end
// state is committed; one whose follower found a different state (or no
// state, the leader being gone) is committed too, since its own execution is
// valid, and the run is rolled back: the leader's generation is stopped and
// that follower becomes the leader of the next generation at the boundary
// where it stopped.

typedef struct {
  bool known;                // a follower of the current generation said hello
  pid_t pid;
  int conn;                  // -1 once closed
  uint64_t start_icount;
  bool record;               // its start record is published
  bool exec_done, result_seen, waiting;
  lf_msg_t done, result;
} lf_slot_t;

#define LF_MAX_CONNS 8192

static struct {
  int listen_fd, sig_fd;
  uint32_t gen;
  pid_t leader_pid;          // current leader, 0 when lost
  pid_t leader_pgid;         // process group of the current generation
  int leader_conn;
  bool leader_ended;         // LEADER_END received from the current leader
  uint32_t final_slice;      // number of slices of the ended run
  uint64_t final_icount;
  lf_msg_t leader_stats;
  int status_final;          // exit status of the final leader, -1 until it exits
  lf_slot_t *s;              // indexed by slice
  uint32_t cap, nr;          // nr: slices of the current generation
  uint32_t next_commit;
  pid_t merger_pid;
  int merger_wr;
  int pending[LF_MAX_CONNS]; // connections that have not said hello
  int nr_pending;
  uint32_t rollbacks;
  uint64_t t_start, t_leader_end, t_end, lost_us;
  // follower statistics over committed slices
  uint64_t f_exec_us, f_instrs, f_startup_sum, f_startup_max, f_finish_sum, f_finish_max, f_ho_sum, f_ho_max;
  double f_mips_min;
} co;

static lf_slot_t *co_slot(uint32_t k) {
  if (k >= co.cap) {
    uint32_t cap = co.cap ? co.cap : 256;
    while (cap <= k) cap *= 2;
    co.s = realloc(co.s, cap * sizeof(lf_slot_t));
    Assert(co.s, "out of memory");
    for (uint32_t i = co.cap; i < cap; i++) co.s[i] = (lf_slot_t){ .conn = -1 };
    co.cap = cap;
  }
  return &co.s[k];
}

static void co_send(int fd, uint32_t type, uint32_t gen) {
  if (fd < 0) return;
  lf_msg_t m = { .type = type, .gen = gen };
  lf_msg_send(fd, &m);
}

static void co_reap(bool block);

// Stops every worker and the merger and exits: a follower failed, so there
// is no valid state to continue from.
__attribute__((format(printf, 2, 3), noreturn))
static void co_abort(int slice, const char *fmt, ...) {
  co_reap(false);
  if (co.leader_pgid > 0) kill(-co.leader_pgid, SIGKILL);
  for (uint32_t k = 0; k < co.nr; k++) if (co.s[k].known && co.s[k].pid > 0) kill(co.s[k].pid, SIGKILL);
  if (co.merger_pid > 0) kill(co.merger_pid, SIGKILL);
  char msg[512];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(msg, sizeof(msg), fmt, ap);
  va_end(ap);
  fflush(stdout);
  if (slice >= 0) {
    char log[PATH_MAX];
    lf_slice_path(log, sizeof(log), slice, "log");
    fprintf(stderr, "leader-follower: follower failure in slice %d: %s (log: %s)\n", slice, msg, log);
  } else {
    fprintf(stderr, "leader-follower: %s\n", msg);
  }
  fprintf(stderr, "leader-follower: work files kept in %s\n", lf_work_dir);
  fflush(stderr);
  exit(LF_RUN_EXIT_ERROR);
}

// The next record of follower k is available: tell it, if it waits for it.
static void co_offer_record(uint32_t k) {
  lf_slot_t *f = co_slot(k);
  if (!f->known || !f->exec_done || !f->waiting) return;
  bool next_ready = (k + 1 < co.nr && co.s[k + 1].record) || (co.leader_ended && co.final_slice == k + 1);
  if (next_ready) {
    co_send(f->conn, LF_MSG_RECORD_READY, co.gen);
    f->waiting = false;
  } else if (co.leader_pid == 0 && k + 1 == co.nr) {
    // The leader is gone and nobody will publish boundary k + 1.
    co_send(f->conn, LF_MSG_NO_RECORD, co.gen);
    f->waiting = false;
  }
}

// Appends the guest console of slice k to the run's stderr.
static void co_emit_console(uint32_t k) {
  char path[PATH_MAX];
  lf_slice_path(path, sizeof(path), k, "console");
  int fd = open(path, O_RDONLY | O_CLOEXEC);
  if (fd < 0) co_abort(k, "cannot open %s: %s", path, strerror(errno));
  char buf[65536];
  ssize_t n;
  while ((n = read(fd, buf, sizeof(buf))) > 0) {
    for (ssize_t off = 0; off < n;) {
      ssize_t w = write(STDERR_FILENO, buf + off, n - off);
      if (w <= 0) break;
      off += w;
    }
  }
  close(fd);
}

static void co_commit(uint32_t k) {
  lf_slot_t *f = &co.s[k];
  lf_merge_req_t r = { .slice = k, .start_icount = f->start_icount, .end_icount = f->done.end_icount };
  if (write(co.merger_wr, &r, sizeof(r)) != sizeof(r)) co_abort(-1, "BBV merger request: %s", strerror(errno));
  co_emit_console(k);
  uint64_t instrs = f->done.end_icount - f->start_icount;
  const lf_msg_t *res = &f->result;
  co.f_instrs += instrs;
  co.f_exec_us += res->exec_us;
  co.f_startup_sum += res->startup_us;
  co.f_finish_sum += res->finish_us;
  co.f_ho_sum += res->handoff_wait_us;
  if (res->startup_us > co.f_startup_max) co.f_startup_max = res->startup_us;
  if (res->finish_us > co.f_finish_max) co.f_finish_max = res->finish_us;
  if (res->handoff_wait_us > co.f_ho_max) co.f_ho_max = res->handoff_wait_us;
  double mips = res->exec_us > 1000 ? (double)instrs / res->exec_us : 0;
  if (mips > 0 && (co.f_mips_min == 0 || mips < co.f_mips_min)) co.f_mips_min = mips;
  co.next_commit = k + 1;
}

// Follower k becomes the leader of the next generation at the boundary where
// it stopped; everything the old generation did after it is dropped.
static void co_rollback(uint32_t k) {
  uint64_t t0 = get_time();
  lf_slot_t *f = &co.s[k];
  co.rollbacks++;
  Log("leader-follower rollback %u: slice %u %s; generation %u restarts at icount %lu",
      co.rollbacks, k, f->result.status == LF_RESULT_DIVERGED ? "diverged" : "has no next record",
      co.gen + 1, f->done.end_icount);
  co_send(f->conn, LF_MSG_PREPARE, co.gen);
  lf_msg_t m;
  do {
    if (lf_msg_recv(f->conn, &m, true) != 1) co_abort(k, "follower exited while becoming the leader");
  } while (m.type != LF_MSG_READY);
  // The old leader and all its other followers share its process group.
  if (co.leader_pgid > 0) kill(-co.leader_pgid, SIGKILL);
  if (co.leader_conn >= 0 && co.leader_conn != f->conn) close(co.leader_conn);
  for (uint32_t j = k + 1; j < co.nr; j++) {
    if (co.s[j].conn >= 0) close(co.s[j].conn);
    if (co.s[j].known && co.s[j].pid > 0) kill(co.s[j].pid, SIGKILL);
    co.s[j] = (lf_slot_t){ .conn = -1 };
  }
  for (int i = 0; i < co.nr_pending; i++) close(co.pending[i]);
  co.nr_pending = 0;
  co.nr = k + 1;
  co.gen++;
  co.leader_pid = f->pid;
  co.leader_pgid = f->pid;
  co.leader_conn = f->conn;
  co.leader_ended = false;
  f->conn = -1;
  co_send(co.leader_conn, LF_MSG_PROMOTE, co.gen);
  co.lost_us += get_time() - t0;
}

static void co_try_commit(void) {
  while (co.next_commit < co.nr && co.s[co.next_commit].result_seen) {
    uint32_t k = co.next_commit;
    int st = co.s[k].result.status;
    if (st == LF_RESULT_ERROR) co_abort(k, "%s", co.s[k].result.reason);
    co_commit(k);
    if (st != LF_RESULT_OK) { co_rollback(k); return; }
  }
  if (co.leader_ended && co.next_commit == co.final_slice && co.nr == co.final_slice) {
    co_send(co.leader_conn, LF_MSG_FINAL_OK, co.gen);
    co.leader_ended = false;  // sent once
    co.t_end = get_time();
  }
}

static void co_close_slot(uint32_t k) {
  lf_slot_t *f = &co.s[k];
  if (f->conn >= 0) close(f->conn);
  f->conn = -1;
}

static void co_on_follower_msg(uint32_t k, const lf_msg_t *m) {
  lf_slot_t *f = &co.s[k];
  switch (m->type) {
    case LF_MSG_RECORD:
      f->record = true;
      if (k > 0) co_offer_record(k - 1);
      break;
    case LF_MSG_EXEC_DONE:
      f->done = *m;
      f->exec_done = true;
      f->waiting = true;
      co_send(co.leader_conn, LF_MSG_SLOT, co.gen);
      co_offer_record(k);
      break;
    case LF_MSG_RESULT:
      f->result = *m;
      f->result_seen = true;
      if (m->status == LF_RESULT_OK) co_close_slot(k);
      co_try_commit();
      break;
    default:
      co_abort(k, "unexpected message %u", m->type);
  }
}

static void co_on_leader_lost(void) {
  if (co.t_end != 0) {
    // The final leader finishing the run after FINAL_OK.
    close(co.leader_conn);
    co.leader_conn = -1;
    return;
  }
  if (co.leader_pid == 0) return;
  Log("leader-follower: the leader of generation %u is gone", co.gen);
  co.leader_pid = 0;
  if (co.leader_conn >= 0) close(co.leader_conn);
  co.leader_conn = -1;
  if (co.nr == 0) co_abort(-1, "the leader exited before forking a follower");
  co_offer_record(co.nr - 1);
}

static void co_on_leader_msg(const lf_msg_t *m) {
  // A promoted follower announces itself on the connection it already has.
  if (m->type == LF_MSG_HELLO && m->role == LF_HELLO_LEADER && m->gen == co.gen) return;
  if (m->type != LF_MSG_LEADER_END) co_abort(-1, "unexpected leader message %u", m->type);
  co.leader_ended = true;
  co.final_slice = m->slice;
  co.final_icount = m->end_icount;
  co.leader_stats = *m;
  co.t_leader_end = get_time();
  if (m->slice > 0) co_offer_record(m->slice - 1);
  co_try_commit();
}

static void co_on_hello(int fd, const lf_msg_t *m) {
  if (m->gen != co.gen) {
    // A worker of a generation that was rolled back.
    if (m->pid > 0) kill(m->pid, SIGKILL);
    close(fd);
    return;
  }
  if (m->role == LF_HELLO_LEADER) {
    if (co.leader_conn >= 0 && co.leader_conn != fd) close(co.leader_conn);
    co.leader_conn = fd;
    co.leader_pid = m->pid;
    return;
  }
  // Followers connect on their own, so consecutive ones may say hello in
  // either order; commits stay in slice order regardless.
  lf_slot_t *f = co_slot(m->slice);
  if (f->known || m->slice < co.next_commit) co_abort(m->slice, "second follower for slice %u", m->slice);
  *f = (lf_slot_t){ .known = true, .pid = m->pid, .conn = fd, .start_icount = m->start_icount };
  if (m->slice + 1 > co.nr) co.nr = m->slice + 1;
  // Slice 0 needs no start record; every later slice publishes one.
  if (m->slice == 0) f->record = true;
}

// Reaps exited descendants; records the exit status of the final leader.
static void co_reap(bool block) {
  for (;;) {
    int st;
    pid_t p = waitpid(-1, &st, block ? 0 : WNOHANG);
    if (p <= 0) return;
    block = false;
    // Rolled-back workers are killed with SIGKILL; anything else is reported.
    if (WIFSIGNALED(st) && WTERMSIG(st) != SIGKILL)
      Log("leader-follower: process %d ended by signal %d (%s)", p, WTERMSIG(st), strsignal(WTERMSIG(st)));
    else if (WIFEXITED(st) && WEXITSTATUS(st) != 0 && p != co.leader_pid)
      Log("leader-follower: process %d exited with status %d", p, WEXITSTATUS(st));
    if (p == co.merger_pid) continue;
    if (p == co.leader_pid && co.t_end != 0) {
      co.status_final = WIFEXITED(st) ? WEXITSTATUS(st) : 128 + WTERMSIG(st);
      return;
    }
  }
}

static double co_mips(uint64_t instrs, uint64_t us) { return us ? (double)instrs / us : 0.0; }

static void co_print_summary(uint64_t merge_us) {
  uint32_t n = co.final_slice ? co.final_slice : 1;
  uint64_t wall = co.t_leader_end - co.t_start;
  uint64_t first = co.s[0].start_icount;
  const lf_msg_t *ls = &co.leader_stats;
  uint64_t nr_scans = ls->nr_scans ? ls->nr_scans : 1;
  Log("leader-follower summary: %u slices, leader exec %lu us at %.1f MIPS, final wait %lu us, merge %lu us",
      co.final_slice, wall, co_mips(co.final_icount - first, wall), co.t_end - co.t_leader_end, merge_us);
  Log("leader-follower summary: %u rollbacks (%lu us), final leader: fork avg %lu us max %lu us, "
      "scan avg %lu us max %lu us (%lu pages), minor faults %ld",
      co.rollbacks, co.lost_us, ls->fork_us / n, ls->fork_max_us, ls->scan_us / nr_scans, ls->scan_max_us,
      ls->scanned_pages, ls->minflt);
  Log("leader-follower summary: followers exec %.1f MIPS avg %.1f MIPS min, startup avg %lu us max %lu us, "
      "finish avg %lu us max %lu us, handoff wait avg %lu us max %lu us",
      co_mips(co.f_instrs, co.f_exec_us), co.f_mips_min, co.f_startup_sum / n, co.f_startup_max,
      co.f_finish_sum / n, co.f_finish_max, co.f_ho_sum / n, co.f_ho_max);
}

static void co_remove_work_files(void) {
  static const char *suffixes[] = { "start.rec", "end.rec", "bbvf", "log", "console" };
  char path[PATH_MAX];
  for (uint32_t i = 0; i <= co.final_slice; i++) {
    for (int k = 0; k < ARRLEN(suffixes); k++) {
      lf_slice_path(path, sizeof(path), i, suffixes[k]);
      unlink(path);
    }
  }
  lf_work_path(path, sizeof(path), "merger.log");
  unlink(path);
  for (uint32_t g = 0; g <= co.gen; g++) {
    lf_leader_err_path(path, sizeof(path), g);
    unlink(path);
  }
}

static void co_write_manifest(void) {
  char path[PATH_MAX];
  lf_work_path(path, sizeof(path), "manifest.txt");
  FILE *fp = fopen(path, "w");
  if (fp == NULL) co_abort(-1, "cannot write %s: %s", path, strerror(errno));
  fprintf(fp, "slices %u\nslice_len %lu\nicount_base %lu\nrollbacks %u\n", co.final_slice, lf_config.slice_len,
          checkpoint_icount_base, co.rollbacks);
  for (uint32_t i = 0; i < co.final_slice; i++) fprintf(fp, "boundary %u %lu\n", i, co.s[i].start_icount);
  fprintf(fp, "boundary %u %lu\n", co.final_slice, co.final_icount);
  fclose(fp);
}

_Noreturn void lf_coordinator_main(pid_t leader_pid) {
  co.leader_pid = leader_pid;
  co.leader_pgid = leader_pid;
  co.leader_conn = -1;
  co.status_final = -1;
  co.t_start = get_time();
  lf_merger_start(&co.merger_pid, &co.merger_wr);
  while (co.status_final < 0) {
    struct pollfd pfd[LF_MAX_CONNS];
    int src[LF_MAX_CONNS];   // -3 listen, -2 signals, -1 leader, >= 0 slice; pending: -(4 + i)
    int n = 0;
    pfd[n] = (struct pollfd){ .fd = co.listen_fd, .events = POLLIN }; src[n++] = -3;
    pfd[n] = (struct pollfd){ .fd = co.sig_fd, .events = POLLIN }; src[n++] = -2;
    if (co.leader_conn >= 0) { pfd[n] = (struct pollfd){ .fd = co.leader_conn, .events = POLLIN }; src[n++] = -1; }
    for (int i = 0; i < co.nr_pending && n < LF_MAX_CONNS; i++) {
      pfd[n] = (struct pollfd){ .fd = co.pending[i], .events = POLLIN }; src[n++] = -(4 + i);
    }
    for (uint32_t k = co.next_commit; k < co.nr && n < LF_MAX_CONNS; k++) {
      if (co.s[k].conn >= 0) { pfd[n] = (struct pollfd){ .fd = co.s[k].conn, .events = POLLIN }; src[n++] = k; }
    }
    int r;
    do { r = poll(pfd, n, -1); } while (r < 0 && errno == EINTR);
    if (r < 0) co_abort(-1, "poll: %s", strerror(errno));
    int nr_pending_before = co.nr_pending;
    uint32_t gen_before = co.gen;
    for (int i = 0; i < n && co.status_final < 0; i++) {
      if (pfd[i].revents == 0) continue;
      if (co.gen != gen_before) break;   // a rollback changed the connections
      lf_msg_t m;
      if (src[i] == -3) {
        int fd = accept4(co.listen_fd, NULL, NULL, SOCK_CLOEXEC);
        if (fd >= 0) {
          if (co.nr_pending == LF_MAX_CONNS) co_abort(-1, "too many connections");
          co.pending[co.nr_pending++] = fd;
        }
      } else if (src[i] == -2) {
        struct signalfd_siginfo si;
        while (read(co.sig_fd, &si, sizeof(si)) == sizeof(si)) {}
        co_reap(false);
      } else if (src[i] == -1) {
        int rr = lf_msg_recv(co.leader_conn, &m, false);
        if (rr == 0) co_on_leader_lost();
        else if (rr == 1) co_on_leader_msg(&m);
      } else if (src[i] <= -4) {
        int idx = -(src[i] + 4);
        if (idx >= nr_pending_before || idx >= co.nr_pending) continue;
        int fd = co.pending[idx];
        int rr = lf_msg_recv(fd, &m, false);
        if (rr == -1) continue;
        co.pending[idx] = co.pending[--co.nr_pending];
        nr_pending_before = -1;  // indices moved: handle the rest next round
        if (rr == 0 || m.type != LF_MSG_HELLO) { close(fd); break; }
        co_on_hello(fd, &m);
        break;
      } else {
        uint32_t k = src[i];
        if (k >= co.nr || co.s[k].conn < 0) continue;
        int rr = lf_msg_recv(co.s[k].conn, &m, false);
        if (rr == 0) {
          lf_slot_t *f = &co.s[k];
          co_close_slot(k);
          if (!f->result_seen || f->result.status != LF_RESULT_OK)
            co_abort(k, "follower exited without a result");
        } else if (rr == 1) {
          co_on_follower_msg(k, &m);
        }
      }
    }
    if (co.t_end != 0) co_reap(false);
  }
  // The final leader printed the run's output and exited; finish the merge.
  co_write_manifest();
  uint64_t t0 = get_time();
  lf_merge_req_t fin = { .finish = 1 };
  if (write(co.merger_wr, &fin, sizeof(fin)) != sizeof(fin)) co_abort(-1, "BBV merger request: %s", strerror(errno));
  close(co.merger_wr);
  int st;
  pid_t r;
  do { r = waitpid(co.merger_pid, &st, 0); } while (r < 0 && errno == EINTR);
  co.merger_pid = 0;
  if (r < 0 || !WIFEXITED(st) || WEXITSTATUS(st) != 0) {
    char log[PATH_MAX];
    lf_work_path(log, sizeof(log), "merger.log");
    co_abort(-1, "BBV merge failed (see %s)", log);
  }
  co_print_summary(get_time() - t0);
  Log("leader-follower merged %u fragments", co.final_slice);
  if (!lf_config.keep_files) co_remove_work_files();
  fflush(stdout);
  fflush(stderr);
  _exit(co.status_final);
}

// ---------------------------------------------------------------------------
// Hooks

// The original process becomes the coordinator; its child is the leader of
// generation 0, which forks follower 0 right here, before the execution
// context below is pushed.
void lf_on_run_entry(uint64_t icount) {
  if (lf_role != LF_ROLE_LEADER) return;
  signal(SIGPIPE, SIG_IGN);
  snprintf(lf_coord_addr + 1, sizeof(lf_coord_addr) - 1, "nemu-lf-%d", getpid());
  lf_coord_addr[0] = '\0';
  co.listen_fd = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
  struct sockaddr_un a;
  socklen_t len = lf_coord_sockaddr(&a);
  if (co.listen_fd < 0 || bind(co.listen_fd, (struct sockaddr *)&a, len) != 0 || listen(co.listen_fd, 256) != 0)
    lf_config_error("coordinator socket: %s", strerror(errno));
  // Descendants whose parent (a rolled-back leader) died are reaped here.
  prctl(PR_SET_CHILD_SUBREAPER, 1);
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGCHLD);
  sigprocmask(SIG_BLOCK, &mask, NULL);
  co.sig_fd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
  lf_orig_stdout = dup(STDOUT_FILENO);
  fflush(NULL);
  pid_t pid = fork();
  if (pid < 0) lf_config_error("fork: %s", strerror(errno));
  if (pid == 0) {
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
    close(co.listen_fd);
    close(co.sig_fd);
    setpgid(0, 0);
    lf_redirect_leader_stderr(0);
    lf_install_crash_report();  // inherited by every follower and later leader
    lf_sock = lf_coord_connect();
    if (lf_sock < 0) lf_follower_exit(LF_FOLLOWER_EXIT_ORPHAN);
    if (lf_mem_tracking()) {
      uint64_t t0 = get_time();
      int rc = lf_open_pmem_tracker();
      if (rc != 0) {
        fprintf(stdout, "leader-follower: memory tracking unavailable (%s); use --lf-mem-track=off\n", strerror(-rc));
        lf_follower_exit(LF_FOLLOWER_EXIT_ERROR);
      }
      Log("leader-follower memory tracking armed in %lu us", get_time() - t0);
    }
    lf_leader_start(0, 0, icount, false);
    return;
  }
  // The coordinator needs no guest memory; dropping it spares the leader
  // copy-on-write faults.
  munmap(guest_to_host(CONFIG_MBASE), MEMORY_SIZE);
  lf_role = LF_ROLE_NONE;
  lf_coordinator_main(pid);
}

void lf_on_loop_top(bool canonical, uint64_t icount) {
  if (lf_role == LF_ROLE_FOLLOWER) lf_follower_loop_top(canonical, icount);
  // A follower promoted there continues as the leader.
  if (lf_role == LF_ROLE_LEADER) lf_leader_loop_top(canonical, icount);
}

void lf_on_run_end(uint64_t icount) {
  if (lf_role == LF_ROLE_FOLLOWER) lf_follower_finish(icount, true);
  if (lf_role == LF_ROLE_LEADER) lf_leader_run_end(icount);
}

#endif // CONFIG_LF_PROFILING
