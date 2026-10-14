// Unit test of the leader-follower page write tracker (userfaultfd
// write-protect in asynchronous mode plus PAGEMAP_SCAN).

#define _GNU_SOURCE
#include <profiling/lf_memtrack.h>
#include <fcntl.h>
#include <linux/userfaultfd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

static int failures = 0;

#define CHECK(cond, ...)                                                       \
  do {                                                                         \
    if (!(cond)) {                                                             \
      printf("  FAIL: " __VA_ARGS__);                                          \
      printf("\n");                                                            \
      failures++;                                                              \
    }                                                                          \
  } while (0)

static double now_ms(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1e3 + tv.tv_usec / 1e3;
}

static uint8_t *map_region(size_t len) {
  void *p = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
  if (p == MAP_FAILED) { perror("mmap"); exit(2); }
  return p;
}

static void touch(uint8_t *base, uint64_t page) { base[page * LF_PAGE_SIZE + 17]++; }
static volatile uint8_t sink;
static void peek(uint8_t *base, uint64_t page) { sink = base[page * LF_PAGE_SIZE + 5]; }

static bool list_equals(const lf_page_list_t *l, const uint64_t *want, size_t n) {
  if (l->n != n) return false;
  for (size_t i = 0; i < n; i++) if (l->pages[i] != want[i]) return false;
  return true;
}

static void print_list(const char *name, const lf_page_list_t *l) {
  printf("  %s (%zu):", name, l->n);
  for (size_t i = 0; i < l->n && i < 16; i++) printf(" %lu", l->pages[i]);
  printf("%s\n", l->n > 16 ? " ..." : "");
}

static bool list_contains(const lf_page_list_t *l, uint64_t p) {
  for (size_t i = 0; i < l->n; i++) if (l->pages[i] == p) return true;
  return false;
}

#define GIB (1ul << 30)

static void test_api(void) {
  printf("case 1: userfaultfd(UFFD_USER_MODE_ONLY) with UFFD_FEATURE_WP_ASYNC\n");
  int fd = syscall(SYS_userfaultfd, O_CLOEXEC | O_NONBLOCK | UFFD_USER_MODE_ONLY);
  CHECK(fd >= 0, "userfaultfd: %m");
  if (fd < 0) return;
  struct uffdio_api api = { .api = UFFD_API, .features = UFFD_FEATURE_WP_ASYNC };
  CHECK(ioctl(fd, UFFDIO_API, &api) == 0, "UFFDIO_API: %m");
  close(fd);
}

