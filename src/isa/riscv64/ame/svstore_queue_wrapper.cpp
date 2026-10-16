// SPDX-License-Identifier: MulanPSL-2.0
// Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
#include <ame/svstore_queue_wrapper.h>

#ifdef CONFIG_AME_MEM_ACCESS_CHECK

#include <algorithm>
#include <cstdint>
#include <deque>
#include <debug.h>

extern "C" {
#include <isa.h>
void monitor_statistic(void);
}

// Keep every completed store until an architectural ordering event covers it.
static std::deque<svstore_info_t> cpp_svstore_queue;

void svstore_queue_reset() {
  cpp_svstore_queue.clear();
}

bool svstore_queue_empty() {
  return cpp_svstore_queue.empty();
}

void svstore_queue_push(svstore_info_t svstore_info) {
  if (svstore_info.len == 0) return;
  // Valid RAM accesses cannot wrap the physical address space.
  Assert(svstore_info.len - 1 <= UINT64_MAX - svstore_info.addr,
         "wrapping scalar/vector store span");
  cpp_svstore_queue.push_back(svstore_info);
}

void svstore_queue_emplace(paddr_t addr, int len, vaddr_t pc, vaddr_t vaddr) {
  svstore_info_t svstore_info = {
    .addr = addr, .len = (uint64_t)len, .pc = pc, .vaddr = vaddr,
    .io = cpu.pbmt == 2,
  };
  svstore_queue_push(svstore_info);
}

void svstore_queue_update_fence(int pred) {
  cpp_svstore_queue.erase(
      std::remove_if(cpp_svstore_queue.begin(), cpp_svstore_queue.end(),
                     [pred](const svstore_info_t &store) {
                       // All tracked stores target RAM. Svpbmt makes PBMT=IO
                       // RAM accesses both memory and I/O for FENCE/aq/rl.
                       int domains = SVSTORE_FENCE_W;
                       if (store.io) domains |= SVSTORE_FENCE_O;
                       return (pred & domains) != 0;
                     }),
      cpp_svstore_queue.end());
}

void svstore_queue_update_cbo(paddr_t addr, int len) {
  if (len <= 0) return;
  // Retain any uncovered prefix/suffix, including a store spanning two blocks.
  // Rotate the original records once; newly appended fragments are not revisited.
  for (size_t n = cpp_svstore_queue.size(); n != 0; --n) {
    svstore_info_t store = cpp_svstore_queue.front();
    cpp_svstore_queue.pop_front();
    if (store.io || (store.addr <= addr ? addr - store.addr >= store.len :
                                        store.addr - addr >= (uint64_t)len)) {
      cpp_svstore_queue.push_back(store);
      continue;
    }
    const uint64_t begin = addr > store.addr ? addr - store.addr : 0;
    const uint64_t end = begin + std::min(store.len - begin,
        (uint64_t)len - (store.addr > addr ? store.addr - addr : 0));
    if (begin != 0) {
      svstore_info_t prefix = store;
      prefix.len = begin;
      cpp_svstore_queue.push_back(prefix);
    }
    if (end < store.len) {
      store.addr += end;
      store.vaddr += end;
      store.len -= end;
      cpp_svstore_queue.push_back(store);
    }
  }
}

// No logging or removal. Optionally return the conflicting store for diagnostics.
static bool svstore_queue_check_addr_conflict(paddr_t addr, uint64_t len,
                                             svstore_info_t *conflict) {
  if (len == 0 || svstore_queue_empty()) return false;
  const uint64_t start = addr;
  Assert(len - 1 <= UINT64_MAX - start, "wrapping matrix load span");
  for (const auto &store : cpp_svstore_queue) {
    // Compare distances rather than adding lengths, avoiding end-address wrap.
    bool overlap = start <= store.addr ? store.addr - start < len :
                                        start - store.addr < store.len;
    if (overlap) {
      if (conflict) *conflict = store;
      return true;
    }
  }
  return false;
}

bool svstore_queue_check_matrix_addr_conflict(
    paddr_t base, paddr_t stride, int row, int column, int msew,
    bool transpose, vaddr_t pc, vaddr_t vbase) {
  if (svstore_queue_empty() || row <= 0 || column <= 0) return false;
  const uint64_t width = 1ULL << msew;
  const uint64_t rows = transpose ? (uint64_t)column : (uint64_t)row;
  const uint64_t cols = transpose ? (uint64_t)row : (uint64_t)column;
  const uint64_t row_len = cols * width;
  for (uint64_t r = 0; r < rows; ++r) {
    // Match the existing matrix memory path's paddr_t arithmetic, including
    // zero/overlapping strides; do not divide by stride or include padding.
    const paddr_t row_base = base + r * stride;
    svstore_info_t store;
    if (svstore_queue_check_addr_conflict(row_base, row_len, &store)) {
      Log("UB: matrix load overlaps unpublished scalar/vector store: "
          "load pc=0x%lx va=0x%lx pa=0x%lx len=%lu, "
          "store pc=0x%lx va=0x%lx pa=0x%lx len=%lu; "
          "insert mfence before the matrix load",
          (uint64_t)pc, (uint64_t)(vbase + r * stride), (uint64_t)row_base, row_len,
          (uint64_t)store.pc, (uint64_t)store.vaddr, (uint64_t)store.addr, store.len);
      return true;
    }
  }
  return false;
}

#endif // CONFIG_AME_MEM_ACCESS_CHECK
