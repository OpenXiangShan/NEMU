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

#ifndef __JIT_INTERNAL_H__
#define __JIT_INTERNAL_H__

#include <isa.h>
#include <cpu/jit.h>
#include <cpu/decode.h>
#include <cpu/tcache.h>
#include "sljitLir.h"

typedef struct {
  bool enabled;           // translate blocks at all
  int threshold;          // block-head entries before translation
  bool persistent;        // keep translations across tcache flushes
  bool chain;             // patch direct jumps between translations
  bool ibcache;           // inline cache for indirect jumps
  int ib_bits;            // log2 of its number of entries
  int pin;                // number of pinned guest registers
  bool mem;               // translate loads and stores
  bool helpers;           // run other straight-line instructions via interpreter helpers
  bool inline_fp;         // inline host floating-point arithmetic
  bool spec_fp;           // config: allow unchecked host floating point

  int min_prefix;         // shortest prefix worth translating
  bool tier2;             // compile hot loops as regions (second tier)
  int t2_samples;         // samples of a block before a loop through it is compiled
  int t2_residency;       // fewest instructions per entry a region must run to stay
  int t2_blocks;          // largest region, in blocks
  int t2_instrs;          // largest region, in instructions
  int t2_opt;             // LLVM optimisation level of regions
  int t2_cg;              // LLVM code generation level of regions
  bool t2_profile;        // keep a region only if the profile shows it faster than tier 1
  bool t2_groups;
  bool t2_canon_nan;      // tier 2 makes NaN results canonical (set after a rollback)         // one host-TLB check per group of accesses through a base register
  bool t2_units;          // hot-path units (not only loops)
  int t2_min_instrs;      // smallest unit without a loop, in instructions
  bool t2_utlb;           // cache the last page per base register in regions
  bool t2_async;          // compile in separate processes
  int t2_servers;         // number of compile processes
} JitOptions;

// The unchecked host floating point is only safe under an active leader that
// a follower validates and can roll back; serial and follower runs are exact.
extern bool jit_leader_mode;

typedef struct {
  uint64_t translated_blocks;
  uint64_t refused_blocks;
  uint64_t refused_by_exec_id[1024];
  uint64_t prefix_by_exec_id[1024];
  uint64_t entries;
  uint64_t exits_fallback;
  uint64_t exits_midblock;
  uint64_t exits_block_end;
  uint64_t exits_indirect;
  uint64_t chains;
  uint64_t ibcache_fills;
  uint64_t helper_instrs;
  uint64_t ib_empty, ib_conflict, ib_stale, ib_other;
  uint64_t guest_instrs;
  uint64_t translate_ns;
  uint64_t code_bytes;
  uint64_t tcache_flushes;
  uint64_t discarded;
  uint64_t t2_regions, t2_blocks, t2_instrs, t2_compile_ns;
  uint64_t t2_rejected_noloop, t2_rejected_fail, t2_removed;
  uint64_t t2_compiled, t2_reused, t2_requests, t2_leader_ns;
  uint64_t t2_kept, t2_lost, t2_psamples;
} JitStats;

extern JitOptions jit_opt;
extern JitStats jit_stats;

// How translated code left: filled by exit stubs, consumed by jit_run.
enum { JIT_REC_FALLBACK, JIT_REC_MIDBLOCK, JIT_REC_TAKEN, JIT_REC_NTAKEN, JIT_REC_INDIRECT };
struct JitTrans;
typedef struct {
  sljit_sw kind;
  Decode *last;        // terminator executed last
  Decode *next;        // resume point of a prefix exit
  sljit_uw target;     // indirect-jump target
  struct JitTrans *trans;  // translation that exited
  // Exits of second-tier regions: the entry slot of the translated successor
  // of a taken or not-taken exit, through which the gate chains (0 until
  // the successor is translated).
  bool region;
  sljit_uw *slot;
} JitExitRec;
// Exit stubs pass a pointer to a static record describing the exit; the
// indirect-jump target is passed separately.
extern const JitExitRec *jit_exit_ptr;
extern sljit_uw jit_exit_target;
#define JIT_MAX_EXITS 4
#define JIT_CHUNK_LEN 256
#define JIT_CHUNK_MIN 1024