static void test_basic_and_rearm(void) {
  printf("case 2: basic collect on a 64 GiB mapping\n");
  size_t len = 64 * GIB;
  uint8_t *m = map_region(len);
  touch(m, 0);
  touch(m, 1);
  touch(m, 600);
  touch(m, 1100);
  lf_memtrack_t t;
  int rc = lf_memtrack_open(&t, m, len);
  CHECK(rc == 0, "open: %s", strerror(-rc));
  if (rc) return;
  touch(m, 1);
  touch(m, 7);
  touch(m, 4096);
  peek(m, 0);
  lf_page_list_t l = {0};
  CHECK(lf_memtrack_collect(&t, &l, true) == 0, "collect");
  // The kernel's fast scan path reports every entry of a page table without
  // the protection bit, so the first scan after a new page table was
  // allocated (here for page 4096) reports all 512 entries of that table.
  bool ok = l.n == 2 + 512 && l.pages[0] == 1 && l.pages[1] == 7;
  for (size_t i = 2; i < l.n && ok; i++) ok = l.pages[i] == 4096 + i - 2;
  CHECK(ok, "expected {1, 7} and the page table of page 4096");
  print_list("written", &l);
  touch(m, 4100);
  CHECK(lf_memtrack_collect(&t, &l, true) == 0, "collect");
  uint64_t want4100[] = {4100};
  CHECK(list_equals(&l, want4100, 1), "expected {4100} once the page table is armed");

  printf("case 3: re-arm and report-only collect\n");
  touch(m, 7);
  CHECK(lf_memtrack_collect(&t, &l, false) == 0, "collect");
  uint64_t want7[] = {7};
  CHECK(list_equals(&l, want7, 1), "expected {7}");
  CHECK(lf_memtrack_collect(&t, &l, false) == 0, "collect");
  CHECK(list_equals(&l, want7, 1), "report-only collect must not re-arm");
  CHECK(lf_memtrack_collect(&t, &l, true) == 0, "collect");
  CHECK(lf_memtrack_collect(&t, &l, false) == 0, "collect");
  CHECK(l.n == 0, "expected nothing after re-arm");

  printf("case 3b: report-only collect over several page tables\n");
  // Page tables for pages 600 and 1100 exist (armed above), so these are real
  // writes only. A complete walk that ends in holes must not be restarted.
  touch(m, 600);
  touch(m, 1100);
  touch(m, 5000);
  CHECK(lf_memtrack_collect(&t, &l, false) == 0, "collect");
  print_list("written", &l);
  bool sorted_unique = true;
  for (size_t i = 1; i < l.n; i++) sorted_unique &= l.pages[i] > l.pages[i - 1];
  CHECK(sorted_unique, "report-only collect returned duplicate or unsorted pages");
  CHECK(lf_memtrack_collect(&t, &l, true) == 0, "collect");

  printf("case 3c: report-only collect of more regions than one kernel buffer holds\n");
  for (uint64_t i = 0; i < 4096; i++) touch(m, 8192 + i);
  CHECK(lf_memtrack_collect(&t, &l, true) == 0, "collect");
  for (uint64_t i = 0; i < 4096; i += 2) touch(m, 8192 + i);
  CHECK(lf_memtrack_collect(&t, &l, false) == 0, "collect");
  sorted_unique = true;
  for (size_t i = 1; i < l.n; i++) sorted_unique &= l.pages[i] > l.pages[i - 1];
  CHECK(sorted_unique && l.n == 2048, "expected 2048 unique pages, got %zu (sorted and unique: %d)",
        l.n, sorted_unique);
  CHECK(lf_memtrack_collect(&t, &l, true) == 0, "collect");

  printf("case 4: kernel-mode write (read() from a pipe)\n");
  int p[2];
  if (pipe(p) != 0) { perror("pipe"); exit(2); }
  char buf[64];
  memset(buf, 0x5a, sizeof(buf));
  CHECK(write(p[1], buf, sizeof(buf)) == sizeof(buf), "pipe write");
  CHECK(read(p[0], m + 12 * LF_PAGE_SIZE + 100, sizeof(buf)) == sizeof(buf), "pipe read");
  close(p[0]);
  close(p[1]);
  CHECK(lf_memtrack_collect(&t, &l, true) == 0, "collect");
  uint64_t want12[] = {12};
  CHECK(list_equals(&l, want12, 1), "expected {12}");
  print_list("written", &l);

  printf("case 5: read of a never-populated page\n");
  peek(m, 100000);
  CHECK(lf_memtrack_collect(&t, &l, true) == 0, "collect");
  printf("  unpopulated read is %sreported as written\n", list_contains(&l, 100000) ? "" : "not ");
  lf_page_list_free(&l);
  lf_memtrack_close(&t);
  munmap(m, len);
}

// Runs the same access script in parent and child after fork() and checks
// that both see the same written set.
static void access_script(uint8_t *m) {
  touch(m, 3);         // inherited page rewritten
  touch(m, 50);        // new page
  peek(m, 2000);       // unpopulated read
  int p[2];
  if (pipe(p) != 0) exit(2);
  char c = 1;
  if (write(p[1], &c, 1) != 1 || read(p[0], m + 60 * LF_PAGE_SIZE, 1) != 1) exit(2);
  close(p[0]);
  close(p[1]);
}

