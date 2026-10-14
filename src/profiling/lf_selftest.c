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

// Self-tests of leader-follower building blocks on the real guest memory
// mapping (--lf-selftest=NAME). They run after the image is loaded and exit.

#include <generated/autoconf.h>

#ifdef CONFIG_LF_PROFILING

#include <utils.h>
#include <memory/paddr.h>
#include "lf_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define GIB (1ul << 30)
#define PT_SPAN_PAGES 512  // pages covered by one last-level page table

// A written page may drag the rest of its newly allocated page table along
// (see lf_memtrack.h); accept exactly the written pages plus such neighbours.
static bool lf_selftest_expect(const lf_page_list_t *l, const uint64_t *want, int n) {
  for (int k = 0; k < n; k++) {
    bool found = false;
    for (size_t i = 0; i < l->n && !found; i++) found = l->pages[i] == want[k];
    if (!found) { printf("  page %lu is missing\n", want[k]); return false; }
  }
  for (size_t i = 0; i < l->n; i++) {
    bool near = false;
    for (int k = 0; k < n && !near; k++) near = l->pages[i] / PT_SPAN_PAGES == want[k] / PT_SPAN_PAGES;
    if (!near) { printf("  unexpected page %lu\n", l->pages[i]); return false; }
  }
  return true;
}

static int lf_selftest_memtrack(void) {
  uint8_t *pmem = guest_to_host(CONFIG_MBASE);
  uint64_t t0 = get_time();
  int rc = lf_open_pmem_tracker();
  uint64_t t_arm = get_time() - t0;
  if (rc != 0) { printf("open failed: %s\n", strerror(-rc)); return 1; }
  printf("memtrack: armed %lu MiB of guest memory mapping in %lu us\n", MEMORY_SIZE >> 20, t_arm);
  uint64_t pages[] = { 0, GIB >> LF_PAGE_SHIFT, (1024 * GIB) >> LF_PAGE_SHIFT };
  int np = 3;
  if ((pages[2] << LF_PAGE_SHIFT) >= MEMORY_SIZE) np = 2;
  for (int k = 0; k < np; k++) pmem[pages[k] << LF_PAGE_SHIFT] ^= 0x5a;
  lf_page_list_t l = {0};
  t0 = get_time();
  rc = lf_memtrack_collect(&lf_track, &l, true);
  uint64_t t_collect = get_time() - t0;
  bool ok = rc == 0 && lf_selftest_expect(&l, pages, np);
  printf("memtrack: parent collect %lu pages in %lu us: %s\n", l.n, t_collect, ok ? "ok" : "FAIL");

  fflush(stdout);
  pid_t pid = fork();
  if (pid == 0) {
    lf_memtrack_forget_inherited(&lf_track);
    if (lf_open_pmem_tracker() != 0) _exit(3);
    uint64_t mine[] = { (2 * GIB) >> LF_PAGE_SHIFT };
    pmem[mine[0] << LF_PAGE_SHIFT] ^= 0x5a;
    lf_page_list_t cl = {0};
    if (lf_memtrack_collect(&lf_track, &cl, false) != 0) _exit(4);
    bool cok = lf_selftest_expect(&cl, mine, 1);
    printf("memtrack: child collect %lu pages: %s\n", cl.n, cok ? "ok" : "FAIL");
    fflush(stdout);
    _exit(cok ? 0 : 5);
  }
  int st = 0;
  waitpid(pid, &st, 0);
  bool child_ok = WIFEXITED(st) && WEXITSTATUS(st) == 0;
  rc = lf_memtrack_collect(&lf_track, &l, false);
  bool parent_ok = rc == 0 && l.n == 0;
  printf("memtrack: parent after fork sees %lu written pages: %s\n", l.n, parent_ok ? "ok" : "FAIL");
  lf_page_list_free(&l);
  ok = ok && child_ok && parent_ok;
  printf("memtrack self-test: %s\n", ok ? "PASS" : "FAIL");
  return ok ? 0 : 1;
}

_Noreturn void lf_run_selftest(const char *name) {
  int rc;
  if (strcmp(name, "memtrack") == 0) rc = lf_selftest_memtrack();
  else { printf("unknown self-test '%s'\n", name); rc = 2; }
  fflush(stdout);
  exit(rc);
}

#endif // CONFIG_LF_PROFILING
