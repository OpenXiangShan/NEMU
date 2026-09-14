// SPDX-License-Identifier: MulanPSL-2.0
// Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
#ifndef __AME_SVSTORE_QUEUE_WRAPPER_H__
#define __AME_SVSTORE_QUEUE_WRAPPER_H__

#include <common.h>
#include <ame/svstore.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_AME_MEM_ACCESS_CHECK
// RISC-V FENCE predecessor bits relevant to the store queue.
enum {
  SVSTORE_FENCE_W = 1 << 0,
  SVSTORE_FENCE_O = 1 << 2,
};

void svstore_queue_reset();
void svstore_queue_push(svstore_info_t svstore_info);
bool svstore_queue_empty();

// Record a completed RAM store, retaining whether its mapping uses PBMT=IO.
void svstore_queue_emplace(paddr_t addr, int len, vaddr_t pc, vaddr_t vaddr);

// See docs/ame-input-visibility.md for the architectural ordering rules.
// All updates affect tracking only; callers establish the required ordering.
// pred uses SVSTORE_FENCE_W/O: W covers all tracked RAM, O covers PBMT=IO RAM.
void svstore_queue_update_fence(int pred);
// Completed clean/flush publishes only the covered regular-memory bytes.
void svstore_queue_update_cbo(paddr_t addr, int len);

// Check the matrix footprint; report the first conflict and return whether found.
bool svstore_queue_check_matrix_addr_conflict(
    paddr_t base, paddr_t stride, int row, int column, int msew,
    bool transpose, vaddr_t pc, vaddr_t vbase);
#endif // CONFIG_AME_MEM_ACCESS_CHECK

#ifdef __cplusplus
}
#endif

#endif // __AME_SVSTORE_QUEUE_WRAPPER_H__