static void test_fork(void) {
  printf("case 6: fork symmetry\n");
  size_t len = 16 * GIB;
  uint8_t *m = map_region(len);
  for (int i = 0; i < 10; i++) touch(m, i);
  lf_memtrack_t t;
  int rc = lf_memtrack_open(&t, m, len);
  CHECK(rc == 0, "open: %s", strerror(-rc));
  if (rc) return;
  int fds[2];
  if (pipe(fds) != 0) exit(2);
  pid_t pid = fork();
  if (pid == 0) {
    close(fds[0]);
    lf_memtrack_forget_inherited(&t);
    if (lf_memtrack_open(&t, m, len) != 0) _exit(3);
    access_script(m);
    lf_page_list_t l = {0};
    if (lf_memtrack_collect(&t, &l, false) != 0) _exit(4);
    uint64_t n = l.n;
    if (write(fds[1], &n, sizeof(n)) != sizeof(n)) _exit(5);
    if (write(fds[1], l.pages, n * sizeof(uint64_t)) != (ssize_t)(n * sizeof(uint64_t))) _exit(5);
    _exit(0);
  }
  close(fds[1]);
  access_script(m);
  lf_page_list_t mine = {0};
  CHECK(lf_memtrack_collect(&t, &mine, false) == 0, "collect");
  uint64_t n = 0;
  CHECK(read(fds[0], &n, sizeof(n)) == sizeof(n), "child result");
  uint64_t *theirs = calloc(n + 1, sizeof(uint64_t));
  size_t got = 0;
  while (got < n * sizeof(uint64_t)) {
    ssize_t r = read(fds[0], (char *)theirs + got, n * sizeof(uint64_t) - got);
    if (r <= 0) break;
    got += r;
  }
  int st;
  waitpid(pid, &st, 0);
  CHECK(WIFEXITED(st) && WEXITSTATUS(st) == 0, "child failed with status %d", st);
  print_list("parent", &mine);
  CHECK(list_equals(&mine, theirs, n), "parent and child sets differ (child has %lu pages)", n);

  printf("case 7: isolation of parent and child writes\n");
  CHECK(lf_memtrack_collect(&t, &mine, true) == 0, "collect");
  pid = fork();
  if (pid == 0) {
    lf_memtrack_forget_inherited(&t);
    if (lf_memtrack_open(&t, m, len) != 0) _exit(3);
    touch(m, 8);  // Q
    usleep(200000);
    lf_page_list_t l = {0};
    if (lf_memtrack_collect(&t, &l, false) != 0) _exit(4);
    _exit(l.n == 1 && l.pages[0] == 8 ? 0 : 6);
  }
  touch(m, 5);  // P
  usleep(300000);
  CHECK(lf_memtrack_collect(&t, &mine, false) == 0, "collect");
  uint64_t want5[] = {5};
  CHECK(list_equals(&mine, want5, 1), "parent expected {5}");
  print_list("parent", &mine);
  waitpid(pid, &st, 0);
  CHECK(WIFEXITED(st) && WEXITSTATUS(st) == 0, "child expected {8}, status %d", st);
  lf_page_list_free(&mine);
  free(theirs);
  lf_memtrack_close(&t);
  munmap(m, len);
}

static void test_scale(void) {
  printf("case 8: scale (4 GiB populated in an 8 TiB mapping)\n");
  size_t len = 8192 * GIB;
  uint8_t *m = map_region(len);
  size_t pages = 4 * GIB / LF_PAGE_SIZE;
  double t0 = now_ms();
  for (size_t i = 0; i < pages; i++) touch(m, i);
  double t1 = now_ms();
  lf_memtrack_t t;
  int rc = lf_memtrack_open(&t, m, len);
  double t2 = now_ms();
  CHECK(rc == 0, "open: %s", strerror(-rc));
  if (rc) return;
  lf_page_list_t l = {0};
  double t3 = now_ms();
  CHECK(lf_memtrack_collect(&t, &l, true) == 0, "collect");
  double t4 = now_ms();
  CHECK(l.n == 0, "expected no written page, got %zu", l.n);
  for (size_t i = 0; i < pages; i += 100) touch(m, i);
  double t5 = now_ms();
  CHECK(lf_memtrack_collect(&t, &l, true) == 0, "collect");
  double t6 = now_ms();
  CHECK(l.n == (pages + 99) / 100, "expected %zu written pages, got %zu", (pages + 99) / 100, l.n);
  printf("  populate %.0f ms, arm %.1f ms, empty collect %.1f ms, 1%% collect %.1f ms (%zu pages)\n",
         t1 - t0, t2 - t1, t4 - t3, t6 - t5, l.n);
  CHECK(t2 - t1 < 1000, "arming 4 GiB took %.0f ms", t2 - t1);
  CHECK(t6 - t5 < 200, "1%% collect took %.0f ms", t6 - t5);

  printf("case 8b: scan continuation with a small output vector\n");
  lf_memtrack_set_vec_cap(&t, 7);
  for (size_t i = 0; i < 1000; i += 2) touch(m, i);
  CHECK(lf_memtrack_collect(&t, &l, true) == 0, "collect");
  CHECK(l.n == 500, "expected 500 pages, got %zu", l.n);
  bool ok = true;
  for (size_t i = 0; i < l.n && ok; i++) ok = l.pages[i] == 2 * i;
  CHECK(ok, "continued scan returned wrong pages");

  printf("case 8c: report-only scan continuation with a small output vector\n");
  for (size_t i = 0; i < 1000; i += 2) touch(m, i);
  CHECK(lf_memtrack_collect(&t, &l, false) == 0, "collect");
  ok = l.n == 500;
  for (size_t i = 0; i < l.n && ok; i++) ok = l.pages[i] == 2 * i;
  CHECK(ok, "continued report-only scan returned %zu pages", l.n);
  lf_page_list_free(&l);
  lf_memtrack_close(&t);
  munmap(m, len);
}

