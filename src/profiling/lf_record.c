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

// Capture, file I/O and comparison of leader-follower boundary records.

#include <generated/autoconf.h>

#ifdef CONFIG_LF_PROFILING

#include <profiling/lf_record.h>
#include <profiling/profiling_control.h>
#include <memory/paddr.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern uint64_t g_nr_guest_instr;
uint64_t cpu_exec_lf_n_remain_total();
uint64_t get_mtime_correction();
uint64_t clint_get_mtime();
uint64_t clint_get_mtimecmp();
void io_space_used(const uint8_t **base, size_t *len);
const char *mmio_map_name_of_space(const void *p);
void uart16550_lf_state(uint64_t out[2]);
void sdcard_lf_state(uint64_t out[5]);

static const char *dev_names[8] = {
  "uart16550 divisor", "uart16550 fcr", "sdcard blkcnt", "sdcard blk_addr",
  "sdcard addr", "sdcard write_cmd", "sdcard read_ext_csd", "unused",
};

void lf_record_capture(lf_record_t *r, uint32_t kind, uint32_t slice, bool run_ended) {
  memset(&r->h, 0, sizeof(r->h));
  r->io_hash = NULL;
  r->pages = NULL;
  lf_record_hdr_t *h = &r->h;
  memcpy(h->magic, LF_REC_MAGIC, sizeof(h->magic));
  h->version = LF_REC_VERSION;
  h->kind = kind;
  h->slice = slice;
  h->run_ended = run_ended;
  h->icount = g_nr_guest_instr;
  h->n_remain_total = cpu_exec_lf_n_remain_total();
  h->icount_base = checkpoint_icount_base;
  h->workload_loaded = workload_loaded;
  h->donot_skip_boot = donot_skip_boot;
  h->nemu_state = nemu_state.state;
  h->halt_ret = nemu_state.halt_ret;
  h->halt_pc = nemu_state.halt_pc;
  isa_lf_capture(&h->isa);
  h->mtime = clint_get_mtime();
  h->mtimecmp = clint_get_mtimecmp();
  h->mtime_correction = get_mtime_correction();
  IFDEF(CONFIG_HAS_UART16550, uart16550_lf_state(&h->dev[0]));
  IFDEF(CONFIG_HAS_SDCARD, sdcard_lf_state(&h->dev[2]));

  const uint8_t *io;
  size_t io_len;
  io_space_used(&io, &io_len);
  h->io_len = io_len;
  h->nr_io_pages = (io_len + LF_PAGE_SIZE - 1) / LF_PAGE_SIZE;
  r->io_hash = malloc(h->nr_io_pages * sizeof(uint64_t) + 1);
  Assert(r->io_hash, "out of memory");
  // new_space() hands out page-aligned sizes, so every page is complete.
  for (uint64_t i = 0; i < h->nr_io_pages; i++) r->io_hash[i] = lf_hash_page(io + i * LF_PAGE_SIZE);
}

void lf_record_set_pages(lf_record_t *r, const lf_page_list_t *pages) {
  free(r->pages);
  r->h.has_pages = 1;
  r->h.nr_pages = pages->n;
  r->pages = malloc(pages->n * sizeof(lf_page_hash_t) + 1);
  Assert(r->pages, "out of memory");
  const uint8_t *pmem = guest_to_host(CONFIG_MBASE);
  for (size_t i = 0; i < pages->n; i++) {
    uint64_t idx = pages->pages[i];
    r->pages[i].gpa = CONFIG_MBASE + (idx << LF_PAGE_SHIFT);
    r->pages[i].hash = lf_hash_page(pmem + (idx << LF_PAGE_SHIFT));
  }
}

static bool lf_write_all(FILE *fp, const void *p, size_t len) {
  return len == 0 || fwrite(p, len, 1, fp) == 1;
}

int lf_record_write(const char *path, const lf_record_t *r) {
  char tmp[4096];
  snprintf(tmp, sizeof(tmp), "%s.tmp", path);
  FILE *fp = fopen(tmp, "wb");
  if (fp == NULL) return -errno;
  bool ok = lf_write_all(fp, &r->h, sizeof(r->h)) &&
            lf_write_all(fp, r->io_hash, r->h.nr_io_pages * sizeof(uint64_t)) &&
            lf_write_all(fp, r->pages, r->h.nr_pages * sizeof(lf_page_hash_t));
  int err = ok ? 0 : errno;
  if (fclose(fp) != 0 && ok) { ok = false; err = errno; }
  if (ok && rename(tmp, path) != 0) { ok = false; err = errno; }
  if (!ok) { unlink(tmp); return -(err ? err : EIO); }
  return 0;
}

int lf_record_read(const char *path, lf_record_t *r) {
  memset(r, 0, sizeof(*r));
  FILE *fp = fopen(path, "rb");
  if (fp == NULL) return -errno;
  int rc = -EINVAL;
  if (fread(&r->h, sizeof(r->h), 1, fp) != 1) goto out;
  if (memcmp(r->h.magic, LF_REC_MAGIC, sizeof(r->h.magic)) != 0 || r->h.version != LF_REC_VERSION) goto out;
  r->io_hash = malloc(r->h.nr_io_pages * sizeof(uint64_t) + 1);
  r->pages = malloc(r->h.nr_pages * sizeof(lf_page_hash_t) + 1);
  if (r->io_hash == NULL || r->pages == NULL) { rc = -ENOMEM; goto out; }
  if (r->h.nr_io_pages && fread(r->io_hash, r->h.nr_io_pages * sizeof(uint64_t), 1, fp) != 1) goto out;
  if (r->h.nr_pages && fread(r->pages, r->h.nr_pages * sizeof(lf_page_hash_t), 1, fp) != 1) goto out;
  rc = 0;
out:
  fclose(fp);
  if (rc != 0) lf_record_free(r);
  return rc;
}