typedef struct JitTrans {
  void *code;          // generated buffer, freed as a whole
  Decode *head;        // block head the translation is installed at
  sljit_uw entry;      // first instruction of the translation
  sljit_sw exec_offset;
  sljit_uw chain_addr[2];  // patchable jumps to the taken / not-taken successor
  sljit_uw chain_disp[2];  // or the displacement of a jump through a slot (JIT_SLOT_CHAIN)
  sljit_uw body;           // the block after its epoch check
  sljit_uw fallback;       // leaves for the interpreter at the block head
  JitExitRec exits[JIT_MAX_EXITS];
  int nexits;
  // Blocks longer than JIT_CHUNK_MIN instructions can also be entered at
  // every JIT_CHUNK_LEN instructions (chunk[j] for instruction j * JIT_CHUNK_LEN + 1),
  // and leave there when the reference runs out of capacity in the chunk
  // (exit record xexits[j]).
  int nchunks;
  sljit_uw *chunk;
  JitExitRec *xexits;
  bool chained[2];
  bool complete;       // the whole block is translated, not a prefix
  bool gate;           // the entry of a second-tier region
  bool superseded;     // replaced by a gate; never entered again
  sljit_uw code_size;
  uint32_t tsamples;   // profile samples of time spent in its code
  void *t2;            // the region of a gate
  uint32_t id;         // dense number, indexing the jit_uts_* arrays
  struct JitTrans *next_alloc;
} JitTrans;

#define JIT_MAX_TRANS (1 << 20)

// Offset of p from cpu, when it fits a 32-bit displacement.
static inline bool cpu_offset(const void *p, sljit_sw *off) {
  sljit_sw d = (sljit_sw)p - (sljit_sw)&cpu;
  if (d != (sljit_sw)(int32_t)d) return false;
  *off = d;
  return true;
}

// Shadow state of translated blocks (see tcache_uts_enter()), indexed by
// translation id. The words read on hot paths are kept dense.
extern uint32_t jit_uts_stamp[JIT_MAX_TRANS];      // epoch in which the block is accounted
extern sljit_uw jit_uts_entry[JIT_MAX_TRANS];      // chained jumps enter here
extern sljit_uw jit_uts_checked[JIT_MAX_TRANS];    // the entry with the epoch check
extern uint32_t jit_uts_accounted[JIT_MAX_TRANS];  // ids whose entry is the body
extern uint32_t jit_uts_naccounted;
sljit_uw jit_uts_miss_slow(uint32_t id);

#if defined(CONFIG_TCACHE_KEEP_ON_TRAP) && defined(SLJIT_CONFIG_X86_64) && SLJIT_CONFIG_X86_64
// Chained jumps read the successor's entry, so only the first entry of a
// block in an epoch runs the check. Unchained exits jump through a per-exit
// slot holding the exit path.
#define JIT_SLOT_CHAIN 1
extern sljit_uw jit_edge_slot[2 * JIT_MAX_TRANS];
#endif

// Inline cache for indirect jumps: guest target -> translated entry, with
// 2^ib_bits entries (--jit=ib-bits=N). Entries are valid only in the lookup
// context they were filled in; the context generation changes whenever
// privilege mode, MMU state or the address space may have changed. The index
// mixes in the context (jit_ctx_mix), so that the same targets of different
// address spaces use different entries.
typedef struct {
  sljit_uw pc;
  sljit_uw entry;
  sljit_uw gen;
  sljit_uw pad;
} JitIbEntry;
#define JIT_IBCACHE_SHIFT 5
extern JitIbEntry *jit_ibcache;
extern sljit_uw jit_ctx_gen, jit_ctx_mix;
static inline sljit_uw jit_ib_index(sljit_uw pc) {
  return ((pc >> 1) ^ (pc >> (jit_opt.ib_bits + 1)) ^ jit_ctx_mix) & ((1ul << jit_opt.ib_bits) - 1);
}

