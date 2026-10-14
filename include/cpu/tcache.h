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

#ifndef __CPU_TCACHE_H__
#define __CPU_TCACHE_H__

#include <cpu/decode.h>

#ifdef CONFIG_TCACHE_KEEP_ON_TRAP
// Blocks kept across context switches, with a shadow of the reference
// block cache's capacity accounting (see tcache.c).

// The reference block cache from its next flush on (exact mode).
void tcache_enter_exact(void);
bool tcache_is_exact(void);

// An epoch lasts from one flush of the reference to the next.
extern uint32_t tcache_uts_epoch;
extern int tcache_uts_tc, tcache_uts_pool;
extern uint32_t tcache_uts_pc_epoch[];
uint32_t tcache_uts_pc_id(vaddr_t pc);
uint32_t *tcache_uts_head_epoch_of(vaddr_t pc);

// Charges the block of head s at its first entry in an epoch; returns true
// when a stop was placed in it.
bool tcache_uts_enter(Decode *s);
// The block start s of a new epoch: charged, after its code is checked; a
// placeholder to decode again is returned when the block's code moved.
Decode *tcache_uts_check(Decode *s);
bool tcache_block_valid(Decode *head);
Decode *tcache_block_invalid(Decode *head);
const uint32_t *tcache_vgen_slot(void);
bool tcache_uts_stop_pending(void);
Decode *tcache_uts_stop(Decode *s);
Decode *tcache_uts_resume(void);
void tcache_set_uts_labels(const void *stop, const void *resume);
// After a stop the reference counts the rest of the block as a new block:
// entries keep their number in the block, and the remainder's numbers are
// taken bias lower while execution stays in it.
extern bool tcache_uts_rem_active;
extern Decode *tcache_uts_rem_first, *tcache_uts_rem_last;
extern int tcache_uts_rem_bias;
void tcache_uts_leave(Decode *next);
// The number of s in its block as the reference counts it now.
static inline int tcache_idx(const Decode *s) {
  if (unlikely(tcache_uts_rem_active) && s >= tcache_uts_rem_first && s <= tcache_uts_rem_last)
    return s->idx_in_bb - tcache_uts_rem_bias;
  return s->idx_in_bb;
}
// Sets the helper of a decoded entry, keeping a stop placed before it.
void tcache_set_helper(Decode *d, const void *helper);
// The helper of a decoded entry, below a stop placed before it.
const void *tcache_get_helper(Decode *d);
// The instruction before which the reference runs out of capacity, or NULL.
extern Decode *tcache_uts_stop_at;
#else
#define tcache_idx(s) ((s)->idx_in_bb)
#endif

// Settles n instructions of the current block, which is restarted.
void cpu_settle_instrs(int n);

#endif