static void test_thp(void) {
  printf("case 9: transparent huge pages\n");
  size_t len = 64ul << 20;
  uint8_t *raw = map_region(len + (2ul << 20));
  uint8_t *m = (uint8_t *)(((uintptr_t)raw + (2ul << 20) - 1) & ~((2ul << 20) - 1));
  if (madvise(m, len, MADV_HUGEPAGE) != 0) perror("madvise");
  memset(m, 1, len);
  lf_memtrack_t t;
  int rc = lf_memtrack_open(&t, m, len);
  CHECK(rc == 0, "open: %s", strerror(-rc));
  if (rc) return;
  int fds[2];
  if (pipe(fds) != 0) exit(2);
  pid_t pid = fork();
  if (pid == 0) {
    lf_memtrack_forget_inherited(&t);
    if (lf_memtrack_open(&t, m, len) != 0) _exit(3);
    touch(m, 1000);
    lf_page_list_t l = {0};
    if (lf_memtrack_collect(&t, &l, false) != 0) _exit(4);
    uint64_t n = l.n;
    if (write(fds[1], &n, sizeof(n)) != sizeof(n)) _exit(5);
    _exit(0);
  }
  touch(m, 1000);
  lf_page_list_t l = {0};
  CHECK(lf_memtrack_collect(&t, &l, false) == 0, "collect");
  uint64_t n = 0;
  CHECK(read(fds[0], &n, sizeof(n)) == sizeof(n), "child result");
  int st;
  waitpid(pid, &st, 0);
  printf("  one byte written in a huge page: parent reports %zu pages, child %lu\n", l.n, n);
  CHECK(l.n == n, "asymmetric THP report");
  lf_page_list_free(&l);
  lf_memtrack_close(&t);
  munmap(raw, len + (2ul << 20));
}

static void test_hash(void) {
  printf("case 10: page hash\n");
  static uint64_t a[512], b[512];
  for (int i = 0; i < 512; i++) a[i] = b[i] = i * 0x9e3779b97f4a7c15ull;
  CHECK(lf_hash_page(a) == lf_hash_page(b), "equal pages hash differently");
  ((uint8_t *)b)[4095] ^= 1;
  CHECK(lf_hash_page(a) != lf_hash_page(b), "one-bit change not detected");
  static uint64_t z[512];
  printf("  hash(zero page) = %016lx\n", lf_hash_page(z));
  double t0 = now_ms();
  uint64_t acc = 0;
  for (int i = 0; i < 100000; i++) { a[i & 511] = i; acc += lf_hash_page(a); }
  double t1 = now_ms();
  printf("  %.1f GB/s (%lx)\n", 100000.0 * 4096 / (t1 - t0) / 1e6, acc & 0xf);
}

int main(void) {
  setvbuf(stdout, NULL, _IONBF, 0);
  test_api();
  test_basic_and_rearm();
  test_fork();
  test_scale();
  test_thp();
  test_hash();
  printf("%s (%d failures)\n", failures ? "FAIL" : "PASS", failures);
  return failures != 0;
}