typedef sljit_sw (SLJIT_FUNC *jit_trampoline_fn)(sljit_uw code, CPU_state *cpu, sljit_sw budget);

bool jit_translate_block(Decode *head, JitTrans *tr);

// Second tier. Loops are selected by sampling: the translation executing
// when a batch runs out of budget is a sample weighted by execution time.
// Translation of head whose body calls region(bind): the region returns the
// exit record to leave through.
bool jit_translate_gate(Decode *head, JitTrans *tr, const void *region, void *bind);
// Interpreter helper that runs d inside translated code, or NULL.
void *jit_helper_fn(const Decode *d);
JitTrans *jit_trans_of(uint32_t id);
void jit_install_gate(JitTrans *old, JitTrans *gate);
void jit_uninstall_gate(JitTrans *old, JitTrans *gate);
void jit_reinstall_gate(JitTrans *old, JitTrans *gate);
void jit_t2_init(void);
void jit_t2_sample(JitTrans *tr);
void jit_t2_note_trans(JitTrans *tr);
void jit_t2_stop(void);
void jit_t2_discard(void);
void jit_t2_report(void);
jit_trampoline_fn jit_build_trampoline(void);
void jit_pin_init(int n);
void jit_free_code(void *code);
void jit_free_chunks(JitTrans *tr);
const char *jit_exec_name(int id);

// Interpreter semantics of one instruction as a callable function.
void (*jit_eh_lookup(int exec_id))(Decode *);

// Direct floating-point helper for an instruction, or NULL.
void (SLJIT_FUNC *jit_fp_lookup(int exec_id))(Decode *);

// Host TLB layout, exported by the memory subsystem.
void *jit_hosttlb_base(void);
long jit_hosttlb_size(void);
long jit_hosttlb_write_delta(void);
long jit_hosttlb_gvpn_offset(void);
long jit_hosttlb_offset_offset(void);

// MMU state that decides whether the inline memory fast path is valid.
const bool *jit_mmu_identity_fast_slot(void);
const int *jit_data_mmu_state_slot(void);

// Helpers called from translated code.
sljit_sw SLJIT_FUNC jit_helper_load(Decode *d, sljit_uw addr, sljit_s32 width_sign);
void SLJIT_FUNC jit_helper_store(Decode *d, sljit_uw addr, sljit_uw data, sljit_s32 width);
sljit_sw SLJIT_FUNC jit_helper_div(sljit_sw a, sljit_sw b);
sljit_sw SLJIT_FUNC jit_helper_divu(sljit_sw a, sljit_sw b);
sljit_sw SLJIT_FUNC jit_helper_rem(sljit_sw a, sljit_sw b);
sljit_sw SLJIT_FUNC jit_helper_remu(sljit_sw a, sljit_sw b);
sljit_sw SLJIT_FUNC jit_helper_divw(sljit_sw a, sljit_sw b);
sljit_sw SLJIT_FUNC jit_helper_divuw(sljit_sw a, sljit_sw b);
sljit_sw SLJIT_FUNC jit_helper_remw(sljit_sw a, sljit_sw b);
sljit_sw SLJIT_FUNC jit_helper_remuw(sljit_sw a, sljit_sw b);
sljit_sw SLJIT_FUNC jit_helper_mulh(sljit_sw a, sljit_sw b);
sljit_sw SLJIT_FUNC jit_helper_mulhu(sljit_sw a, sljit_sw b);
sljit_sw SLJIT_FUNC jit_helper_mulhsu(sljit_sw a, sljit_sw b);
bool SLJIT_FUNC jit_t2_load(Decode *d, sljit_uw addr, sljit_s32 width_sign, sljit_uw *out);
bool SLJIT_FUNC jit_t2_store(Decode *d, sljit_uw addr, sljit_uw data, sljit_s32 width);

#endif
