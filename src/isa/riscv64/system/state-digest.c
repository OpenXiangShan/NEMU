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

// Whole-machine state digest. Two runs that reach the same architectural
// state print the same digest, independent of host paging: guest pages that
// were never touched and pages that are all zero hash identically.

#include <isa.h>
#include <cpu/cpu.h>
#include <memory/paddr.h>
#include <memory/vaddr.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include "../local-include/csr.h"

bool state_digest_enabled = false;

#define DIGEST_SEED  0x9e3779b97f4a7c15ull
#define DIGEST_MUL   0xff51afd7ed558ccdull

static inline uint64_t digest_mix(uint64_t h, uint64_t w) {
  h ^= w;
  h *= DIGEST_MUL;
  h ^= h >> 29;
  return h;
}

static uint64_t digest_words(uint64_t h, const void *buf, size_t len) {
  assert(len % sizeof(uint64_t) == 0);
  const uint64_t *p = buf;
  for (size_t i = 0; i < len / sizeof(uint64_t); i++) h = digest_mix(h, p[i]);
  return h;
}

// Pages that are resident or swapped out, according to /proc/self/pagemap.
// Swapped pages are included so the digest does not depend on host memory
// pressure.
static uint64_t digest_pmem(void) {
  uint8_t *pmem = get_pmem();
  const size_t page = PAGE_SIZE;
  size_t limit = get_effective_pmem_size(pmem, MEMORY_SIZE);

  int fd = open("/proc/self/pagemap", O_RDONLY);
  Assert(fd >= 0, "state digest: cannot open /proc/self/pagemap");

  const size_t batch = 1ul << 16;
  uint64_t *entries = malloc(batch * sizeof(uint64_t));
  assert(entries);

  uint64_t h = DIGEST_SEED;
  for (size_t base = 0; base < limit; base += batch * page) {
    size_t n = MIN_OF(batch, (limit - base) / page);
    off_t off = (off_t)(((uintptr_t)(pmem + base) / page) * sizeof(uint64_t));
    ssize_t got = pread(fd, entries, n * sizeof(uint64_t), off);
    Assert(got == (ssize_t)(n * sizeof(uint64_t)), "state digest: short pagemap read");
    for (size_t i = 0; i < n; i++) {
      bool present = (entries[i] >> 63) & 1;
      bool swapped = (entries[i] >> 62) & 1;
      if (!present && !swapped) continue;
      const uint64_t *p = (const uint64_t *)(pmem + base + i * page);
      if (pmem_page_is_zero(p)) continue;
      h = digest_mix(h, (base / page) + i);
      h = digest_words(h, p, page);
    }
  }
  free(entries);
  close(fd);
  return h;
}

void isa_state_digest_report(void) {
  uint64_t get_mtime();
  uint64_t clint_get_mtimecmp();
  uint64_t get_mtime_correction();

  uint64_t regs = DIGEST_SEED;
  regs = digest_words(regs, cpu.gpr, sizeof(cpu.gpr));
  IFNDEF(CONFIG_FPU_NONE, regs = digest_words(regs, cpu.fpr, sizeof(cpu.fpr)));
  IFDEF(CONFIG_RVV, regs = digest_words(regs, cpu.vr, sizeof(cpu.vr)));
  regs = digest_mix(regs, cpu.pc);
  regs = digest_mix(regs, cpu.mode);
  IFDEF(CONFIG_RVH, regs = digest_mix(regs, cpu.v));

  uint64_t csr = DIGEST_SEED;
  csr = digest_words(csr, csr_array, sizeof(csr_array));
  csr = digest_mix(csr, get_mtime());
  csr = digest_mix(csr, clint_get_mtimecmp());
  csr = digest_mix(csr, get_mtime_correction());

  uint64_t mem = digest_pmem();

  Log("state digest: icount=%lu regs=%016lx csr=%016lx mem=%016lx",
      get_abs_instr_count(), regs, csr, mem);
}