void lf_record_free(lf_record_t *r) {
  free(r->io_hash);
  free(r->pages);
  r->io_hash = NULL;
  r->pages = NULL;
}

#define LF_CMP(ok, name, a, b)                                                 \
  do {                                                                         \
    if ((a) != (b)) {                                                          \
      Log("%s is different, leader = 0x%lx, follower = 0x%lx", name,           \
          (uint64_t)(a), (uint64_t)(b));                                       \
      ok = false;                                                              \
    }                                                                          \
  } while (0)

#define LF_MAX_REPORTED_PAGES 16

static bool lf_compare_pages(const lf_record_t *l, const lf_record_t *m) {
  size_t i = 0, j = 0;
  uint64_t only_l = 0, only_m = 0, differ = 0, stray = 0;
  while (i < l->h.nr_pages || j < m->h.nr_pages) {
    const lf_page_hash_t *a = i < l->h.nr_pages ? &l->pages[i] : NULL;
    const lf_page_hash_t *b = j < m->h.nr_pages ? &m->pages[j] : NULL;
    if (b == NULL || (a != NULL && a->gpa < b->gpa)) {
      // This runs in the follower, whose copy of the page still holds the
      // content from the start of the slice. When it equals the leader's,
      // the states agree on the page: the leader's write left the content
      // unchanged, or the write was reported spuriously.
      if (lf_hash_page(guest_to_host(a->gpa)) == a->hash) stray++;
      else if (only_l++ < LF_MAX_REPORTED_PAGES) Log("page 0x%lx written only by the leader", a->gpa);
      i++;
    } else if (a == NULL || b->gpa < a->gpa) {
      if (only_m++ < LF_MAX_REPORTED_PAGES) Log("page 0x%lx written only by the follower", b->gpa);
      j++;
    } else {
      if (a->hash != b->hash && differ++ < LF_MAX_REPORTED_PAGES)
        Log("page 0x%lx content differs, leader hash %016lx, follower hash %016lx", a->gpa, a->hash, b->hash);
      i++;
      j++;
    }
  }
  if (stray) Log("memory digest: %lu pages written only by the leader keep their content", stray);
  if (only_l || only_m || differ)
    Log("memory digest: %lu pages written only by the leader, %lu only by the follower, "
        "%lu with different content (%lu and %lu pages written)",
        only_l, only_m, differ, l->h.nr_pages, m->h.nr_pages);
  return !(only_l || only_m || differ);
}

bool lf_record_compare(const lf_record_t *leader, const lf_record_t *mine) {
  const lf_record_hdr_t *l = &leader->h, *m = &mine->h;
  if (l->icount != m->icount) {
    // Every other field would differ as well.
    Log("icount is different, leader = %lu, follower = %lu (pc leader = " FMT_WORD ", follower = " FMT_WORD ")",
        l->icount, m->icount, l->isa.regs.pc, m->isa.regs.pc);
    return false;
  }
  bool ok = true;
  LF_CMP(ok, "n_remain_total", l->n_remain_total, m->n_remain_total);
  LF_CMP(ok, "icount_base", l->icount_base, m->icount_base);
  LF_CMP(ok, "workload_loaded", l->workload_loaded, m->workload_loaded);
  LF_CMP(ok, "donot_skip_boot", l->donot_skip_boot, m->donot_skip_boot);
  LF_CMP(ok, "run_ended", l->run_ended, m->run_ended);
  LF_CMP(ok, "nemu_state", l->nemu_state, m->nemu_state);
  LF_CMP(ok, "halt_ret", l->halt_ret, m->halt_ret);
  LF_CMP(ok, "halt_pc", l->halt_pc, m->halt_pc);
  if (!isa_lf_compare(&l->isa, &m->isa)) ok = false;
  LF_CMP(ok, "mtime", l->mtime, m->mtime);
  LF_CMP(ok, "mtimecmp", l->mtimecmp, m->mtimecmp);
  LF_CMP(ok, "mtime_correction", l->mtime_correction, m->mtime_correction);
  if (l->io_len != m->io_len || l->nr_io_pages != m->nr_io_pages) {
    Log("device register space size is different, leader = %lu, follower = %lu", l->io_len, m->io_len);
    ok = false;
  } else {
    const uint8_t *io;
    size_t io_len;
    io_space_used(&io, &io_len);
    for (uint64_t p = 0; p < l->nr_io_pages; p++) {
      if (leader->io_hash[p] != mine->io_hash[p]) {
        Log("device register page at offset 0x%lx (%s) is different",
            p * LF_PAGE_SIZE, mmio_map_name_of_space(io + p * LF_PAGE_SIZE));
        ok = false;
      }
    }
  }
  for (int d = 0; d < 8; d++) {
    if (l->dev[d] != m->dev[d]) {
      Log("device state %s is different, leader = 0x%lx, follower = 0x%lx", dev_names[d], l->dev[d], m->dev[d]);
      ok = false;
    }
  }
  if (l->has_pages != m->has_pages) {
    Log("memory digest present only on one side (leader %lu, follower %lu)", l->has_pages, m->has_pages);
    ok = false;
  } else if (l->has_pages && !lf_compare_pages(leader, mine)) {
    ok = false;
  }
  return ok;
}

#endif // CONFIG_LF_PROFILING
