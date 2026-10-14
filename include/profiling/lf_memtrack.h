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

// Tracking of written pages in an anonymous mapping.
//
// Backend: userfaultfd write-protection in asynchronous mode plus the
// PAGEMAP_SCAN ioctl. The kernel resolves write-protect faults itself and
// clears the protection bit of the written page, so the emulator's store path
// is untouched and every write is seen, whatever path it takes. A scan reports
// the pages written since the previous scan and can re-protect them
// atomically.
//
// A forked child loses the userfaultfd registration and the protection bits;
// it must forget the parent's descriptors and open its own tracker.
//
// Depends only on libc and Linux headers, so it also builds standalone.

#ifndef __PROFILING_LF_MEMTRACK_H__
#define __PROFILING_LF_MEMTRACK_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LF_PAGE_SHIFT 12
#define LF_PAGE_SIZE (1ul << LF_PAGE_SHIFT)

struct page_region;

typedef struct {
  uint8_t *base;              // tracked range, page aligned
  size_t len;
  int uffd;                   // -1 when closed
  int pagemap_fd;             // /proc/self/pagemap of the owning process
  struct page_region *vec;    // scan output buffer
  size_t vec_cap;             // entries in vec
} lf_memtrack_t;

// List of page indices relative to the tracked base, sorted ascending.
typedef struct {
  uint64_t *pages;
  size_t n, cap;
} lf_page_list_t;

// Registers the range and write-protects every present page. Returns 0 or a
// negative errno value.
int lf_memtrack_open(lf_memtrack_t *t, void *base, size_t len);
// Collects the pages written since the last arming into out (may be NULL).
// With rearm set, the collected pages are write-protected again atomically.
int lf_memtrack_collect(lf_memtrack_t *t, lf_page_list_t *out, bool rearm);
// Drops descriptors inherited from the parent without touching its state.
void lf_memtrack_forget_inherited(lf_memtrack_t *t);
void lf_memtrack_close(lf_memtrack_t *t);
// Limits the scan buffer; used by tests to exercise scan continuation.
void lf_memtrack_set_vec_cap(lf_memtrack_t *t, size_t cap);

void lf_page_list_free(lf_page_list_t *l);

// 64-bit hash of one 4 KiB page (xxh64-style lanes and avalanche).
uint64_t lf_hash_page(const void *page);

#ifdef __cplusplus
}
#endif

#endif // __PROFILING_LF_MEMTRACK_H__
