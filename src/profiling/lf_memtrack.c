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

#ifndef LF_MEMTRACK_STANDALONE
#include <generated/autoconf.h>
#endif

#if defined(CONFIG_LF_PROFILING) || defined(LF_MEMTRACK_STANDALONE)

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <profiling/lf_memtrack.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <linux/userfaultfd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>

// ---------------------------------------------------------------------------
// Page hash: xxh64-style lanes and avalanche over 64-bit words.

#define H_P1 0x9E3779B185EBCA87ull
#define H_P2 0xC2B2AE3D27D4EB4Full
#define H_P3 0x165667B19E3779F9ull
#define H_P4 0x85EBCA77C2B2AE63ull

static inline uint64_t h_rotl(uint64_t x, int r) { return (x << r) | (x >> (64 - r)); }
static inline uint64_t h_round(uint64_t acc, uint64_t in) { return h_rotl(acc + in * H_P2, 31) * H_P1; }
static inline uint64_t h_merge(uint64_t acc, uint64_t v) { return (acc ^ h_round(0, v)) * H_P1 + H_P4; }

uint64_t lf_hash_page(const void *page) {
  const uint64_t *w = (const uint64_t *)page;
  const size_t n = LF_PAGE_SIZE / 8;
  uint64_t v1 = H_P1 + H_P2, v2 = H_P2, v3 = 0, v4 = -H_P1;
  for (size_t i = 0; i < n; i += 4) {
    v1 = h_round(v1, w[i]);
    v2 = h_round(v2, w[i + 1]);
    v3 = h_round(v3, w[i + 2]);
    v4 = h_round(v4, w[i + 3]);
  }
  uint64_t h = h_rotl(v1, 1) + h_rotl(v2, 7) + h_rotl(v3, 12) + h_rotl(v4, 18);
  h = h_merge(h, v1);
  h = h_merge(h, v2);
  h = h_merge(h, v3);
  h = h_merge(h, v4);
  h += LF_PAGE_SIZE;
  h ^= h >> 33;
  h *= H_P2;
  h ^= h >> 29;
  h *= H_P3;
  h ^= h >> 32;
  return h;
}

// ---------------------------------------------------------------------------
// Page lists

static int lf_page_list_push(lf_page_list_t *l, uint64_t page) {
  if (l->n == l->cap) {
    size_t cap = l->cap ? l->cap * 2 : 1024;
    uint64_t *p = realloc(l->pages, cap * sizeof(*p));
    if (p == NULL) return -ENOMEM;
    l->pages = p;
    l->cap = cap;
  }
  l->pages[l->n++] = page;
  return 0;
}

void lf_page_list_free(lf_page_list_t *l) {
  free(l->pages);
  *l = (lf_page_list_t){0};
}

// ---------------------------------------------------------------------------
// Tracker

void lf_memtrack_set_vec_cap(lf_memtrack_t *t, size_t cap) {
  struct page_region *v = realloc(t->vec, cap * sizeof(*v));
  if (v == NULL) return;
  t->vec = v;
  t->vec_cap = cap;
}

int lf_memtrack_open(lf_memtrack_t *t, void *base, size_t len) {
  *t = (lf_memtrack_t){ .base = base, .len = len, .uffd = -1, .pagemap_fd = -1 };
  int rc = 0;
  t->pagemap_fd = open("/proc/self/pagemap", O_RDONLY | O_CLOEXEC);
  if (t->pagemap_fd < 0) { rc = -errno; goto fail; }
  lf_memtrack_set_vec_cap(t, 1u << 16);
  if (t->vec == NULL) { rc = -ENOMEM; goto fail; }
  // Unprivileged processes may only create user-mode-only descriptors when
  // vm.unprivileged_userfaultfd is 0. Asynchronous write-protect faults are
  // resolved inside the kernel, including those raised by kernel-mode writes.
  t->uffd = syscall(SYS_userfaultfd, O_CLOEXEC | O_NONBLOCK | UFFD_USER_MODE_ONLY);
  if (t->uffd < 0) { rc = -errno; goto fail; }
  struct uffdio_api api = { .api = UFFD_API, .features = UFFD_FEATURE_WP_ASYNC };
  if (ioctl(t->uffd, UFFDIO_API, &api) != 0) { rc = -errno; goto fail; }
  struct uffdio_register reg = {
    .range = { .start = (uintptr_t)base, .len = len }, .mode = UFFDIO_REGISTER_MODE_WP };
  if (ioctl(t->uffd, UFFDIO_REGISTER, &reg) != 0) { rc = -errno; goto fail; }
  // Arm: protect every present page. Holes stay unprotected (the category
  // mask below never matches them), so no markers are installed there.
  rc = lf_memtrack_collect(t, NULL, true);
  if (rc == 0) return 0;
fail:
  lf_memtrack_close(t);
  return rc;
}

int lf_memtrack_collect(lf_memtrack_t *t, lf_page_list_t *out, bool rearm) {
  struct pm_scan_arg arg = {
    .size = sizeof(arg),
    .flags = PM_SCAN_CHECK_WPASYNC | (rearm ? PM_SCAN_WP_MATCHING : 0),
    .start = (uintptr_t)t->base,
    .end = (uintptr_t)t->base + t->len,
    .vec = (uintptr_t)t->vec,
    .vec_len = t->vec_cap,
    .category_mask = PAGE_IS_WRITTEN,
    .return_mask = PAGE_IS_WRITTEN,
  };
  if (out) out->n = 0;
  for (;;) {
    long n = ioctl(t->pagemap_fd, PAGEMAP_SCAN, &arg);
    if (n < 0) return -errno;
    for (long i = 0; out && i < n; i++) {
      for (uint64_t a = t->vec[i].start; a < t->vec[i].end; a += LF_PAGE_SIZE) {
        uint64_t page = (a - (uintptr_t)t->base) >> LF_PAGE_SHIFT;
        if (out->n > 0 && page <= out->pages[out->n - 1]) continue;  // keep the list sorted and unique
        int rc = lf_page_list_push(out, page);
        if (rc != 0) return rc;
      }
    }
    // The walk stops early only when the output vector is full. walk_end is
    // not reliable otherwise: after a complete walk the kernel may leave it
    // before the end of the range, and restarting there would report pages
    // twice.
    if ((size_t)n < t->vec_cap || arg.walk_end >= arg.end) break;
    arg.start = arg.walk_end;
  }
  return 0;
}

void lf_memtrack_forget_inherited(lf_memtrack_t *t) {
  // The child's copy of the mapping is no longer registered, so closing the
  // descriptors cannot affect the parent, which keeps its own references.
  if (t->uffd >= 0) close(t->uffd);
  if (t->pagemap_fd >= 0) close(t->pagemap_fd);
  free(t->vec);
  t->vec = NULL;
  t->vec_cap = 0;
  t->uffd = t->pagemap_fd = -1;
}

void lf_memtrack_close(lf_memtrack_t *t) {
  if (t->uffd >= 0) {
    struct uffdio_range range = { .start = (uintptr_t)t->base, .len = t->len };
    ioctl(t->uffd, UFFDIO_UNREGISTER, &range);
  }
  lf_memtrack_forget_inherited(t);
}

#endif // CONFIG_LF_PROFILING || LF_MEMTRACK_STANDALONE
