/***************************************************************************************
* Copyright (c) 2014-2021 Zihao Yu, Nanjing University
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

#include <sys/mman.h>
#include <cpu/decode.h>
#include <cpu/tcache.h>
#include <memory/vaddr.h>
#include <memory/host-tlb.h>
#include <cpu/cpu.h>

#ifdef CONFIG_PERF_OPT

#define TCACHE_BB_SIZE_OF(tcache_size) ((tcache_size) / 4 + 2)
#define TCACHE_BB_SIZE TCACHE_BB_SIZE_OF(CONFIG_TCACHE_SIZE)

typedef struct bb_t {
  Decode *s;
  struct bb_t *next;
  vaddr_t pc;
  IFDEF(CONFIG_TCACHE_KEEP_ON_TRAP, uint64_t ctx);
  uint32_t gen;   // list heads are valid only in the current flush generation
} bb_t;

#ifdef CONFIG_TCACHE_KEEP_ON_TRAP
// Exact mode runs the reference (upstream) block cache: blocks are matched
// by pc only, discarded wherever the reference discards them, and limited to
// the reference capacities. Otherwise blocks are tagged with the context
// they were decoded under and kept across context switches.
static bool tcache_exact = false;
static bool tcache_invalid = false;   // decoded code or mappings may have changed
static int tc_cap = CONFIG_TCACHE_SIZE;
static int bb_pool_cap = CONFIG_BB_POOL_SIZE;
static int tcache_bb_cap = TCACHE_BB_SIZE;
#define bb_list_size (tcache_exact ? CONFIG_TCACHE_EXACT_BB_LIST_SIZE : CONFIG_BB_LIST_SIZE)
// The context of the current lookup, computed when a block's pc matches.
typedef struct { bool known; uint64_t val; } bb_ctx_t;
static inline bool bb_match(const bb_t *bb, vaddr_t pc, bb_ctx_t *ctx) {
  if (bb->pc != pc) return false;
  if (tcache_exact) return true;
  if (!ctx->known) *ctx = (bb_ctx_t){ .known = true, .val = isa_tcache_context() };
  return bb->ctx == ctx->val;
}
// Kept blocks, and placeholders abandoned at traps, are not bounded by the
// block capacities as in the reference, so running short of placeholders is
// one more capacity overflow of this cache. The reserve covers the
// placeholders allocated between two checks.
#define TCACHE_BB_RESERVE 8
#define tcache_bb_short() (tcache_bb_live > tcache_bb_cap - TCACHE_BB_RESERVE)
#else
#define tc_cap CONFIG_TCACHE_SIZE
#define bb_pool_cap CONFIG_BB_POOL_SIZE
#define bb_list_size CONFIG_BB_LIST_SIZE
#define tcache_bb_cap TCACHE_BB_SIZE
#define bb_match(bb, _pc, ctx) ((bb)->pc == (_pc))
#endif

enum { BB_RECORD_TYPE_NTAKEN = 1, BB_RECORD_TYPE_TAKEN };

// The tables are mapped at initialisation, so that large sizes cost only the
// pages in use (and are not limited by the static data model).
static Decode *tcache_pool;
static int tc_idx = 0;
static Decode *tcache_bb_pool;
static Decode *tcache_bb_freelist = NULL;
static int tcache_bb_bump = 0;   // placeholders never allocated since the last flush start here
IFDEF(CONFIG_TCACHE_KEEP_ON_TRAP, static int tcache_bb_live = 0);   // placeholders allocated and not freed
static uint32_t bb_gen = 1;
static bb_t *bb_pool;
static int bb_idx = 0;
static bb_t *bb_list;
static const void *g_exec_nemu_decode;

static inline Decode* tcache_entry_init(Decode *s, vaddr_t pc) {
  s->tnext = s->ntnext = NULL;
  s->type = 0;
  s->pc = pc;
  s->EHelper = g_exec_nemu_decode;
  return s;
}

static inline Decode* tcache_new(vaddr_t pc) {
  if (tc_idx == tc_cap) return NULL;
  assert(tc_idx < tc_cap);
  Decode *s = &tcache_pool[tc_idx];
  tc_idx ++;
  return tcache_entry_init(s, pc);
}

#ifdef CONFIG_RT_CHECK
#define tcache_bb_check(s) do { \
  int idx = s - tcache_bb_pool; \
  Assert(idx >= 0 && idx < TCACHE_BB_SIZE, "idx = %d, s = %p", idx, s); \
} while (0)
#else
#define tcache_bb_check(s)
#endif

// Freed placeholders are reused first, most recently freed first; then
// placeholders are taken in pool order.
static inline Decode* tcache_bb_new(vaddr_t pc) {
  Decode *s = tcache_bb_freelist;
  if (s != NULL) {
    tcache_bb_check(s);
    tcache_bb_freelist = s->tnext;
  } else {
    assert(tcache_bb_bump < tcache_bb_cap);
    s = &tcache_bb_pool[tcache_bb_bump++];
  }
  IFDEF(CONFIG_TCACHE_KEEP_ON_TRAP, tcache_bb_live++);
  return tcache_entry_init(s, pc);
}

static inline void tcache_bb_free(Decode *s) {
  tcache_bb_check(s);
  s->tnext = tcache_bb_freelist;
  tcache_bb_freelist = s;
  IFDEF(CONFIG_TCACHE_KEEP_ON_TRAP, tcache_bb_live--);
}


static inline bb_t* bb_new(Decode *s, vaddr_t pc, bb_t *next) {
  if (bb_idx == bb_pool_cap) return NULL;
  assert(bb_idx < bb_pool_cap);
  bb_t *bb = &bb_pool[bb_idx ++];
  bb->s = s;
  bb->pc = pc;
  bb->next = next;
  return bb;
}

static inline void bb_move(bb_t *dst, const bb_t *src) {
  dst->s = src->s;
  dst->pc = src->pc;
  IFDEF(CONFIG_TCACHE_KEEP_ON_TRAP, dst->ctx = src->ctx);
}

static inline bb_t* bb_hash(vaddr_t pc) {
  int idx = (pc / CONFIG_ILEN_MIN) % bb_list_size;
  return &bb_list[idx];
}

static struct bb_t* bb_insert(vaddr_t pc, Decode *fill) {
  bb_t *head = bb_hash(pc);
  IFDEF(CONFIG_TCACHE_KEEP_ON_TRAP, uint64_t ctx = tcache_exact ? 0 : isa_tcache_context());
  if (head->gen != bb_gen) {
    // first time
    head->s = fill;
    head->pc = pc;
    head->next = (void *)-1ul;
    head->gen = bb_gen;
    IFDEF(CONFIG_TCACHE_KEEP_ON_TRAP, head->ctx = ctx);
    return head;
  }
  // second time
  bb_t *bb = bb_new(head->s, head->pc, head->next);
  if (bb == NULL) return NULL;
  IFDEF(CONFIG_TCACHE_KEEP_ON_TRAP, bb->ctx = head->ctx);
  head->s = fill;
  head->pc = pc;
  IFDEF(CONFIG_TCACHE_KEEP_ON_TRAP, head->ctx = ctx);
  head->next = bb;
  return head;
}

static bb_t* bb_find(vaddr_t pc) {
  bb_t *bb = bb_hash(pc);
  if (bb->gen != bb_gen) return NULL;
  IFDEF(CONFIG_TCACHE_KEEP_ON_TRAP, bb_ctx_t ctx = { .known = false });
  if (likely(bb_match(bb, pc, &ctx))) return bb;
  bb_t *head = bb;
  do {
    bb = bb->next;
    if (bb == (void *)-1ul) return NULL;
    if (bb_match(bb, pc, &ctx)) {
      bb_t tmp;
      bb_move(&tmp, bb);
      bb_move(bb, head);
      bb_move(head, &tmp);
      return head;
    }
  } while (1);
}

#ifdef CONFIG_TCACHE_KEEP_ON_TRAP
// Shadow of the reference block cache's capacity accounting.
//
// The reference discards all blocks at every context switch and runs out of
// capacity after a few thousand decoded instructions. Running out ends the
// batch (NEMU_EXEC_AGAIN), which moves every later interrupt check. Blocks
// kept across context switches do not run out that way, so the reference's
// capacity events are reproduced from this shadow. An epoch lasts from one
// reference discard to the next; in it, the reference decodes a block at the
// first entry of each block pc, which the shadow charges.
#define UTS_TC      CONFIG_TCACHE_EXACT_SIZE
#define UTS_LIST    CONFIG_TCACHE_EXACT_BB_LIST_SIZE
#define UTS_POOL    CONFIG_TCACHE_EXACT_BB_POOL_SIZE
uint32_t tcache_uts_epoch = 1;
static void block_record(Decode *head, Decode *last);
static bool tcache_block_valid(Decode *head);
static Decode *tcache_block_invalid(Decode *head);
static int tcache_uts_tc, tcache_uts_pool;          // decoded instructions, chained entries
static uint32_t tcache_uts_head_epoch[UTS_LIST];    // epoch in which a list head is taken

// The block start pcs at which the reference decoded a block in the current
// epoch (it matches blocks by pc only). Its block list holds at most
// UTS_LIST + UTS_POOL blocks, so a small table suffices; records of older
// epochs count as free.
typedef struct { vaddr_t pc; uint32_t epoch; } UtsPc;
#define UTS_PCS_BITS 13
static UtsPc uts_pcs[1 << UTS_PCS_BITS];
_Static_assert((1 << UTS_PCS_BITS) >= 2 * (UTS_LIST + UTS_POOL), "the shadow pc table is too small");

static bool uts_charging;      // the block being decoded is decoded by the reference too
static vaddr_t uts_block_pc;   // its start pc
static Decode *uts_stop_at;    // the reference runs out of capacity before this instruction
static const void *uts_stop_saved;
static const void *g_exec_uts_stop, *g_exec_uts_resume;

// After a stop the reference decodes a new block at the stop instruction. The
// kept block continues there instead, its remaining instructions renumbered
// as that new block until it is left.
bool tcache_uts_rem_active;
Decode *tcache_uts_rem_first, *tcache_uts_rem_last;
int tcache_uts_rem_bias;
#define uts_rem_first tcache_uts_rem_first
#define uts_rem_last tcache_uts_rem_last
#define uts_rem_bias tcache_uts_rem_bias
// A block restarted after this cache ran out of capacity in its middle is
// still the reference's block: its instructions are numbered bias higher
// (the bias is negative) until execution leaves it, or enters it again at
// its first instruction, where the reference would decode a block of its own.
static bool uts_rem_restart;
static Decode uts_resume;          // resumes after the new batch starts
static Decode *uts_resume_target;
static bool uts_resume_charged;    // the resumed block was charged already

// A block whose decoding the reference interrupted by running out of
// capacity: the reference decodes a new block at the stop instruction, while
// this cache keeps the instructions decoded before it and goes on decoding
// the same block, the rest of it counted as a remainder from the stop (see
// tcache_uts_stop()), which ends at the block's terminator once decoded.
static Decode *uts_split_first;
static bool uts_split_resume;      // the new batch starts by charging the split instruction

_Static_assert(UTS_TC <= CONFIG_TCACHE_SIZE && UTS_LIST <= CONFIG_BB_LIST_SIZE &&
               UTS_POOL <= CONFIG_BB_POOL_SIZE, "exact-mode capacities exceed the tables");

static void uts_cancel_stop(void) {
  if (uts_stop_at != NULL) {
    uts_stop_at->EHelper = uts_stop_saved;
    uts_stop_at = NULL;
  }
}

static void uts_rem_restore(void) { tcache_uts_rem_active = uts_rem_restart = false; }

// Instructions from first to last are numbered bias lower.
static void uts_rem_set(Decode *first, Decode *last, int bias, bool restart) {
  uts_rem_first = first;
  uts_rem_last = last;
  uts_rem_bias = bias;
  tcache_uts_rem_active = true;
  uts_rem_restart = restart;
}

// Until the terminator of the block being decoded is decoded.
#define UTS_REM_OPEN (tcache_pool + CONFIG_TCACHE_SIZE - 1)

static void uts_place_stop(Decode *at) {
  uts_stop_at = at;
  uts_stop_saved = at->EHelper;
  at->EHelper = g_exec_uts_stop;
}

// The entry the new batch starts with: it continues at s, after the executed
// instructions of the block are settled (see the longjmp handler).
static Decode *uts_arm_resume(Decode *s, int executed, bool charged) {
  uts_resume_target = s;
  uts_resume_charged = charged;
  uts_resume.pc = s->pc;
  uts_resume.idx_in_bb = executed + 1;
  uts_resume.EHelper = g_exec_uts_resume;
  return &uts_resume;
}

// Execution continues at next: the renumbered remainder is over once next is
// outside it.
void tcache_uts_leave(Decode *next) {
  if (next < uts_rem_first || next > uts_rem_last || (uts_rem_restart && next == uts_rem_first))
    uts_rem_restore();
}

// Epoch numbers are reused after a wrap, so every stamp is cleared.
static void uts_epoch_wrap(void) {
  memset(tcache_uts_head_epoch, 0, sizeof(tcache_uts_head_epoch));
  memset(uts_pcs, 0, sizeof(uts_pcs));
  for (int i = 0; i < tc_idx; i++) tcache_pool[i].uts_epoch = 0;
  tcache_uts_epoch = 1;
}

static void uts_flush(void) {
  uts_cancel_stop();
  uts_rem_restore();
  tcache_uts_tc = tcache_uts_pool = 0;
  if (++tcache_uts_epoch == 0) uts_epoch_wrap();
}

// The record of pc in this epoch, or the free record it would take.
static UtsPc *uts_pc(vaddr_t pc) {
  uint32_t mask = (1u << UTS_PCS_BITS) - 1;
  uint32_t h = (uint32_t)(((uint64_t)(pc >> 1) * 0x9e3779b97f4a7c15ull) >> (64 - UTS_PCS_BITS));
  for (;; h = (h + 1) & mask) {
    UtsPc *r = &uts_pcs[h];
    if (r->epoch != tcache_uts_epoch || r->pc == pc) return r;
  }
}

// Whether the reference decoded a block at pc in this epoch.
static inline bool uts_decoded(vaddr_t pc) { return uts_pc(pc)->epoch == tcache_uts_epoch; }

// One decoded instruction (tcache_new() in the reference).
static bool uts_alloc(void) {
  if (tcache_uts_tc == UTS_TC) return false;
  tcache_uts_tc++;
  return true;
}

// Block list insertion at the terminator (bb_insert() in the reference).
static bool uts_insert(vaddr_t pc) {
  uint32_t *head = &tcache_uts_head_epoch[(pc / CONFIG_ILEN_MIN) % UTS_LIST];
  if (*head != tcache_uts_epoch) *head = tcache_uts_epoch;
  else if (tcache_uts_pool == UTS_POOL) return false;
  else tcache_uts_pool++;
  *uts_pc(pc) = (UtsPc){ .pc = pc, .epoch = tcache_uts_epoch };
  return true;
}

// Charges a whole block of len instructions. Returns the index of the
// instruction before which the reference runs out of capacity, or -1.
static int uts_charge(vaddr_t pc, int len) {
  int avail = UTS_TC - tcache_uts_tc;
  // The reference allocates the entry of the next instruction while decoding
  // the current one, and stops before executing the current one.
  if (len > avail) return avail > 0 ? avail - 1 : 0;
  tcache_uts_tc += len;
  return uts_insert(pc) ? -1 : len - 1;
}

// Whether s is the head of a decoded block of this cache.
static inline bool uts_is_head(const Decode *s) {
  return s >= tcache_pool && s < tcache_pool + CONFIG_TCACHE_SIZE && s->idx_in_bb == 1 &&
         s->EHelper != g_exec_nemu_decode;
}

// s is about to execute. When it is the head of a decoded block entered for
// the first time in the epoch, the block is charged; if the reference runs
// out of capacity inside it, a stop is placed before that instruction.
static void tcache_uts_enter(Decode *s) {
  if (tcache_exact) { s->uts_epoch = tcache_uts_epoch; return; }
  if (uts_stop_at != NULL || !uts_is_head(s)) return;
  if (!uts_decoded(s->pc)) {
    int k = uts_charge(s->pc, s->bb_len);
    if (k >= 0) { uts_place_stop(s + k); return; }
  }
  s->uts_epoch = tcache_uts_epoch;
}

// The reference runs out of capacity before executing s: it flushes its
// blocks and continues at s in a new batch, decoding a block that starts at
// s. Returns the entry to resume with; its idx_in_bb settles the
// instructions of the block executed before s (see the longjmp handler).
Decode *tcache_uts_stop(Decode *s) {
  // Executed in this batch: the instructions before s, counted in the
  // numbering they run with (a renumbered remainder when the block is longer
  // than the reference's capacity and stopped before).
  int executed = tcache_idx(s) - 1;
  uts_flush();
  int k = s->idx_in_bb - 1;
  if (k > 0) uts_rem_set(s, s - k + (s - k)->bb_len - 1, k, false);
  return uts_arm_resume(s, executed, false);
}

// The new batch starts at the stop instruction: the reference decodes the
// block from there, which is charged now.
Decode *tcache_uts_resume(void) {
  Decode *s = uts_resume_target;
  if (uts_resume_charged) {
    uts_resume_charged = false;
    return s;
  }
  int len = tcache_uts_rem_active ? (int)(uts_rem_last - s + 1) : s->bb_len;
  if (!uts_decoded(s->pc)) {
    // A block longer than the reference's capacity stops again inside.
    int k = uts_charge(s->pc, len);
    if (k >= 0) { uts_place_stop(s + k); return s; }
  }
  if (!tcache_uts_rem_active) s->uts_epoch = tcache_uts_epoch;
  return s;
}
#endif

static void tcache_bb_fetch(Decode *_this, int is_taken, vaddr_t jpc) {
  bb_t* bb = bb_find(jpc);
  if (bb != NULL) {
    if (is_taken) { _this->tnext = bb->s; }
    else { _this->ntnext = bb->s; }
  } else {
    Decode *ret = tcache_bb_new(jpc);
    if (is_taken) { ret->type = BB_RECORD_TYPE_TAKEN; _this->tnext = ret; }
    else { ret->type = BB_RECORD_TYPE_NTAKEN; _this->ntnext = ret; }
    ret->bb_src = _this;
  }
}

// Discards the blocks of this cache.
static void tcache_flush_blocks(void) {
#ifdef CONFIG_TCACHE_KEEP_ON_TRAP
  uts_split_first = NULL;
  uts_cancel_stop();   // the entries of a stop and a remainder are gone
  uts_rem_restore();
  uts_split_resume = uts_resume_charged = false;
#endif
  tc_idx = 0;
  bb_idx = 0;
  // Invalidating the list heads by generation keeps a flush independent of
  // the table sizes.
  if (++bb_gen == 0) {
    memset(bb_list, 0, sizeof(*bb_list) * CONFIG_BB_LIST_SIZE);
    bb_gen = 1;
  }
  tcache_bb_freelist = NULL;
  tcache_bb_bump = 0;
  IFDEF(CONFIG_TCACHE_KEEP_ON_TRAP, tcache_bb_live = 0);
}

#ifdef CONFIG_TCACHE_KEEP_ON_TRAP
// This cache ran out of capacity: the blocks are discarded, and a block is
// decoded again at pc from a placeholder that has no source to patch.
static Decode *tcache_discard_for(vaddr_t pc) {
  tcache_flush_blocks();
  return tcache_bb_new(pc);
}
#endif

// A flush of the reference block cache.
void tcache_flush() {
  tcache_flush_blocks();
  IFDEF(CONFIG_TCACHE_KEEP_ON_TRAP, uts_flush());
}

enum { TCACHE_BB_BUILDING, TCACHE_RUNNING };
static int tcache_state = TCACHE_RUNNING;
static Decode *bb_now = NULL, *bb_now_record = NULL;

__attribute__((noinline))
Decode* tcache_jr_fetch(Decode *s, vaddr_t jpc) {
  s->ntnext = s->tnext;
  tcache_bb_fetch(s, true, jpc);
  return s->tnext;
}

static inline void tcache_patch_and_free(Decode *bb_record, Decode *bb) {
  Decode *src = bb_record->bb_src;
  if (bb_record->type == BB_RECORD_TYPE_TAKEN)  { src->tnext = bb; }
  if (bb_record->type == BB_RECORD_TYPE_NTAKEN) { src->ntnext = bb; }
  tcache_bb_free(bb_record);
}

__attribute__((noinline))
Decode* tcache_decode(Decode *s) {
  static int idx_in_bb = 0;
  vaddr_t thispc = s->pc;

  if (tcache_state == TCACHE_RUNNING) {  // start of a basic block
    // first check whether this basic block is already decoded
    bb_t *bb = bb_find(thispc);
#ifdef CONFIG_TCACHE_KEEP_ON_TRAP
    if (bb != NULL && !tcache_exact && !tcache_block_valid(bb->s)) {
      s = tcache_block_invalid(bb->s);
      bb = NULL;
    }
#endif
    if (bb != NULL) { // already decoded
      tcache_patch_and_free(s, bb->s);
      IFDEF(CONFIG_TCACHE_KEEP_ON_TRAP, tcache_uts_enter(bb->s));
      return bb->s;
    }

    idx_in_bb = 1;
#ifdef CONFIG_TCACHE_KEEP_ON_TRAP
    if (!tcache_exact) {
      // Out of this cache's capacity at a block start: discard the blocks
      // before the block is charged, so that nothing is restarted.
      if (tc_idx == tc_cap || tcache_bb_short()) s = tcache_discard_for(thispc);
      uts_block_pc = thispc;
      uts_charging = !uts_decoded(thispc);
      if (uts_charging && !uts_alloc()) goto uts_full;
    }
#endif
    Decode *old = s;
    s = tcache_new(thispc);
    if (s == NULL) { goto full; }

    bb_now_record = old;
    bb_now = s;
    tcache_state = TCACHE_BB_BUILDING;
  }
  // Set when this call decodes an instruction again after this cache ran
  // out of capacity: its step is charged already.
  __attribute__((unused)) bool restarted = false;
#ifdef CONFIG_TCACHE_KEEP_ON_TRAP
  if (uts_split_resume) {
    uts_split_resume = false;
    uts_block_pc = thispc;
    uts_charging = true;
    Assert(uts_alloc(), "the reference cannot hold one instruction");
  }
#endif

decode: __attribute__((unused));
  save_globals(s);
  s->idx_in_bb = idx_in_bb;
  fetch_decode(s, thispc); // note that exception may happen!

  if (s->type == INSTR_TYPE_N) {
#ifdef CONFIG_TCACHE_KEEP_ON_TRAP
    if (!tcache_exact && uts_charging && !restarted && !uts_alloc()) goto uts_full;
#endif
    Decode *next = tcache_new(s->snpc);
    if (next == NULL) { goto full; }
    assert(next == s + 1);
  } else {
    // the end of the basic block
#ifdef CONFIG_TCACHE_KEEP_ON_TRAP
    if (!tcache_exact && uts_charging && !restarted && !uts_insert(uts_block_pc)) goto uts_full;
    bb_now->bb_len = s - bb_now + 1;
    // A block restarted after this cache ran out of capacity starts inside
    // the block that was charged; entered at its own pc, the reference
    // decodes and charges it as a block of its own.
    bb_now->uts_epoch = bb_now->pc == uts_block_pc ? tcache_uts_epoch : 0;
    if (!tcache_exact) block_record(bb_now, s);
    if (uts_split_first != NULL) {
      if (tcache_uts_rem_active) uts_rem_last = s;
      uts_split_first = NULL;
    }
#endif
    bb_t *ret = bb_insert(bb_now->pc, bb_now);
    if (ret == NULL) { goto full; } // basic block list is full
    tcache_patch_and_free(bb_now_record, bb_now);
    bb_now = bb_now_record = NULL;

    switch (s->type) {
      case INSTR_TYPE_J: tcache_bb_fetch(s, true, s->jnpc); break;
      case INSTR_TYPE_B:
        tcache_bb_fetch(s, true, s->jnpc);
        tcache_bb_fetch(s, false, s->snpc + MUXDEF(__ISA_mips32__, 4, 0));
        break;
      case INSTR_TYPE_I: s->tnext = s->ntnext = s; break; // update dynamically
      default: assert(0);
    }
    tcache_state = TCACHE_RUNNING;
  }

  idx_in_bb ++;
  return s;

full:
#ifdef CONFIG_TCACHE_KEEP_ON_TRAP
  if (!tcache_exact) {
    // Running out of this (larger) cache is not an event of the reference:
    // discard the blocks and continue in the same batch, decoding a block
    // that starts at this instruction. It goes on with the numbering of the
    // block the reference is in, as a remainder up to its terminator.
    int executed = s != NULL ? tcache_idx(s) - 1 : 0;
    bb_now_record = tcache_discard_for(thispc);
    s = tcache_new(thispc);
    bb_now = s;
    idx_in_bb = 1;
    if (executed > 0) {
      uts_split_first = s;
      uts_rem_set(s, UTS_REM_OPEN, -executed, true);
    }
    tcache_state = TCACHE_BB_BUILDING;
    restarted = true;
    goto decode;
  }
#endif
  tcache_flush();
  s = tcache_bb_new(thispc); // decode this instruction again
  s->idx_in_bb = idx_in_bb;
  save_globals(s);
  bb_now = bb_now_record = NULL;
  tcache_state = TCACHE_RUNNING;
  longjmp_context(NEMU_EXEC_AGAIN);

#ifdef CONFIG_TCACHE_KEEP_ON_TRAP
uts_full:
  // The reference runs out of capacity before executing s (see
  // tcache_uts_stop()) and decodes a new block at s in a new batch.
  if (tcache_state == TCACHE_BB_BUILDING && s != bb_now) {
    // Mid-block: the instructions decoded so far stay. The new batch starts
    // at s, after the instructions executed before it are settled, and
    // decodes s again as the first instruction of the reference's new block
    // (charged then, after any interrupt the new batch may take first). A
    // segment split off before has been executed and takes the block's
    // numbering again.
    int executed = tcache_idx(s) - 1;
    uts_flush();
    uts_split_first = s;
    uts_rem_set(s, UTS_REM_OPEN, s->idx_in_bb - 1, false);
    uts_split_resume = true;
    s->EHelper = g_exec_nemu_decode;
    save_globals(uts_arm_resume(s, executed, true));
    longjmp_context(NEMU_EXEC_AGAIN);
  }
  // The block being decoded is abandoned; it is the most recent allocation.
  uts_split_first = NULL;
  if (tcache_state == TCACHE_BB_BUILDING) tc_idx = bb_now - tcache_pool;
  bb_now = bb_now_record = NULL;
  tcache_state = TCACHE_RUNNING;
  uts_flush();
  s = tcache_bb_new(thispc);
  s->idx_in_bb = idx_in_bb;
  save_globals(s);
  longjmp_context(NEMU_EXEC_AGAIN);
#endif
}

static Decode ex = {};

void tcache_handle_exception(vaddr_t jpc) {
#ifdef CONFIG_TCACHE_KEEP_ON_TRAP
  uts_split_first = NULL;
  uts_split_resume = uts_resume_charged = false;
#endif
  tcache_bb_fetch(&ex, true, jpc);
  save_globals(ex.tnext);
  tcache_state = TCACHE_RUNNING;
}

// Called wherever the reference discards all blocks. Without exact mode,
// blocks stay and are looked up under the new context, unless their code or
// mappings may have changed.
Decode* tcache_handle_flush(vaddr_t snpc) {
#ifdef CONFIG_TCACHE_KEEP_ON_TRAP
  if (tcache_exact || tcache_invalid) tcache_flush_blocks();
  tcache_invalid = false;
  uts_flush();
#else
  tcache_flush();
#endif
  tcache_handle_exception(snpc);
  return ex.tnext;
}

#ifdef CONFIG_TCACHE_KEEP_ON_TRAP
// Decoded code or address mappings may have changed: the next flush of the
// reference also discards the kept blocks.
void tcache_invalidate(void) { tcache_invalid = true; }

// Kept blocks carry the physical pages of their code (code_sig) as fetched in
// the mapping generation vgen. After an SFENCE.VMA the reference discards its
// blocks and fetches their instructions again; a kept block runs again
// without that only where those fetches would find the same physical pages
// and change nothing (tcache_block_valid()); otherwise all blocks are
// discarded, as before.
static uint32_t tcache_vgen = 1;
void tcache_mappings_changed(void) { tcache_vgen++; }

// Results of fetch probes in the current mapping generation, as a TLB holds
// them: many blocks share a page.
#define PROBE_MEMO 4096
static struct { vaddr_t page; uint64_t ctx; paddr_t pa; uint32_t gen; } probe_memo[PROBE_MEMO];

static bool probe_page(vaddr_t va, paddr_t *pa) {
  vaddr_t page = va & ~(vaddr_t)PAGE_MASK;
  uint64_t ctx = isa_tcache_context();
  uint32_t k = ((page >> PAGE_SHIFT) ^ (ctx * 0x9e3779b97f4a7c15ull >> 40)) & (PROBE_MEMO - 1);
  if (probe_memo[k].gen == tcache_vgen && probe_memo[k].page == page && probe_memo[k].ctx == ctx) {
    *pa = probe_memo[k].pa;
    return true;
  }
  if (!isa_ifetch_probe(va, pa)) return false;
  probe_memo[k].page = page;
  probe_memo[k].ctx = ctx;
  probe_memo[k].pa = *pa;
  probe_memo[k].gen = tcache_vgen;
  return true;
}

// The physical pages of the code from pc up to (not including) end, folded
// into one value; false if a fetch would fault or update a page table.
// Right after the code was fetched, the host TLB holds the pages it was
// fetched from.
static bool code_signature(vaddr_t pc, vaddr_t end, bool fetched, uint64_t *sig) {
  uint64_t h = 0x9e3779b97f4a7c15ull;
  for (vaddr_t page = pc & ~(vaddr_t)PAGE_MASK; page < end; page += PAGE_SIZE) {
    paddr_t pa;
    vaddr_t va = page < pc ? pc : page;
    if (!(fetched && hosttlb_ifetch_page(va, &pa)) && !probe_page(va, &pa)) return false;
    h = (h ^ pa) * 0xff51afd7ed558ccdull;
    h ^= h >> 32;
  }
  *sig = h;
  return true;
}

static void block_record(Decode *head, Decode *last) {
  head->vgen = code_signature(head->pc, last->snpc, true, &head->code_sig) ? tcache_vgen : 0;
}

static bool tcache_block_valid(Decode *head) {
  if (head->vgen == tcache_vgen) return true;
  uint64_t sig;
  if (head->vgen == 0 || !code_signature(head->pc, (head + head->bb_len - 1)->snpc, false, &sig) ||
      sig != head->code_sig) return false;
  head->vgen = tcache_vgen;
  return true;
}

// The block at head runs no more: all blocks are discarded, and the
// returned placeholder decodes a block at its pc again.
static Decode *tcache_block_invalid(Decode *head) {
  return tcache_discard_for(head->pc);
}

// s is about to execute at a block start in a new epoch of the reference.
Decode *tcache_uts_check(Decode *s) {
  if (!tcache_exact && uts_is_head(s) && !tcache_block_valid(s)) return tcache_block_invalid(s);
  tcache_uts_enter(s);
  return s;
}

// The reference capacities and matching from the next flush on; the kept
// blocks are discarded now.
void tcache_enter_exact(void) {
  tcache_exact = true;
  tc_cap = CONFIG_TCACHE_EXACT_SIZE;
  bb_pool_cap = CONFIG_TCACHE_EXACT_BB_POOL_SIZE;
  tcache_bb_cap = TCACHE_BB_SIZE_OF(CONFIG_TCACHE_EXACT_SIZE);
  tcache_flush_blocks();
}

void tcache_set_uts_labels(const void *stop, const void *resume) {
  g_exec_uts_stop = stop;
  g_exec_uts_resume = resume;
}
#endif

// Large tables are accessed at random: huge pages keep their address
// translations in the TLB.
static void *tcache_map(size_t bytes) {
  void *p = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
  Assert(p != MAP_FAILED, "cannot map %zu bytes for the block cache", bytes);
  madvise(p, bytes, MADV_HUGEPAGE);
  return p;
}

Decode* tcache_init(const void *exec_nemu_decode, vaddr_t reset_vector) {
  if (tcache_pool == NULL) {
    tcache_pool = tcache_map(sizeof(Decode) * CONFIG_TCACHE_SIZE);
    tcache_bb_pool = tcache_map(sizeof(Decode) * TCACHE_BB_SIZE);
    bb_pool = tcache_map(sizeof(bb_t) * CONFIG_BB_POOL_SIZE);
    bb_list = tcache_map(sizeof(bb_t) * CONFIG_BB_LIST_SIZE);
  }
  tcache_flush();
  g_exec_nemu_decode = exec_nemu_decode;
  return tcache_bb_new(reset_vector);
}

#else // CONFIG_PERF_OPT
#define SIMPLE_TCACHE_SIZE 8192
#define SIMPLE_TCACHE_WAYS 4
#define SIMPLE_TCACHE_SETS (SIMPLE_TCACHE_SIZE / SIMPLE_TCACHE_WAYS)
#define SIMPLE_TCACHE_SET_MASK (SIMPLE_TCACHE_SETS - 1)

typedef struct {
  vaddr_t pc;
  Decode s;
  int valid;
} simple_tcache_entry_t;

static simple_tcache_entry_t tcache[SIMPLE_TCACHE_SETS][SIMPLE_TCACHE_WAYS] = {0};
static uint8_t simple_tcache_next_way[SIMPLE_TCACHE_SETS] = {0};

static inline int tcache_set_idx(vaddr_t pc) {
  return (pc >> 2) & SIMPLE_TCACHE_SET_MASK;
}

static inline simple_tcache_entry_t *tcache_set(vaddr_t pc) {
  return tcache[tcache_set_idx(pc)];
}

static inline simple_tcache_entry_t *tcache_find(simple_tcache_entry_t *set, vaddr_t pc) {
  for (int way = 0; way < SIMPLE_TCACHE_WAYS; way++) {
    if (set[way].valid && set[way].pc == pc) {
      return &set[way];
    }
  }
  return NULL;
}

static inline simple_tcache_entry_t *tcache_victim(simple_tcache_entry_t *set, int set_idx) {
  for (int way = 0; way < SIMPLE_TCACHE_WAYS; way++) {
    if (!set[way].valid) {
      return &set[way];
    }
  }
  int victim = simple_tcache_next_way[set_idx];
  simple_tcache_next_way[set_idx] = (victim + 1) % SIMPLE_TCACHE_WAYS;
  return &set[victim];
}

Decode* tcache_lookup_instr(vaddr_t pc) {
  simple_tcache_entry_t *set = tcache_set(pc);
  simple_tcache_entry_t *entry = tcache_find(set, pc);
  if (entry != NULL) {
    return &entry->s;
  }
  return NULL;
}

void tcache_insert_instr(vaddr_t pc, Decode *s) {
  int set_idx = tcache_set_idx(pc);
  simple_tcache_entry_t *set = tcache[set_idx];
  simple_tcache_entry_t *entry = tcache_find(set, pc);
  if (entry == NULL) {
    entry = tcache_victim(set, set_idx);
  }
  entry->pc = pc;
  entry->valid = 1;
  memcpy(&entry->s, s, sizeof(Decode));
}

void tcache_handle_flush() {
  memset(tcache, 0, sizeof(tcache));
  memset(simple_tcache_next_way, 0, sizeof(simple_tcache_next_way));
}
#endif
