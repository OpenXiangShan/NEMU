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

// Second JIT tier: a hot loop of translated blocks is compiled with LLVM as
// one region. Everything the interpreter can observe is done block by block
// exactly as the tier-1 translations of the same blocks do it (budget, shadow
// epoch checks, exit records, slow paths, helpers), so the execution schedule
// does not change.
//
// Guest registers are read from host registers (LLVM locals) across the
// whole region, and every write also goes through to cpu, so exits and
// helpers find them published without keeping every written value live to
// the end of the loop (which made the register allocator spill). The slow
// paths of loads and stores never need them: a fault is reported instead of
// raised, and the region leaves before the access. Guest memory, the host
// TLB, the shadow bookkeeping, CSRs and published state are declared not to
// alias.
//
// A region is entered through a gate installed at its head (see
// jit_translate_gate()): the gate publishes the guest state to cpu, calls the
// region as an ordinary function, and leaves through the exit record the
// region returns.

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <generated/autoconf.h>

#ifdef CONFIG_JIT_TIER2

#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Error.h>
#include <llvm-c/LLJIT.h>
#include <llvm-c/Orc.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Transforms/PassBuilder.h>

#include <llvm-c/BitReader.h>
#include <llvm-c/BitWriter.h>

#include <isa-all-instr.h>
#include <memory/vaddr.h>
#include "jit-internal.h"
#include <errno.h>
#include <signal.h>
#include <linux/perf_event.h>
#include <sched.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <x86intrin.h>
#include <xmmintrin.h>

#ifdef CONFIG_RVB
long _rv_clz(long); long _rv_ctz(long); long _rv_cpop(long);
long _rv_rev8(long); long _rv_orc_b(long); long _rv_revb(long);
int32_t _rv32_clz(int32_t); int32_t _rv32_ctz(int32_t); int32_t _rv32_cpop(int32_t);
long _rv_rol(long, long); long _rv_ror(long, long);
int32_t _rv32_rol(int32_t, int32_t); int32_t _rv32_ror(int32_t, int32_t);
long _rv_bclr(long, long); long _rv_bset(long, long); long _rv_binv(long, long); long _rv_bext(long, long);
long _rv_clmul(long, long); long _rv_clmulh(long, long); long _rv_clmulr(long, long);
long _rv_pack(long, long); long _rv_packh(long, long); int32_t _rv32_pack(int32_t, int32_t);
#endif

#define T2_MAX_BLOCKS 64

typedef struct {
  Decode *head, *last;
  JitTrans *tr;          // the block's current translation (for its id and chaining)
  int nsucc;             // direct successors: taken (tnext), not taken (ntnext)
  int succ[2];           // their index in the region, or -1
  bool keep;
} T2Block;

// Compiled code, shared by every region with the same IR: the code reads
// everything specific to one region (decoded instructions, exit records,
// stamps) from the binding table the gate passes, so it outlives the
// translations it was first compiled for.
typedef struct T2Code {
  char *key;                // the module's bitcode, before naming
  size_t klen;
  uint64_t hash;
  LLVMOrcExecutorAddress fn;
  struct T2Region *current; // the region running it now
  struct T2Code *next;
} T2Code;

// Executions of a region, for the report.
typedef struct {
  vaddr_t pc;
  int nb, instrs, loaded, published;   // shape: blocks, instructions, registers at entry and exit
  uint64_t entries, executed;          // entries and guest instructions executed
  vaddr_t block_pc[T2_MAX_BLOCKS];
} T2Stats;

// A region installed at a head: its binding table and exit records, and
// the translations it replaces at the head and contains.
typedef struct T2Region {
  void **bind;
  JitExitRec *recs;
  T2Stats *stats;
  Decode *head;
  uint32_t gen;            // lifetime of the translations it is built from
  JitTrans *old, *gate;
  uint32_t members[T2_MAX_BLOCKS];
  int nmembers;
  bool removed, decided;
  struct T2Code *code;
  JitTrans *mtr[T2_MAX_BLOCKS];   // the blocks' translations when it was formed
  uint64_t pre_time, pre_instrs;  // profile of its blocks in tier 1 until it was installed
  uint64_t time;                  // profile samples in its gate and code
  struct T2Region *next;
} T2Region;

static T2Stats **t2_all_stats;
static int t2_nstats, t2_cap_stats;

typedef struct {
  bool dry;              // only check that every instruction is supported
  LLVMContextRef C;
  LLVMModuleRef M;
  LLVMBuilderRef B;
  LLVMValueRef F;
  LLVMTypeRef i1, i8, i16, i32, i64, f32, f64, ptr;
  unsigned md_tbaa, md_prof;
  // Memory classes that never alias: guest memory, the host TLB, the shadow
  // of the reference block cache, CSRs, and published interpreter state.
  LLVMValueRef tb_guest, tb_tlb, tb_shadow, tb_csr, tb_state, tb_bind;
  LLVMValueRef likely, unlikely;   // branch weights: first target hot, cold
  LLVMValueRef gpr[32], fpr[32];   // locals of the registers the region uses
  bool gpr_w[32], fpr_w[32];       // written by the region
  bool write_through;              // register writes are stored to cpu at once
  bool block_wb;                   // registers are stored to cpu at every block end
  bool dirty_g[32], dirty_f[32];   // written in the current block and not stored yet
  LLVMValueRef budget, budget0;
  LLVMValueRef all_charged;        // i1: every block's stamp was the epoch at entry
  T2Stats *stats;
  LLVMValueRef mem_out;            // result slot of the load slow path
  LLVMBasicBlockRef entry;
  LLVMValueRef utlb_page[2][32], utlb_off[2][32];   // per table (load, store) and base register
  // Access groups of the current block (see t2_groups()).
  int *grp_of;                     // per instruction of the block, or -1
  int grp_cap;
  struct T2Group { Decode *first; int base, maxw, count; bool store; sljit_sw lo, hi;
                   LLVMValueRef ok, gbase; } *grp;
  int ngrp, grp_cap2;
  Decode *cur_first;               // first instruction of the block being emitted
  T2Block b[T2_MAX_BLOCKS];
  int nb;
  LLVMBasicBlockRef body[T2_MAX_BLOCKS];
  bool fs_dirty;                   // the current block marked FP state dirty
  JitTrans *cur_tr;                // translation of the block being emitted
  Decode *cur_head;                // and its head
  JitExitRec *recs;
  int nrecs;
  LLVMValueRef bind_arg;           // the binding table
  void **bind;
  int nbind, cap_bind;
  // Exits branch to one shared block that publishes the state and returns
  // the record of the exit taken.
  LLVMBasicBlockRef leave;
  LLVMValueRef leave_rec;
} T2;

static LLVMOrcLLJITRef t2_jit;
static LLVMTargetMachineRef t2_tm;
static T2Region *t2_regions;
static T2Code *t2_codes;

// Code ranges of the profile: tier-1 translations and gates, region code.
enum { T2P_TRANS, T2P_UNIT };
static void t2_profile_note(uintptr_t lo, uintptr_t hi, int kind, void *owner);
static void t2_profile_close(void);
static int t2_prof_fd = -1;
static uint32_t t2_gen = 1;                    // lifetime of the current translations
static uint32_t t2_tried[JIT_MAX_TRANS];       // lifetime in which a head was tried
static uint32_t t2_member[JIT_MAX_TRANS];      // lifetime in which a block is in a region
static struct { uint32_t gen, n; } t2_samples[JIT_MAX_TRANS];
static uint32_t t2_nsampled;
static uint32_t t2_seq;

static void t2_check(LLVMErrorRef e, const char *what) {
  if (e == NULL) return;
  char *m = LLVMGetErrorMessage(e);
  panic("JIT tier 2: %s: %s", what, m);
}

static uint64_t t2_now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000000000ull + ts.tv_nsec;
}

static bool __attribute__((unused)) t2_has_fma(void) {
  static int cached = -1;
  if (cached < 0) cached = __builtin_cpu_supports("fma");
  return cached;
}

// ---------------------------------------------------------------------------
// IR helpers

static LLVMValueRef K(T2 *g, int64_t v) { return LLVMConstInt(g->i64, (unsigned long long)v, 1); }
static LLVMValueRef K32(T2 *g, int32_t v) { return LLVMConstInt(g->i32, (unsigned long long)(int64_t)v, 1); }
static LLVMValueRef P(T2 *g, const void *p) {
  return LLVMConstIntToPtr(LLVMConstInt(g->i64, (uintptr_t)p, 0), g->ptr);
}

// A pointer specific to this region, read from its binding table.
static LLVMValueRef t2_bound(T2 *g, const void *p) {
  int k = 0;
  while (k < g->nbind && g->bind[k] != p) k++;
  if (k == g->nbind) {
    if (g->nbind == g->cap_bind) {
      g->cap_bind = g->cap_bind ? 2 * g->cap_bind : 64;
      g->bind = realloc(g->bind, g->cap_bind * sizeof(void *));
    }
    g->bind[g->nbind++] = (void *)p;
  }
  LLVMValueRef idx = LLVMConstInt(g->i64, k, 0);
  LLVMValueRef slot = LLVMBuildGEP2(g->B, g->ptr, g->bind_arg, &idx, 1, "");
  LLVMValueRef v = LLVMBuildLoad2(g->B, g->ptr, slot, "");
  LLVMSetMetadata(v, g->md_tbaa, g->tb_bind);
  LLVMSetAlignment(v, 8);
  return v;
}

static LLVMValueRef t2_ld(T2 *g, LLVMTypeRef ty, LLVMValueRef p, LLVMValueRef tb, unsigned align) {
  LLVMValueRef v = LLVMBuildLoad2(g->B, ty, p, "");
  if (tb) LLVMSetMetadata(v, g->md_tbaa, tb);
  LLVMSetAlignment(v, align);
  return v;
}

static void t2_st(T2 *g, LLVMValueRef v, LLVMValueRef p, LLVMValueRef tb, unsigned align) {
  LLVMValueRef s = LLVMBuildStore(g->B, v, p);
  if (tb) LLVMSetMetadata(s, g->md_tbaa, tb);
  LLVMSetAlignment(s, align);
}

// Calls into C go through a wrapper of the module that preserves every
// register (preserve_all): calls sit on cold paths (slow paths, helpers),
// and with the C convention every value live across one would have to be
// kept in memory even on the hot path.
static LLVMValueRef t2_call(T2 *g, LLVMTypeRef ret, const void *fn, LLVMValueRef *args, int n) {
  LLVMTypeRef at[4];
  for (int i = 0; i < n; i++) at[i] = LLVMTypeOf(args[i]);
  LLVMTypeRef ft = LLVMFunctionType(ret, at, n, 0);
  return LLVMBuildCall2(g->B, ft, P(g, fn), args, n, "");
}

// A call to a function without side effects (arithmetic helpers).
static LLVMValueRef t2_call_pure(T2 *g, LLVMTypeRef ret, const void *fn, LLVMValueRef *args, int n) {
  LLVMValueRef c = t2_call(g, ret, fn, args, n);
  static const char *attrs[] = { "nounwind", "willreturn" };
  for (int i = 0; i < 2; i++) {
    LLVMAddCallSiteAttribute(c, LLVMAttributeFunctionIndex,
        LLVMCreateEnumAttribute(g->C, LLVMGetEnumAttributeKindForName(attrs[i], strlen(attrs[i])), 0));
  }
  // memory(none)
  LLVMAddCallSiteAttribute(c, LLVMAttributeFunctionIndex,
      LLVMCreateEnumAttribute(g->C, LLVMGetEnumAttributeKindForName("memory", 6), 0));
  return c;
}

static void t2_condbr(T2 *g, LLVMValueRef c, LLVMBasicBlockRef t, LLVMBasicBlockRef f, LLVMValueRef w) {
  LLVMValueRef br = LLVMBuildCondBr(g->B, c, t, f);
  if (w) LLVMSetMetadata(br, g->md_prof, w);
}

static LLVMBasicBlockRef t2_new_bb(T2 *g) { return LLVMAppendBasicBlockInContext(g->C, g->F, ""); }

static LLVMValueRef lo32(T2 *g, LLVMValueRef v) { return LLVMBuildTrunc(g->B, v, g->i32, ""); }
static LLVMValueRef sx64(T2 *g, LLVMValueRef v) { return LLVMBuildSExt(g->B, v, g->i64, ""); }
static LLVMValueRef zx64(T2 *g, LLVMValueRef v) { return LLVMBuildZExt(g->B, v, g->i64, ""); }

// Guest registers: x0 reads as zero; writes to x0 (or to a discarded
// destination, register -1) are dropped.
static LLVMValueRef gx(T2 *g, int r) {
  if (r == 0) return K(g, 0);
  Assert(g->gpr[r], "JIT tier 2: x%d not prepared", r);
  return LLVMBuildLoad2(g->B, g->i64, g->gpr[r], "");
}

static void sx(T2 *g, int r, LLVMValueRef v) {
  if (r <= 0) return;
  Assert(g->gpr_w[r], "JIT tier 2: write of x%d not prepared", r);
  LLVMBuildStore(g->B, v, g->gpr[r]);
  if (g->write_through) t2_st(g, v, P(g, &cpu.gpr[r]), g->tb_state, 8);
  g->dirty_g[r] = true;
}

#ifndef CONFIG_FPU_NONE
static LLVMValueRef gf(T2 *g, int f) {
  Assert(g->fpr[f], "JIT tier 2: f%d not prepared", f);
  return LLVMBuildLoad2(g->B, g->i64, g->fpr[f], "");
}

static void sf(T2 *g, int f, LLVMValueRef v) {
  Assert(g->fpr_w[f], "JIT tier 2: write of f%d not prepared", f);
  LLVMBuildStore(g->B, v, g->fpr[f]);
  if (g->write_through) t2_st(g, v, P(g, &cpu.fpr[f]), g->tb_state, 8);
  g->dirty_f[f] = true;
}
#endif

static int reg_of(const rtlreg_t *p) {
  uintptr_t base = (uintptr_t)&cpu.gpr[0], a = (uintptr_t)p;
  if (a < base || a >= base + sizeof(cpu.gpr) || (a - base) % sizeof(cpu.gpr[0]) != 0) return -1;
  return (a - base) / sizeof(cpu.gpr[0]);
}

static int freg_of(const rtlreg_t *p) {
#ifndef CONFIG_FPU_NONE
  uintptr_t base = (uintptr_t)&cpu.fpr[0], a = (uintptr_t)p;
  if (a < base || a >= base + sizeof(cpu.fpr) || (a - base) % sizeof(cpu.fpr[0]) != 0) return -1;
  return (a - base) / sizeof(cpu.fpr[0]);
#else
  return -1;
#endif
}

// The budget and the current instruction, as tier 1 publishes them before a
// helper.
static void t2_sync(T2 *g, Decode *d) {
  t2_st(g, LLVMBuildLoad2(g->B, g->i32, g->budget, ""), P(g, jit_budget_slot()), g->tb_state, 4);
  t2_st(g, t2_bound(g, d), P(g, jit_prev_s_slot()), g->tb_state, 8);
}

// Stores the registers written in the current block so far; clear only where
// the stores are on the block's own path (not in a side branch).
static void t2_store_dirty(T2 *g, bool clear) {
  if (g->write_through) return;
  for (int r = 1; r < 32; r++) {
    if (g->dirty_g[r]) t2_st(g, LLVMBuildLoad2(g->B, g->i64, g->gpr[r], ""), P(g, &cpu.gpr[r]), g->tb_state, 8);
    if (clear) g->dirty_g[r] = false;
  }
#ifndef CONFIG_FPU_NONE
  for (int f = 0; f < 32; f++) {
    if (g->dirty_f[f]) t2_st(g, LLVMBuildLoad2(g->B, g->i64, g->fpr[f], ""), P(g, &cpu.fpr[f]), g->tb_state, 8);
    if (clear) g->dirty_f[f] = false;
  }
#endif
}

// The registers the region may have changed (already in cpu when written
// through or stored at block ends), and the budget.
static void t2_publish(T2 *g) {
  t2_st(g, LLVMBuildLoad2(g->B, g->i32, g->budget, ""), P(g, jit_budget_slot()), g->tb_state, 4);
  if (g->write_through || g->block_wb) return;
  for (int r = 1; r < 32; r++) {
    if (g->gpr_w[r]) t2_st(g, LLVMBuildLoad2(g->B, g->i64, g->gpr[r], ""), P(g, &cpu.gpr[r]), g->tb_state, 8);
  }
#ifndef CONFIG_FPU_NONE
  for (int f = 0; f < 32; f++) {
    if (g->fpr_w[f]) t2_st(g, LLVMBuildLoad2(g->B, g->i64, g->fpr[f], ""), P(g, &cpu.fpr[f]), g->tb_state, 8);
  }
#endif
}

// The instruction as a whole through the interpreter's helper, which reads
// and writes guest registers in cpu.
static void t2_helper(T2 *g, Decode *d, void *fn) {
  t2_publish(g);
  if (g->block_wb) t2_store_dirty(g, false);
  t2_st(g, t2_bound(g, d), P(g, jit_prev_s_slot()), g->tb_state, 8);
  LLVMValueRef a = t2_bound(g, d);
  t2_call(g, LLVMVoidTypeInContext(g->C), fn, &a, 1);
  int r = reg_of(d->dest.preg);
  if (r > 0) sx(g, r, t2_ld(g, g->i64, P(g, &cpu.gpr[r]), g->tb_state, 8));
#ifndef CONFIG_FPU_NONE
  int f = freg_of(d->dest.preg);
  if (f >= 0) sf(g, f, t2_ld(g, g->i64, P(g, &cpu.fpr[f]), g->tb_state, 8));
#endif
}

static void *t2_interp_fn(const Decode *d) {
  void *fn = (void *)jit_fp_lookup(d->exec_id);
  return fn ? fn : (void *)jit_eh_lookup(d->exec_id);
}

// Leaves the region with rec: the published state, then the record.
static LLVMBasicBlockRef t2_exit(T2 *g, JitExitRec *rec) {
  LLVMBasicBlockRef saved = LLVMGetInsertBlock(g->B);
  LLVMBasicBlockRef bb = t2_new_bb(g);
  LLVMPositionBuilderAtEnd(g->B, bb);
  if (g->block_wb) t2_store_dirty(g, false);
  LLVMValueRef v = t2_bound(g, rec);
  LLVMBuildBr(g->B, g->leave);
  LLVMAddIncoming(g->leave_rec, &v, &bb, 1);
  LLVMPositionBuilderAtEnd(g->B, saved);
  return bb;
}

static JitExitRec *t2_rec(T2 *g, sljit_sw kind, Decode *last, Decode *next, JitTrans *tr) {
  JitExitRec *r = &g->recs[g->nrecs++];
  *r = (JitExitRec){ .kind = kind, .last = last, .next = next, .trans = tr, .region = true };
  return r;
}

// FS dirty at the first floating-point write of each block, as tier 1.
static void t2_fs_dirty(T2 *g) {
  if (g->fs_dirty) return;
  g->fs_dirty = true;
  LLVMValueRef p = P(g, &csr_array[0x300]);
  LLVMValueRef v = t2_ld(g, g->i64, p, g->tb_csr, 8);
  t2_st(g, LLVMBuildOr(g->B, v, K(g, 3 << 13), ""), p, g->tb_csr, 8);
}

// ---------------------------------------------------------------------------
// Instructions (mirroring emit_instr() of tier 1)

typedef enum { T2_ADD, T2_SUB, T2_AND, T2_OR, T2_XOR, T2_SLL, T2_SRL, T2_SRA, T2_MUL } T2Op;

static LLVMValueRef t2_op(T2 *g, T2Op op, bool word, LLVMValueRef a, LLVMValueRef b) {
  if (word) { a = lo32(g, a); b = lo32(g, b); }
  LLVMValueRef m = word ? K32(g, 31) : K(g, 63), r = NULL;
  switch (op) {
    case T2_ADD: r = LLVMBuildAdd(g->B, a, b, ""); break;
    case T2_SUB: r = LLVMBuildSub(g->B, a, b, ""); break;
    case T2_AND: r = LLVMBuildAnd(g->B, a, b, ""); break;
    case T2_OR:  r = LLVMBuildOr(g->B, a, b, ""); break;
    case T2_XOR: r = LLVMBuildXor(g->B, a, b, ""); break;
    case T2_MUL: r = LLVMBuildMul(g->B, a, b, ""); break;
    case T2_SLL: r = LLVMBuildShl(g->B, a, LLVMBuildAnd(g->B, b, m, ""), ""); break;
    case T2_SRL: r = LLVMBuildLShr(g->B, a, LLVMBuildAnd(g->B, b, m, ""), ""); break;
    case T2_SRA: r = LLVMBuildAShr(g->B, a, LLVMBuildAnd(g->B, b, m, ""), ""); break;
  }
  return word ? sx64(g, r) : r;
}

enum { RF_GPR, RF_FPR64, RF_FPR32 };

// A loaded value (zero-extended for floating point) into its register.
static void t2_mem_result(T2 *g, Decode *d, int rf, int rd, LLVMValueRef v) {
  if (rf == RF_GPR) { sx(g, rd, v); return; }
  if (rf == RF_FPR32) v = LLVMBuildOr(g->B, v, K(g, (int64_t)0xffffffff00000000ull), "");
  sf(g, rd, v);
}

// The access itself at host address ha.
static void t2_mem_host(T2 *g, Decode *d, int width, bool store, bool sign, int rf, int rd,
                        LLVMTypeRef wt, LLVMValueRef data, LLVMValueRef ha) {
  LLVMBuilderRef B = g->B;
  LLVMValueRef hp = LLVMBuildIntToPtr(B, ha, g->ptr, "");
  if (store) {
    t2_st(g, width < 8 ? LLVMBuildTrunc(B, data, wt, "") : data, hp, g->tb_guest, width);
  } else {
    LLVMValueRef v = t2_ld(g, wt, hp, g->tb_guest, width);
    if (width < 8) v = sign && rf == RF_GPR ? sx64(g, v) : zx64(g, v);
    t2_mem_result(g, d, rf, rd, v);
  }
}

// A local of the region initialised in its entry block.
static LLVMValueRef t2_entry_local(T2 *g, LLVMValueRef init) {
  LLVMBuilderRef eb = LLVMCreateBuilderInContext(g->C);
  LLVMPositionBuilderBefore(eb, LLVMGetBasicBlockTerminator(g->entry));
  LLVMValueRef a = LLVMBuildAlloca(eb, LLVMTypeOf(init), "");
  LLVMBuildStore(eb, init, a);
  LLVMDisposeBuilder(eb);
  return a;
}

// Load or store with the host-TLB fast path of tier 1, behind a cache of the
// last page accessed through the same base register (translations do not
// change inside a region: every instruction that can change them leaves
// it).
// The width and direction of a translated load or store, 0 for others.
static int t2_mem_width(const Decode *d, bool *store) {
  *store = false;
  switch (d->exec_id) {
    case EXEC_ID_ld_mmu: case EXEC_ID_fld_mmu: return 8;
    case EXEC_ID_lw_mmu: case EXEC_ID_lwu_mmu: case EXEC_ID_flw_mmu: return 4;
    case EXEC_ID_lh_mmu: case EXEC_ID_lhu_mmu: return 2;
    case EXEC_ID_lb_mmu: case EXEC_ID_lbu_mmu: return 1;
    case EXEC_ID_sd_mmu: case EXEC_ID_fsd_mmu: *store = true; return 8;
    case EXEC_ID_sw_mmu: case EXEC_ID_fsw_mmu: *store = true; return 4;
    case EXEC_ID_sh_mmu: *store = true; return 2;
    case EXEC_ID_sb_mmu: *store = true; return 1;
    default: return 0;
  }
}

// Groups the loads (and, separately, the stores) of a block that use the same
// base register while it is not written, with naturally aligned offsets
// within one page. A group is checked once: when the whole range lies in one
// page that the host TLB maps and the base is aligned, every access of the
// group goes directly to its host address.
static void t2_groups(T2 *g, Decode *head, Decode *last) {
  int n = last - head + 1;
  if (n > g->grp_cap) { g->grp_cap = n; g->grp_of = realloc(g->grp_of, n * sizeof(int)); }
  g->ngrp = 0;
  int open[2][32];
  for (int s = 0; s < 2; s++) for (int r = 0; r < 32; r++) open[s][r] = -1;
  for (int i = 0; i < n; i++) {
    Decode *d = head + i;
    g->grp_of[i] = -1;
    bool store;
    int w = jit_opt.t2_groups ? t2_mem_width(d, &store) : 0;
    int base = reg_of(d->src1.preg);
    sljit_sw off = (sljit_sw)d->src2.imm;
    if (w && base > 0 && off % w == 0) {
      int k = open[store][base];
      if (k >= 0) {
        struct T2Group *gr = &g->grp[k];
        sljit_sw lo = MIN_OF(gr->lo, off), hi = MAX_OF(gr->hi, off + w);
        if (hi - lo > 4096) k = -1;
        else { gr->lo = lo; gr->hi = hi; gr->maxw = MAX_OF(gr->maxw, w); gr->count++; }
      }
      if (k < 0) {
        if (g->ngrp == g->grp_cap2) {
          g->grp_cap2 = g->grp_cap2 ? 2 * g->grp_cap2 : 64;
          g->grp = realloc(g->grp, g->grp_cap2 * sizeof(*g->grp));
        }
        k = g->ngrp++;
        g->grp[k] = (struct T2Group){ .first = d, .base = base, .maxw = w, .count = 1, .store = store,
                                      .lo = off, .hi = off + w };
        open[store][base] = k;
      }
      g->grp_of[i] = k;
    }
    // A write of the base register ends its groups (after this access).
    int rd = reg_of(d->dest.preg);
    if (rd > 0) open[0][rd] = open[1][rd] = -1;
  }
}

static struct T2Group *t2_group_of(T2 *g, Decode *d) {
  if (!jit_opt.t2_groups || g->cur_first == NULL) return NULL;
  int k = g->grp_of[d - g->cur_first];
  return k >= 0 && g->grp[k].count >= 2 ? &g->grp[k] : NULL;
}

static bool t2_mem(T2 *g, Decode *d, int width, bool store, bool sign, int rf) {
  int base = reg_of(d->src1.preg);
  int rd = rf == RF_GPR ? reg_of(d->dest.preg) : freg_of(d->dest.preg);
  if (base < 0 || (store && rd < 0) || (rf != RF_GPR && rd < 0)) return false;
  if (g->dry) return true;
  LLVMBuilderRef B = g->B;
  LLVMTypeRef wt = width == 1 ? g->i8 : width == 2 ? g->i16 : width == 4 ? g->i32 : g->i64;
  LLVMValueRef addr = LLVMBuildAdd(B, gx(g, base), K(g, (sljit_sw)d->src2.imm), "");
  uint64_t tag_mask = ~(uint64_t)PAGE_MASK | (uint64_t)(width - 1);
  int64_t slot_mask = (jit_hosttlb_size() - 1) << 4;
  uintptr_t table = (uintptr_t)jit_hosttlb_base() + (store ? jit_hosttlb_write_delta() : 0);
  LLVMValueRef key = LLVMBuildAnd(B, addr, K(g, tag_mask), "");
  LLVMValueRef data = store ? (rf == RF_GPR ? gx(g, rd) : gf(g, rd)) : NULL;
  struct T2Group *gp = t2_group_of(g, d);
  LLVMBasicBlockRef gjoin = NULL;
  if (gp) {
    if (gp->first == d) {
      // The group check, before its first access.
      LLVMValueRef b = gx(g, base);
      LLVMValueRef alo = LLVMBuildAdd(B, b, K(g, gp->lo), ""), ahi = LLVMBuildAdd(B, b, K(g, gp->hi - 1), "");
      LLVMValueRef same = LLVMBuildICmp(B, LLVMIntEQ, LLVMBuildLShr(B, LLVMBuildXor(B, alo, ahi, ""), K(g, PAGE_SHIFT), ""), K(g, 0), "");
      LLVMValueRef aligned = LLVMBuildICmp(B, LLVMIntEQ, LLVMBuildAnd(B, b, K(g, gp->maxw - 1), ""), K(g, 0), "");
      LLVMValueRef gslot = LLVMBuildAdd(B, LLVMBuildAnd(B, LLVMBuildLShr(B, alo, K(g, PAGE_SHIFT - 4), ""), K(g, slot_mask), ""),
                                        K(g, table), "");
      LLVMValueRef gtag = t2_ld(g, g->i64, LLVMBuildIntToPtr(B, LLVMBuildAdd(B, gslot, K(g, jit_hosttlb_gvpn_offset()), ""), g->ptr, ""),
                                g->tb_tlb, 8);
      LLVMValueRef goff = t2_ld(g, g->i64, LLVMBuildIntToPtr(B, LLVMBuildAdd(B, gslot, K(g, jit_hosttlb_offset_offset()), ""), g->ptr, ""),
                                g->tb_tlb, 8);
      LLVMValueRef hit = LLVMBuildICmp(B, LLVMIntEQ, gtag, LLVMBuildAnd(B, alo, K(g, ~(int64_t)PAGE_MASK), ""), "");
      gp->ok = LLVMBuildAnd(B, LLVMBuildAnd(B, same, aligned, ""), hit, "");
      gp->gbase = LLVMBuildAdd(B, goff, b, "");
    }
    LLVMBasicBlockRef direct = t2_new_bb(g), normal = t2_new_bb(g);
    gjoin = t2_new_bb(g);
    t2_condbr(g, gp->ok, direct, normal, g->likely);
    LLVMPositionBuilderAtEnd(B, direct);
    t2_mem_host(g, d, width, store, sign, rf, rd, wt, data, LLVMBuildAdd(B, gp->gbase, K(g, (sljit_sw)d->src2.imm), ""));
    LLVMBuildBr(B, gjoin);
    LLVMPositionBuilderAtEnd(B, normal);
  }
  LLVMBasicBlockRef join = t2_new_bb(g);
  LLVMValueRef *cpage = NULL, *coff = NULL;
  if (jit_opt.t2_utlb) {
    // The page last accessed through this base register (a key never equal
    // to the one of an aligned access until set) and its host offset.
    cpage = &g->utlb_page[store][base];
    coff = &g->utlb_off[store][base];
    if (*cpage == NULL) {
      *cpage = t2_entry_local(g, K(g, 1));
      *coff = t2_entry_local(g, K(g, 0));
    }
    LLVMBasicBlockRef cached = t2_new_bb(g), lookup = t2_new_bb(g);
    t2_condbr(g, LLVMBuildICmp(B, LLVMIntEQ, key, LLVMBuildLoad2(B, g->i64, *cpage, ""), ""), cached, lookup, g->likely);
    LLVMPositionBuilderAtEnd(B, cached);
    t2_mem_host(g, d, width, store, sign, rf, rd, wt, data, LLVMBuildAdd(B, LLVMBuildLoad2(B, g->i64, *coff, ""), addr, ""));
    LLVMBuildBr(B, join);
    LLVMPositionBuilderAtEnd(B, lookup);
  }
  LLVMValueRef slot = LLVMBuildAdd(B, LLVMBuildAnd(B, LLVMBuildLShr(B, addr, K(g, PAGE_SHIFT - 4), ""),
                                                K(g, slot_mask), ""), K(g, table), "");
  LLVMValueRef tag = t2_ld(g, g->i64, LLVMBuildIntToPtr(B, LLVMBuildAdd(B, slot, K(g, jit_hosttlb_gvpn_offset()), ""), g->ptr, ""),
                           g->tb_tlb, 8);
  LLVMValueRef hit = LLVMBuildICmp(B, LLVMIntEQ, tag, key, "");
  LLVMBasicBlockRef fast = t2_new_bb(g), slow = t2_new_bb(g);
  t2_condbr(g, hit, fast, slow, g->likely);

  LLVMPositionBuilderAtEnd(B, fast);
  LLVMValueRef off = t2_ld(g, g->i64, LLVMBuildIntToPtr(B, LLVMBuildAdd(B, slot, K(g, jit_hosttlb_offset_offset()), ""), g->ptr, ""),
                           g->tb_tlb, 8);
  if (cpage) {
    LLVMBuildStore(B, key, *cpage);
    LLVMBuildStore(B, off, *coff);
  }
  t2_mem_host(g, d, width, store, sign, rf, rd, wt, data, LLVMBuildAdd(B, off, addr, ""));
  LLVMBuildBr(B, join);

  // The slow path: the access through the interpreter's memory functions;
  // on a fault the region leaves before the instruction.
  LLVMPositionBuilderAtEnd(B, slow);
  t2_sync(g, d);
  LLVMValueRef ok;
  if (store) {
    LLVMValueRef a[4] = { t2_bound(g, d), addr, data, K32(g, width) };
    ok = t2_call(g, g->i8, (void *)jit_t2_store, a, 4);
  } else {
    LLVMValueRef a[4] = { t2_bound(g, d), addr, K32(g, width | (sign && rf == RF_GPR ? 0x100 : 0)), g->mem_out };
    ok = t2_call(g, g->i8, (void *)jit_t2_load, a, 4);
  }
  ok = LLVMBuildICmp(B, LLVMIntNE, ok, LLVMConstInt(g->i8, 0, 0), "");
  // The interpreter executes the access again: from the instruction, or,
  // for the first instruction of a block, by interpreting the block (a
  // mid-block exit there would enter the translation again).
  JitExitRec *rec = d == g->cur_head ? t2_rec(g, JIT_REC_FALLBACK, NULL, d, g->cur_tr)
                                     : t2_rec(g, JIT_REC_MIDBLOCK, NULL, d, g->cur_tr);
  LLVMBasicBlockRef done = t2_new_bb(g);
  t2_condbr(g, ok, done, t2_exit(g, rec), g->likely);
  LLVMPositionBuilderAtEnd(B, done);
  if (!store) t2_mem_result(g, d, rf, rd, LLVMBuildLoad2(B, g->i64, g->mem_out, ""));
  LLVMBuildBr(B, join);
  LLVMPositionBuilderAtEnd(B, join);
  if (gjoin) {
    LLVMBuildBr(B, gjoin);
    LLVMPositionBuilderAtEnd(B, gjoin);
  }
  if (!store && rf != RF_GPR) t2_fs_dirty(g);
  return true;
}

// Branches to a new block on ok (likely) and to the helper fn otherwise;
// returns the block that continues after both, to be closed by t2_fast_done().
static LLVMBasicBlockRef t2_fast_or_helper(T2 *g, Decode *d, LLVMValueRef ok, void *fn) {
  LLVMBasicBlockRef fast = t2_new_bb(g), slow = t2_new_bb(g), join = t2_new_bb(g);
  t2_condbr(g, ok, fast, slow, g->likely);
  LLVMPositionBuilderAtEnd(g->B, slow);
  t2_helper(g, d, fn);
  LLVMBuildBr(g->B, join);
  LLVMPositionBuilderAtEnd(g->B, fast);
  return join;
}

// Ends the fast path of an FP instruction, which wrote FP state.
static void t2_fast_done(T2 *g, LLVMBasicBlockRef join) {
  t2_fs_dirty(g);
  LLVMBuildBr(g->B, join);
  LLVMPositionBuilderAtEnd(g->B, join);
}

// A single-precision value properly NaN-boxed in 64 bits.
static LLVMValueRef t2_boxed(T2 *g, LLVMValueRef bits) {
  return LLVMBuildICmp(g->B, LLVMIntEQ, LLVMBuildLShr(g->B, bits, K(g, 32), ""), K(g, 0xffffffff), "");
}

#if defined(CONFIG_FPU_SOFT_HOST_FASTPATH) && !defined(CONFIG_FPU_NONE)
enum { FPK_ADD, FPK_SUB, FPK_MUL, FPK_DIV, FPK_FMA };

static LLVMValueRef fval(T2 *g, int f, bool w64) {
  LLVMValueRef bits = gf(g, f);
  return w64 ? LLVMBuildBitCast(g->B, bits, g->f64, "") : LLVMBuildBitCast(g->B, lo32(g, bits), g->f32, "");
}

static LLVMValueRef t2_intrinsic(T2 *g, const char *name, LLVMTypeRef ty, LLVMValueRef *args, unsigned n) {
  LLVMValueRef fn = LLVMGetIntrinsicDeclaration(g->M, LLVMLookupIntrinsicID(name, strlen(name)), &ty, 1);
  LLVMTypeRef params[3] = { ty, ty, ty };
  return LLVMBuildCall2(g->B, LLVMFunctionType(ty, params, n, 0), fn, args, n, "");
}

// Stores a result as tier 1 does: single precision NaN-boxed.
static void fset(T2 *g, int f, LLVMValueRef v, bool w64) {
  LLVMValueRef bits = w64 ? LLVMBuildBitCast(g->B, v, g->i64, "")
                          : LLVMBuildOr(g->B, zx64(g, LLVMBuildBitCast(g->B, v, g->i32, "")),
                                        K(g, (int64_t)0xffffffff00000000ull), "");
  sf(g, f, bits);
  t2_fs_dirty(g);
}

static LLVMValueRef t2_canonical_nan(T2 *g, LLVMValueRef r, bool w64);
static LLVMValueRef t2_result(T2 *g, LLVMValueRef r, bool w64);

// Speculative arithmetic (spec_fp), as emit_fp_arith_spec().
static bool t2_fp_arith(T2 *g, Decode *d, int kind, bool w64, bool na, bool nc) {
  // Unchecked host FP is only safe under an active leader that a follower
  // validates and can roll back. Outside leader mode the region uses the
  // exact interpreter helper instead.
  if (!jit_leader_mode) return false;
  int fd = freg_of(d->dest.preg), f1 = freg_of(d->src1.preg), f2 = freg_of(d->src2.preg);
  int rm = d->isa.instr.fp.rm;
  if (fd < 0 || f1 < 0 || f2 < 0 || (rm != 0 && rm != 7)) return false;
  if (kind == FPK_FMA && !t2_has_fma()) return false;
  if (g->dry) return true;
  LLVMValueRef a = fval(g, f1, w64), b = fval(g, f2, w64), r;
  if (kind == FPK_FMA) {
    if (na) a = LLVMBuildFNeg(g->B, a, "");
    LLVMValueRef c = fval(g, d->isa.instr.fp.funct5, w64);
    if (nc) c = LLVMBuildFNeg(g->B, c, "");
    LLVMValueRef args[3] = { a, b, c };
    r = t2_intrinsic(g, "llvm.fma", w64 ? g->f64 : g->f32, args, 3);
  } else {
    switch (kind) {
      case FPK_ADD: r = LLVMBuildFAdd(g->B, a, b, ""); break;
      case FPK_SUB: r = LLVMBuildFSub(g->B, a, b, ""); break;
      case FPK_MUL: r = LLVMBuildFMul(g->B, a, b, ""); break;
      default:      r = LLVMBuildFDiv(g->B, a, b, ""); break;
    }
  }
  fset(g, fd, t2_result(g, r, w64), w64);
  return true;
}

static bool t2_fp_cmp(T2 *g, Decode *d, LLVMRealPredicate pred, bool w64) {
  if (!jit_leader_mode) return false;
  int f1 = freg_of(d->src1.preg), f2 = freg_of(d->src2.preg), rd = reg_of(d->dest.preg);
  if (f1 < 0 || f2 < 0) return false;
  if (g->dry) return true;
  LLVMValueRef c = LLVMBuildFCmp(g->B, pred, fval(g, f1, w64), fval(g, f2, w64), "");
  sx(g, rd, zx64(g, c));
  return true;
}

// RISC-V's canonical NaN in place of any NaN result.
static LLVMValueRef t2_canonical_nan(T2 *g, LLVMValueRef r, bool w64) {
  LLVMValueRef qnan = w64 ? LLVMBuildBitCast(g->B, K(g, 0x7ff8000000000000ll), g->f64, "")
                          : LLVMBuildBitCast(g->B, K32(g, 0x7fc00000), g->f32, "");
  return LLVMBuildSelect(g->B, LLVMBuildFCmp(g->B, LLVMRealUNO, r, r, ""), qnan, r, "");
}

// The result of speculative arithmetic. A NaN result is the host's own NaN
// (the sign bit set), where RISC-V has its canonical NaN: the slice then
// fails validation if the NaN's bits are observed. After a rollback the
// leader replaces NaN results by the canonical NaN (t2_canon_nan), which
// costs a compare and a select per operation (up to a third of the time of
// floating-point loops) but avoids rolling back again and again on programs
// that produce NaNs.
static LLVMValueRef t2_result(T2 *g, LLVMValueRef r, bool w64) {
  return jit_opt.t2_canon_nan ? t2_canonical_nan(g, r, w64) : r;
}

// Square root, speculative as the arithmetic; a negative operand gives the
// canonical NaN.
static bool t2_fp_sqrt(T2 *g, Decode *d, bool w64) {
  if (!jit_leader_mode) return false;
  int fd = freg_of(d->dest.preg), f1 = freg_of(d->src1.preg);
  int rm = d->isa.instr.fp.rm;
  if (fd < 0 || f1 < 0 || (rm != 0 && rm != 7)) return false;
  if (g->dry) return true;
  LLVMValueRef a = fval(g, f1, w64);
  LLVMValueRef r = t2_intrinsic(g, "llvm.sqrt", w64 ? g->f64 : g->f32, &a, 1);
  fset(g, fd, t2_result(g, r, w64), w64);
  return true;
}

// fmin/fmax of two numbers (single precision properly NaN-boxed); NaN
// operands take the helper, which also raises the invalid flag for
// signaling NaNs. Equal operands differ at most in the sign of zero:
// minimum ORs the bits (-0 wins), maximum ANDs them.
static bool t2_fp_minmax(T2 *g, Decode *d, bool w64, bool max) {
  if (!jit_leader_mode) return false;
  int fd = freg_of(d->dest.preg), f1 = freg_of(d->src1.preg), f2 = freg_of(d->src2.preg);
  if (fd < 0 || f1 < 0 || f2 < 0 || jit_helper_fn(d) == NULL) return false;
  if (g->dry) return true;
  LLVMBuilderRef B = g->B;
  LLVMValueRef ab = gf(g, f1), bb = gf(g, f2), a = fval(g, f1, w64), b = fval(g, f2, w64);
  LLVMValueRef ok = LLVMBuildFCmp(B, LLVMRealORD, a, b, "");
  if (!w64) ok = LLVMBuildAnd(B, ok, LLVMBuildAnd(B, t2_boxed(g, ab), t2_boxed(g, bb), ""), "");
  LLVMBasicBlockRef join = t2_fast_or_helper(g, d, ok, jit_helper_fn(d));
  LLVMValueRef first = LLVMBuildFCmp(B, max ? LLVMRealOGT : LLVMRealOLT, a, b, "");
  LLVMValueRef second = LLVMBuildFCmp(B, max ? LLVMRealOGT : LLVMRealOLT, b, a, "");
  LLVMValueRef eq = max ? LLVMBuildAnd(B, ab, bb, "") : LLVMBuildOr(B, ab, bb, "");
  LLVMValueRef r = LLVMBuildSelect(B, first, ab, LLVMBuildSelect(B, second, bb, eq, ""), "");
  sf(g, fd, r);
  t2_fast_done(g, join);
  return true;
}

// Conversion to a signed integer rounding toward zero (the C conversion)
// for values whose result is in range; others, and NaN, take the helper.
// Inexact is raised by the host conversion, like the arithmetic's flags.
static bool t2_fcvt_int(T2 *g, Decode *d, bool w64, bool l) {
  if (!jit_leader_mode) return false;
  int rd = reg_of(d->dest.preg), f1 = freg_of(d->src1.preg);
  if (f1 < 0 || d->isa.instr.fp.rm != 1 || jit_helper_fn(d) == NULL) return false;
  if (g->dry) return true;
  LLVMBuilderRef B = g->B;
  LLVMValueRef v = fval(g, f1, w64);
  if (!w64) v = LLVMBuildFPExt(B, v, g->f64, "");
  double lo = l ? -9223372036854775808.0 : -2147483649.0, hi = l ? 9223372036854775808.0 : 2147483648.0;
  LLVMValueRef ok = LLVMBuildAnd(B, LLVMBuildFCmp(B, l ? LLVMRealOGE : LLVMRealOGT, v, LLVMConstReal(g->f64, lo), ""),
                                    LLVMBuildFCmp(B, LLVMRealOLT, v, LLVMConstReal(g->f64, hi), ""), "");
  if (!w64) ok = LLVMBuildAnd(B, ok, t2_boxed(g, gf(g, f1)), "");
  LLVMBasicBlockRef join = t2_fast_or_helper(g, d, ok, jit_helper_fn(d));
  LLVMValueRef r = l ? LLVMBuildFPToSI(B, v, g->i64, "") : sx64(g, LLVMBuildFPToSI(B, v, g->i32, ""));
  sx(g, rd, r);
  t2_fast_done(g, join);
  return true;
}

// Integer to double; a dynamic rounding mode other than a valid one takes
// the helper, as in tier 1.
static bool t2_fcvt_d_int(T2 *g, Decode *d, bool is_signed, bool w32) {
  if (!jit_leader_mode) return false;
  int fd = freg_of(d->dest.preg), rs1 = reg_of(d->src1.preg);
  int rm = d->isa.instr.fp.rm;
  if (fd < 0 || rs1 < 0 || rm == 5 || rm == 6) return false;
  if (g->dry) return true;
  LLVMValueRef a = gx(g, rs1);
  if (w32) a = lo32(g, a);
  LLVMValueRef v = is_signed ? LLVMBuildSIToFP(g->B, a, g->f64, "") : LLVMBuildUIToFP(g->B, a, g->f64, "");
  if (rm != 7) {
    fset(g, fd, v, true);
    return true;
  }
  LLVMValueRef frm = LLVMBuildAnd(g->B, t2_ld(g, g->i64, P(g, &csr_array[0x003]), g->tb_csr, 8), K(g, 0xe0), "");
  LLVMBasicBlockRef join = t2_fast_or_helper(g, d, LLVMBuildICmp(g->B, LLVMIntULE, frm, K(g, 4 << 5), ""),
                                             t2_interp_fn(d));
  fset(g, fd, v, true);
  LLVMBuildBr(g->B, join);
  LLVMPositionBuilderAtEnd(g->B, join);
  return true;
}
#endif

static bool t2_instr(T2 *g, Decode *d) {
  int rd  = reg_of(d->dest.preg);
  int rs1 = reg_of(d->src1.preg);
  int rs2 = reg_of(d->src2.preg);
  sljit_sw i1 = (sljit_sw)d->src1.imm;
  sljit_sw i2 = (sljit_sw)d->src2.imm;
  LLVMBuilderRef B = g->B;

#define DRY() do { if (g->dry) return true; } while (0)
#define RR(op, w) do { if (rs1 < 0 || rs2 < 0) return false; DRY(); sx(g, rd, t2_op(g, op, w, gx(g, rs1), gx(g, rs2))); return true; } while (0)
#define RI(op, w) do { if (rs1 < 0) return false; DRY(); sx(g, rd, t2_op(g, op, w, gx(g, rs1), K(g, i2))); return true; } while (0)
#define CRR(op, w) do { if (rs2 < 0) return false; DRY(); if (rd >= 0) sx(g, rd, t2_op(g, op, w, gx(g, rd), gx(g, rs2))); return true; } while (0)
#define CRI(op, w) do { DRY(); if (rd >= 0) sx(g, rd, t2_op(g, op, w, gx(g, rd), K(g, i2))); return true; } while (0)
#define LI(v) do { DRY(); sx(g, rd, K(g, v)); return true; } while (0)
#define CALL2(fn) do { if (rs1 < 0 || rs2 < 0) return false; DRY(); \
    LLVMValueRef a_[2] = { gx(g, rs1), gx(g, rs2) }; sx(g, rd, t2_call_pure(g, g->i64, (void *)fn, a_, 2)); return true; } while (0)

  switch (d->exec_id) {
    case EXEC_ID_add:  RR(T2_ADD, false);
    case EXEC_ID_sub:  RR(T2_SUB, false);
    case EXEC_ID_and:  RR(T2_AND, false);
    case EXEC_ID_or:   RR(T2_OR,  false);
    case EXEC_ID_xor:  RR(T2_XOR, false);
    case EXEC_ID_sll:  RR(T2_SLL, false);
    case EXEC_ID_srl:  RR(T2_SRL, false);
    case EXEC_ID_sra:  RR(T2_SRA, false);
    case EXEC_ID_addw: RR(T2_ADD, true);
    case EXEC_ID_subw: RR(T2_SUB, true);
    case EXEC_ID_sllw: RR(T2_SLL, true);
    case EXEC_ID_srlw: RR(T2_SRL, true);
    case EXEC_ID_sraw: RR(T2_SRA, true);
    case EXEC_ID_mul:  RR(T2_MUL, false);
    case EXEC_ID_mulw: RR(T2_MUL, true);
    case EXEC_ID_slt:
    case EXEC_ID_sltu:
      if (rs1 < 0 || rs2 < 0) return false;
      DRY();
      sx(g, rd, zx64(g, LLVMBuildICmp(B, d->exec_id == EXEC_ID_slt ? LLVMIntSLT : LLVMIntULT, gx(g, rs1), gx(g, rs2), "")));
      return true;

    case EXEC_ID_addi:  RI(T2_ADD, false);
    case EXEC_ID_andi:  RI(T2_AND, false);
    case EXEC_ID_ori:   RI(T2_OR,  false);
    case EXEC_ID_xori:  RI(T2_XOR, false);
    case EXEC_ID_slli:  RI(T2_SLL, false);
    case EXEC_ID_srli:  RI(T2_SRL, false);
    case EXEC_ID_srai:  RI(T2_SRA, false);
    case EXEC_ID_addiw: RI(T2_ADD, true);
    case EXEC_ID_slliw: RI(T2_SLL, true);
    case EXEC_ID_srliw: RI(T2_SRL, true);
    case EXEC_ID_sraiw: RI(T2_SRA, true);
    case EXEC_ID_slti:
    case EXEC_ID_sltui:
      if (rs1 < 0) return false;
      DRY();
      sx(g, rd, zx64(g, LLVMBuildICmp(B, d->exec_id == EXEC_ID_slti ? LLVMIntSLT : LLVMIntULT, gx(g, rs1), K(g, i2), "")));
      return true;

    case EXEC_ID_lui:
    case EXEC_ID_auipc:  LI(i1);
    case EXEC_ID_c_li:   LI(i2);
    case EXEC_ID_p_li_0: LI(0);
    case EXEC_ID_p_li_1: LI(1);
    case EXEC_ID_c_mv:
      if (rs1 < 0) return false;
      DRY();
      sx(g, rd, gx(g, rs1));
      return true;
    case EXEC_ID_p_sext_w:
      if (rs1 < 0) return false;
      DRY();
      sx(g, rd, sx64(g, lo32(g, gx(g, rs1))));
      return true;
    case EXEC_ID_c_addi:  CRI(T2_ADD, false);
    case EXEC_ID_c_addiw: CRI(T2_ADD, true);
    case EXEC_ID_c_andi:  CRI(T2_AND, false);
    case EXEC_ID_c_slli:  CRI(T2_SLL, false);
    case EXEC_ID_c_srli:  CRI(T2_SRL, false);
    case EXEC_ID_c_srai:  CRI(T2_SRA, false);
    case EXEC_ID_p_inc: DRY(); if (rd >= 0) sx(g, rd, LLVMBuildAdd(B, gx(g, rd), K(g, 1), "")); return true;
    case EXEC_ID_p_dec: DRY(); if (rd >= 0) sx(g, rd, LLVMBuildSub(B, gx(g, rd), K(g, 1), "")); return true;
    case EXEC_ID_c_add:  CRR(T2_ADD, false);
    case EXEC_ID_c_and:  CRR(T2_AND, false);
    case EXEC_ID_c_or:   CRR(T2_OR,  false);
    case EXEC_ID_c_xor:  CRR(T2_XOR, false);
    case EXEC_ID_c_sub:  CRR(T2_SUB, false);
    case EXEC_ID_c_addw: CRR(T2_ADD, true);
    case EXEC_ID_c_subw: CRR(T2_SUB, true);

    case EXEC_ID_div:    CALL2(jit_helper_div);
    case EXEC_ID_divu:   CALL2(jit_helper_divu);
    case EXEC_ID_rem:    CALL2(jit_helper_rem);
    case EXEC_ID_remu:   CALL2(jit_helper_remu);
    case EXEC_ID_divw:   CALL2(jit_helper_divw);
    case EXEC_ID_divuw:  CALL2(jit_helper_divuw);
    case EXEC_ID_remw:   CALL2(jit_helper_remw);
    case EXEC_ID_remuw:  CALL2(jit_helper_remuw);
    case EXEC_ID_mulh:   CALL2(jit_helper_mulh);
    case EXEC_ID_mulhu:  CALL2(jit_helper_mulhu);
    case EXEC_ID_mulhsu: CALL2(jit_helper_mulhsu);

    case EXEC_ID_ld_mmu:  return jit_opt.mem && t2_mem(g, d, 8, false, true, RF_GPR);
    case EXEC_ID_lw_mmu:  return jit_opt.mem && t2_mem(g, d, 4, false, true, RF_GPR);
    case EXEC_ID_lh_mmu:  return jit_opt.mem && t2_mem(g, d, 2, false, true, RF_GPR);
    case EXEC_ID_lb_mmu:  return jit_opt.mem && t2_mem(g, d, 1, false, true, RF_GPR);
    case EXEC_ID_lwu_mmu: return jit_opt.mem && t2_mem(g, d, 4, false, false, RF_GPR);
    case EXEC_ID_lhu_mmu: return jit_opt.mem && t2_mem(g, d, 2, false, false, RF_GPR);
    case EXEC_ID_lbu_mmu: return jit_opt.mem && t2_mem(g, d, 1, false, false, RF_GPR);
    case EXEC_ID_sd_mmu:  return jit_opt.mem && t2_mem(g, d, 8, true, false, RF_GPR);
    case EXEC_ID_sw_mmu:  return jit_opt.mem && t2_mem(g, d, 4, true, false, RF_GPR);
    case EXEC_ID_sh_mmu:  return jit_opt.mem && t2_mem(g, d, 2, true, false, RF_GPR);
    case EXEC_ID_sb_mmu:  return jit_opt.mem && t2_mem(g, d, 1, true, false, RF_GPR);
#ifndef CONFIG_FPU_NONE
    case EXEC_ID_fld_mmu: return jit_opt.mem && t2_mem(g, d, 8, false, false, RF_FPR64);
    case EXEC_ID_flw_mmu: return jit_opt.mem && t2_mem(g, d, 4, false, false, RF_FPR32);
    case EXEC_ID_fsd_mmu: return jit_opt.mem && t2_mem(g, d, 8, true, false, RF_FPR64);
    case EXEC_ID_fsw_mmu: return jit_opt.mem && t2_mem(g, d, 4, true, false, RF_FPR64);
    case EXEC_ID_fmv_x_d: {
      int fs1 = freg_of(d->src1.preg);
      if (fs1 < 0) return false;
      DRY();
      sx(g, rd, gf(g, fs1));
      return true;
    }
    case EXEC_ID_fmv_d_x: {
      int fd = freg_of(d->dest.preg);
      if (fd < 0 || rs1 < 0) return false;
      DRY();
      sf(g, fd, gx(g, rs1));
      t2_fs_dirty(g);
      return true;
    }
    case EXEC_ID_fmv_w_x: {
      int fd = freg_of(d->dest.preg);
      if (fd < 0 || rs1 < 0) return false;
      DRY();
      sf(g, fd, LLVMBuildOr(B, gx(g, rs1), K(g, (int64_t)0xffffffff00000000ull), ""));
      t2_fs_dirty(g);
      return true;
    }
    case EXEC_ID_fmv_x_w: {
      int fs1 = freg_of(d->src1.preg);
      if (fs1 < 0) return false;
      DRY();
      sx(g, rd, sx64(g, lo32(g, gf(g, fs1))));
      return true;
    }
#ifdef CONFIG_FPU_SOFT_HOST_FASTPATH
    case EXEC_ID_faddd: return jit_leader_mode && jit_opt.inline_fp && t2_fp_arith(g, d, FPK_ADD, true, false, false);
    case EXEC_ID_fsubd: return jit_leader_mode && jit_opt.inline_fp && t2_fp_arith(g, d, FPK_SUB, true, false, false);
    case EXEC_ID_fmuld: return jit_leader_mode && jit_opt.inline_fp && t2_fp_arith(g, d, FPK_MUL, true, false, false);
    case EXEC_ID_fdivd: return jit_leader_mode && jit_opt.inline_fp && t2_fp_arith(g, d, FPK_DIV, true, false, false);
    case EXEC_ID_fmaddd:  return jit_leader_mode && jit_opt.inline_fp && t2_fp_arith(g, d, FPK_FMA, true, false, false);
    case EXEC_ID_fmsubd:  return jit_leader_mode && jit_opt.inline_fp && t2_fp_arith(g, d, FPK_FMA, true, false, true);
    case EXEC_ID_fnmsubd: return jit_leader_mode && jit_opt.inline_fp && t2_fp_arith(g, d, FPK_FMA, true, true, false);
    case EXEC_ID_fnmaddd: return jit_leader_mode && jit_opt.inline_fp && t2_fp_arith(g, d, FPK_FMA, true, true, true);
    case EXEC_ID_fadds: return jit_leader_mode && jit_opt.inline_fp && t2_fp_arith(g, d, FPK_ADD, false, false, false);
    case EXEC_ID_fsubs: return jit_leader_mode && jit_opt.inline_fp && t2_fp_arith(g, d, FPK_SUB, false, false, false);
    case EXEC_ID_fmuls: return jit_leader_mode && jit_opt.inline_fp && t2_fp_arith(g, d, FPK_MUL, false, false, false);
    case EXEC_ID_fdivs: return jit_leader_mode && jit_opt.inline_fp && t2_fp_arith(g, d, FPK_DIV, false, false, false);
    case EXEC_ID_fmadds:  return jit_leader_mode && jit_opt.inline_fp && t2_fp_arith(g, d, FPK_FMA, false, false, false);
    case EXEC_ID_fmsubs:  return jit_leader_mode && jit_opt.inline_fp && t2_fp_arith(g, d, FPK_FMA, false, false, true);
    case EXEC_ID_fnmsubs: return jit_leader_mode && jit_opt.inline_fp && t2_fp_arith(g, d, FPK_FMA, false, true, false);
    case EXEC_ID_fnmadds: return jit_leader_mode && jit_opt.inline_fp && t2_fp_arith(g, d, FPK_FMA, false, true, true);
    case EXEC_ID_fltd: return jit_leader_mode && jit_opt.inline_fp && t2_fp_cmp(g, d, LLVMRealOLT, true);
    case EXEC_ID_fled: return jit_leader_mode && jit_opt.inline_fp && t2_fp_cmp(g, d, LLVMRealOLE, true);
    case EXEC_ID_feqd: return jit_leader_mode && jit_opt.inline_fp && t2_fp_cmp(g, d, LLVMRealOEQ, true);
    case EXEC_ID_flts: return jit_leader_mode && jit_opt.inline_fp && t2_fp_cmp(g, d, LLVMRealOLT, false);
    case EXEC_ID_fles: return jit_leader_mode && jit_opt.inline_fp && t2_fp_cmp(g, d, LLVMRealOLE, false);
    case EXEC_ID_feqs: return jit_leader_mode && jit_opt.inline_fp && t2_fp_cmp(g, d, LLVMRealOEQ, false);
    case EXEC_ID_fcvt_d_w:  return jit_leader_mode && jit_opt.inline_fp && t2_fcvt_d_int(g, d, true, true);
    case EXEC_ID_fcvt_d_wu: return jit_leader_mode && jit_opt.inline_fp && t2_fcvt_d_int(g, d, false, true);
    case EXEC_ID_fcvt_d_l:  return jit_leader_mode && jit_opt.inline_fp && t2_fcvt_d_int(g, d, true, false);
    case EXEC_ID_fcvt_d_lu: return jit_leader_mode && jit_opt.inline_fp && t2_fcvt_d_int(g, d, false, false);
    case EXEC_ID_fsqrtd: return jit_leader_mode && jit_opt.inline_fp && t2_fp_sqrt(g, d, true);
    case EXEC_ID_fsqrts: return jit_leader_mode && jit_opt.inline_fp && t2_fp_sqrt(g, d, false);
    case EXEC_ID_fmind: return jit_leader_mode && jit_opt.inline_fp && t2_fp_minmax(g, d, true, false);
    case EXEC_ID_fmaxd: return jit_leader_mode && jit_opt.inline_fp && t2_fp_minmax(g, d, true, true);
    case EXEC_ID_fmins: return jit_leader_mode && jit_opt.inline_fp && t2_fp_minmax(g, d, false, false);
    case EXEC_ID_fmaxs: return jit_leader_mode && jit_opt.inline_fp && t2_fp_minmax(g, d, false, true);
    case EXEC_ID_fcvt_w_d: return jit_leader_mode && jit_opt.inline_fp && t2_fcvt_int(g, d, true, false);
    case EXEC_ID_fcvt_l_d: return jit_leader_mode && jit_opt.inline_fp && t2_fcvt_int(g, d, true, true);
    case EXEC_ID_fcvt_w_s: return jit_leader_mode && jit_opt.inline_fp && t2_fcvt_int(g, d, false, false);
    case EXEC_ID_fcvt_l_s: return jit_leader_mode && jit_opt.inline_fp && t2_fcvt_int(g, d, false, true);
    case EXEC_ID_fcvt_s_d: {
      int fd = freg_of(d->dest.preg), f1 = freg_of(d->src1.preg);
      int rm = d->isa.instr.fp.rm;
      if (!jit_leader_mode || !jit_opt.inline_fp || fd < 0 || f1 < 0 || (rm != 0 && rm != 7)) return false;
      DRY();
      LLVMValueRef r = LLVMBuildFPTrunc(B, fval(g, f1, true), g->f32, "");
      fset(g, fd, t2_result(g, r, false), false);
      return true;
    }
    case EXEC_ID_fcvt_d_s: {
      int fd = freg_of(d->dest.preg), f1 = freg_of(d->src1.preg);
      if (!jit_leader_mode || !jit_opt.inline_fp || fd < 0 || f1 < 0) return false;
      DRY();
      fset(g, fd, t2_result(g, LLVMBuildFPExt(B, fval(g, f1, false), g->f64, ""), true), true);
      return true;
    }
#endif
    case EXEC_ID_fsgnjs:
    case EXEC_ID_fsgnjns:
    case EXEC_ID_fsgnjxs: {
      // Only properly NaN-boxed operands are handled inline.
      int fd = freg_of(d->dest.preg), f1 = freg_of(d->src1.preg), f2 = freg_of(d->src2.preg);
      if (fd < 0 || f1 < 0 || f2 < 0) return false;
      DRY();
      LLVMValueRef a = gf(g, f1), b = gf(g, f2);
      LLVMBasicBlockRef join = t2_fast_or_helper(g, d, LLVMBuildAnd(B, t2_boxed(g, a), t2_boxed(g, b), ""),
                                                 t2_interp_fn(d));
      if (d->exec_id == EXEC_ID_fsgnjns) b = LLVMBuildXor(B, b, K(g, -1), "");
      if (d->exec_id == EXEC_ID_fsgnjxs) b = LLVMBuildXor(B, b, a, "");
      int64_t sign = 1u << 31;
      sf(g, fd, LLVMBuildOr(B, LLVMBuildAnd(B, a, K(g, ~sign), ""), LLVMBuildAnd(B, b, K(g, sign), ""), ""));
      t2_fast_done(g, join);
      return true;
    }
    case EXEC_ID_fsgnjd:
    case EXEC_ID_fsgnjnd:
    case EXEC_ID_fsgnjxd: {
      int fd = freg_of(d->dest.preg), f1 = freg_of(d->src1.preg), f2 = freg_of(d->src2.preg);
      if (fd < 0 || f1 < 0 || f2 < 0) return false;
      DRY();
      LLVMValueRef a = gf(g, f1), b = gf(g, f2);
      if (d->exec_id == EXEC_ID_fsgnjnd) b = LLVMBuildXor(B, b, K(g, -1), "");
      if (d->exec_id == EXEC_ID_fsgnjxd) b = LLVMBuildXor(B, b, a, "");
      int64_t sign = (int64_t)(1ull << 63);
      sf(g, fd, LLVMBuildOr(B, LLVMBuildAnd(B, a, K(g, ~sign), ""), LLVMBuildAnd(B, b, K(g, sign), ""), ""));
      t2_fs_dirty(g);
      return true;
    }
#endif

#ifdef CONFIG_RVB
    case EXEC_ID_sh1add: case EXEC_ID_sh2add: case EXEC_ID_sh3add:
    case EXEC_ID_sh1adduw: case EXEC_ID_sh2adduw: case EXEC_ID_sh3adduw: case EXEC_ID_adduw: {
      if (rs1 < 0 || rs2 < 0) return false;
      DRY();
      int sh = d->exec_id == EXEC_ID_sh1add || d->exec_id == EXEC_ID_sh1adduw ? 1
             : d->exec_id == EXEC_ID_sh2add || d->exec_id == EXEC_ID_sh2adduw ? 2
             : d->exec_id == EXEC_ID_adduw ? 0 : 3;
      bool uw = d->exec_id == EXEC_ID_sh1adduw || d->exec_id == EXEC_ID_sh2adduw ||
                d->exec_id == EXEC_ID_sh3adduw || d->exec_id == EXEC_ID_adduw;
      LLVMValueRef a = gx(g, rs1);
      if (uw) a = zx64(g, lo32(g, a));
      sx(g, rd, LLVMBuildAdd(B, LLVMBuildShl(B, a, K(g, sh), ""), gx(g, rs2), ""));
      return true;
    }
    case EXEC_ID_slliuw:
      if (rs1 < 0) return false;
      DRY();
      sx(g, rd, LLVMBuildShl(B, zx64(g, lo32(g, gx(g, rs1))), K(g, i2 & 0x3f), ""));
      return true;
    case EXEC_ID_andn: case EXEC_ID_orn: case EXEC_ID_xnor: {
      if (rs1 < 0 || rs2 < 0) return false;
      DRY();
      LLVMValueRef nb = LLVMBuildXor(B, gx(g, rs2), K(g, -1), "");
      LLVMValueRef a = gx(g, rs1);
      sx(g, rd, d->exec_id == EXEC_ID_andn ? LLVMBuildAnd(B, a, nb, "")
              : d->exec_id == EXEC_ID_orn ? LLVMBuildOr(B, a, nb, "") : LLVMBuildXor(B, a, nb, ""));
      return true;
    }
    case EXEC_ID_min: case EXEC_ID_max: case EXEC_ID_minu: case EXEC_ID_maxu: {
      if (rs1 < 0 || rs2 < 0) return false;
      DRY();
      // rd = cond(rs1, rs2) ? rs2 : rs1
      LLVMIntPredicate p = d->exec_id == EXEC_ID_min ? LLVMIntSGT : d->exec_id == EXEC_ID_max ? LLVMIntSLT
                         : d->exec_id == EXEC_ID_minu ? LLVMIntUGT : LLVMIntULT;
      LLVMValueRef a = gx(g, rs1), b = gx(g, rs2);
      sx(g, rd, LLVMBuildSelect(B, LLVMBuildICmp(B, p, a, b, ""), b, a, ""));
      return true;
    }
    case EXEC_ID_sext_b: case EXEC_ID_sext_h: case EXEC_ID_zext_h: {
      if (rs1 < 0) return false;
      DRY();
      LLVMValueRef v = LLVMBuildTrunc(B, gx(g, rs1), d->exec_id == EXEC_ID_sext_b ? g->i8 : g->i16, "");
      sx(g, rd, d->exec_id == EXEC_ID_zext_h ? zx64(g, v) : sx64(g, v));
      return true;
    }
#define UNARY(id, fn, w) case EXEC_ID_##id: { if (rs1 < 0) return false; DRY(); \
      LLVMValueRef a_ = w ? lo32(g, gx(g, rs1)) : gx(g, rs1); \
      LLVMValueRef r_ = t2_call_pure(g, w ? g->i32 : g->i64, (void *)fn, &a_, 1); \
      sx(g, rd, w ? sx64(g, r_) : r_); return true; }
#define IMMOP(id, fn, w) case EXEC_ID_##id: { if (rs1 < 0) return false; DRY(); \
      LLVMValueRef a_[2] = { w ? lo32(g, gx(g, rs1)) : gx(g, rs1), w ? K32(g, i2) : K(g, i2) }; \
      LLVMValueRef r_ = t2_call_pure(g, w ? g->i32 : g->i64, (void *)fn, a_, 2); \
      sx(g, rd, w ? sx64(g, r_) : r_); return true; }
#define BINOP(id, fn, w) case EXEC_ID_##id: { if (rs1 < 0 || rs2 < 0) return false; DRY(); \
      LLVMValueRef a_[2] = { w ? lo32(g, gx(g, rs1)) : gx(g, rs1), w ? lo32(g, gx(g, rs2)) : gx(g, rs2) }; \
      LLVMValueRef r_ = t2_call_pure(g, w ? g->i32 : g->i64, (void *)fn, a_, 2); \
      sx(g, rd, w ? sx64(g, r_) : r_); return true; }
    UNARY(clz, _rv_clz, false) UNARY(ctz, _rv_ctz, false) UNARY(cpop, _rv_cpop, false)
    UNARY(clzw, _rv32_clz, true) UNARY(ctzw, _rv32_ctz, true) UNARY(cpopw, _rv32_cpop, true)
    UNARY(rev8, _rv_rev8, false) UNARY(orc_b, _rv_orc_b, false) UNARY(revb, _rv_revb, false)
    BINOP(rol, _rv_rol, false) BINOP(ror, _rv_ror, false)
    BINOP(rolw, _rv32_rol, true) BINOP(rorw, _rv32_ror, true)
    IMMOP(rori, _rv_ror, false) IMMOP(roriw, _rv32_ror, true)
    BINOP(bclr, _rv_bclr, false) BINOP(bset, _rv_bset, false) BINOP(binv, _rv_binv, false) BINOP(bext, _rv_bext, false)
    IMMOP(bclri, _rv_bclr, false) IMMOP(bseti, _rv_bset, false) IMMOP(binvi, _rv_binv, false) IMMOP(bexti, _rv_bext, false)
    BINOP(clmul, _rv_clmul, false) BINOP(clmulh, _rv_clmulh, false) BINOP(clmulr, _rv_clmulr, false)
    BINOP(pack, _rv_pack, false) BINOP(packh, _rv_packh, false) BINOP(packw, _rv32_pack, true)
#undef UNARY
#undef IMMOP
#undef BINOP
#endif

#ifdef CONFIG_RV_ZICOND
    case EXEC_ID_czero_eqz: case EXEC_ID_czero_nez: {
      if (rs1 < 0 || rs2 < 0) return false;
      DRY();
      LLVMValueRef z = LLVMBuildICmp(B, d->exec_id == EXEC_ID_czero_eqz ? LLVMIntEQ : LLVMIntNE, gx(g, rs2), K(g, 0), "");
      sx(g, rd, LLVMBuildSelect(B, z, K(g, 0), gx(g, rs1), ""));
      return true;
    }
#endif

#ifdef CONFIG_RV_ZCB
    case EXEC_ID_c_zext_b: case EXEC_ID_c_sext_b: case EXEC_ID_c_zext_h: case EXEC_ID_c_sext_h: case EXEC_ID_c_zext_w: {
      if (rs1 < 0) return false;
      DRY();
      int id = d->exec_id;
      LLVMTypeRef ty = id == EXEC_ID_c_zext_b || id == EXEC_ID_c_sext_b ? g->i8
                     : id == EXEC_ID_c_zext_w ? g->i32 : g->i16;
      LLVMValueRef v = LLVMBuildTrunc(B, gx(g, rs1), ty, "");
      sx(g, rd, id == EXEC_ID_c_sext_b || id == EXEC_ID_c_sext_h ? sx64(g, v) : zx64(g, v));
      return true;
    }
    case EXEC_ID_c_not:
      if (rs1 < 0) return false;
      DRY();
      sx(g, rd, LLVMBuildXor(B, gx(g, rs1), K(g, -1), ""));
      return true;
    case EXEC_ID_c_mul: RR(T2_MUL, false);
#endif

    default:
      return false;
  }
#undef DRY
#undef RR
#undef RI
#undef CRR
#undef CRI
#undef LI
#undef CALL2
}

// A straight-line instruction: inline, or through the interpreter's helper
// where tier 1 would use it.
static bool t2_straight(T2 *g, Decode *d) {
  if (t2_instr(g, d)) return true;
  void *fn = jit_helper_fn(d);
  if (fn == NULL) return false;
  if (!g->dry) t2_helper(g, d, fn);
  return true;
}

// ---------------------------------------------------------------------------
// Blocks and terminators

// Number of direct successors of a terminator the region can follow (0 for
// indirect jumps, which leave the unit), or -1.
static int t2_nsucc(const Decode *d) {
  switch (d->exec_id) {
    case EXEC_ID_jalr: case EXEC_ID_c_jr: case EXEC_ID_c_jalr: case EXEC_ID_p_ret:
      return jit_opt.t2_units ? 0 : -1;
    case EXEC_ID_jal: case EXEC_ID_p_jal: case EXEC_ID_c_j:
      return 1;
    case EXEC_ID_beq: case EXEC_ID_bne: case EXEC_ID_blt: case EXEC_ID_bge:
    case EXEC_ID_bltu: case EXEC_ID_bgeu: case EXEC_ID_c_beqz: case EXEC_ID_c_bnez:
    case EXEC_ID_p_blez: case EXEC_ID_p_bgtz: case EXEC_ID_p_bltz: case EXEC_ID_p_bgez:
      return 2;
    default:
      return -1;
  }
}

static int t2_index(T2 *g, const Decode *head) {
  for (int i = 0; i < g->nb; i++) if (g->b[i].head == head) return i;
  return -1;
}

// Edge `which` of block i. It stays in the region while the budget lasts and
// the successor is in the region and already accounted in this epoch;
// otherwise it leaves through the exit record tier 1 uses for the same edge
// when it is not chained, so the glue and the interpreter continue exactly
// as they would after tier 1 (including the first entry of the successor in
// an epoch).
static LLVMBasicBlockRef t2_edge(T2 *g, int i, int which, LLVMValueRef budget) {
  T2Block *b = &g->b[i];
  JitExitRec *rec = t2_rec(g, which == 0 ? JIT_REC_TAKEN : JIT_REC_NTAKEN, b->last, NULL, b->tr);
  LLVMBasicBlockRef out = t2_exit(g, rec);
  int s = b->succ[which];
  if (s < 0) return out;
  LLVMBasicBlockRef saved = LLVMGetInsertBlock(g->B);
  LLVMBasicBlockRef bb = t2_new_bb(g), check = t2_new_bb(g);
  LLVMPositionBuilderAtEnd(g->B, bb);
  if (g->all_charged) {
    // Every block of the region was charged in this epoch when it was
    // entered, and neither the epoch nor a stamp changes inside the region.
    LLVMBasicBlockRef go = t2_new_bb(g);
    t2_condbr(g, LLVMBuildICmp(g->B, LLVMIntSLE, budget, K32(g, 0), ""), out, go, g->unlikely);
    LLVMPositionBuilderAtEnd(g->B, go);
    t2_condbr(g, g->all_charged, g->body[s], check, g->likely);
  } else {
    t2_condbr(g, LLVMBuildICmp(g->B, LLVMIntSLE, budget, K32(g, 0), ""), out, check, g->unlikely);
  }
  LLVMPositionBuilderAtEnd(g->B, check);
  LLVMValueRef epoch = t2_ld(g, g->i32, P(g, &tcache_uts_epoch), g->tb_shadow, 4);
  LLVMValueRef stamp = t2_ld(g, g->i32, t2_bound(g, &jit_uts_stamp[g->b[s].tr->id]), g->tb_shadow, 4);
  t2_condbr(g, LLVMBuildICmp(g->B, LLVMIntEQ, epoch, stamp, ""), g->body[s], out, g->likely);
  LLVMPositionBuilderAtEnd(g->B, saved);
  return bb;
}

// Terminator d of block i (i is not used by a dry run).
static bool t2_term(T2 *g, int i, Decode *d) {
  int rd = reg_of(d->dest.preg), rs1 = reg_of(d->src1.preg), rs2 = reg_of(d->src2.preg);
  sljit_sw i2 = (sljit_sw)d->src2.imm;
  LLVMIntPredicate pred = LLVMIntEQ;
  LLVMValueRef a = NULL, b = NULL;
  switch (d->exec_id) {
    case EXEC_ID_jal:
    case EXEC_ID_p_jal:
    case EXEC_ID_c_j: {
      if (g->dry) return true;
      if (d->exec_id == EXEC_ID_jal) sx(g, rd, K(g, i2));
      if (d->exec_id == EXEC_ID_p_jal) sx(g, 1, K(g, i2));
      if (g->block_wb) t2_store_dirty(g, true);
      LLVMValueRef nb = LLVMBuildSub(g->B, LLVMBuildLoad2(g->B, g->i32, g->budget, ""), K32(g, d->idx_in_bb), "");
      LLVMBuildStore(g->B, nb, g->budget);
      LLVMBuildBr(g->B, t2_edge(g, i, 0, nb));
      return true;
    }
#define BR(p, x, y) do { if (x < 0 || y < 0) return false; if (g->dry) return true; \
                         pred = p; a = gx(g, x); b = gx(g, y); } while (0)
    case EXEC_ID_beq:  BR(LLVMIntEQ,  rs1, rs2); break;
    case EXEC_ID_bne:  BR(LLVMIntNE,  rs1, rs2); break;
    case EXEC_ID_blt:  BR(LLVMIntSLT, rs1, rs2); break;
    case EXEC_ID_bge:  BR(LLVMIntSGE, rs1, rs2); break;
    case EXEC_ID_bltu: BR(LLVMIntULT, rs1, rs2); break;
    case EXEC_ID_bgeu: BR(LLVMIntUGE, rs1, rs2); break;
    case EXEC_ID_c_beqz: BR(LLVMIntEQ,  rs1, 0); break;
    case EXEC_ID_c_bnez: BR(LLVMIntNE,  rs1, 0); break;
    case EXEC_ID_p_blez: BR(LLVMIntSGE, 0, rs2); break;
    case EXEC_ID_p_bgtz: BR(LLVMIntSLT, 0, rs2); break;
    case EXEC_ID_p_bltz: BR(LLVMIntSLT, rs1, 0); break;
    case EXEC_ID_p_bgez: BR(LLVMIntSGE, rs1, 0); break;
#undef BR
    case EXEC_ID_jalr:
    case EXEC_ID_c_jr:
    case EXEC_ID_c_jalr:
    case EXEC_ID_p_ret: {
      // As emit_indirect(): the target is published for the glue.
      if (d->exec_id != EXEC_ID_p_ret && rs1 < 0) return false;
      if (g->dry) return true;
      LLVMValueRef target;
      switch (d->exec_id) {
        case EXEC_ID_jalr:
          target = LLVMBuildAnd(g->B, LLVMBuildAdd(g->B, gx(g, rs1), K(g, i2), ""), K(g, ~(int64_t)1), "");
          sx(g, rd, K(g, (sljit_sw)d->snpc));
          break;
        case EXEC_ID_c_jalr:
          // The interpreter writes the link register before reading rs1.
          sx(g, 1, K(g, (sljit_sw)d->snpc));
          target = gx(g, rs1);
          break;
        case EXEC_ID_c_jr: target = gx(g, rs1); break;
        default: target = gx(g, 1); break;
      }
      if (g->block_wb) t2_store_dirty(g, true);
      LLVMValueRef nb = LLVMBuildSub(g->B, LLVMBuildLoad2(g->B, g->i32, g->budget, ""), K32(g, d->idx_in_bb), "");
      LLVMBuildStore(g->B, nb, g->budget);
      t2_st(g, target, P(g, &jit_exit_target), g->tb_state, 8);
      LLVMBuildBr(g->B, t2_exit(g, t2_rec(g, JIT_REC_INDIRECT, d, NULL, g->b[i].tr)));
      return true;
    }
    default:
      return false;
  }
  LLVMValueRef cond = LLVMBuildICmp(g->B, pred, a, b, "");
  if (g->block_wb) t2_store_dirty(g, true);
  LLVMValueRef nb = LLVMBuildSub(g->B, LLVMBuildLoad2(g->B, g->i32, g->budget, ""), K32(g, d->idx_in_bb), "");
  LLVMBuildStore(g->B, nb, g->budget);
  LLVMBasicBlockRef taken = t2_edge(g, i, 0, nb), ntaken = t2_edge(g, i, 1, nb);
  LLVMBuildCondBr(g->B, cond, taken, ntaken);
  return true;
}

// A block can join a region when its whole tier-1 translation is active and
// every instruction and its terminator are supported.
static bool t2_block_ok(T2 *g, Decode *head, T2Block *out) {
  if (head == NULL || head->jit_state != JIT_ACTIVE || head->jit_code == NULL) return false;
  JitTrans *tr = head->jit_code;
  if (!tr->complete) return false;
  Decode *last = head;
  while (last->type == INSTR_TYPE_N) last++;
  int nsucc = t2_nsucc(last);
  if (nsucc < 0) return false;
  bool dry = g->dry;
  g->dry = true;
  bool ok = true;
  for (Decode *d = head; d != last && ok; d++) ok = t2_straight(g, d);
  if (ok) ok = t2_term(g, -1, last);
  g->dry = dry;
  if (!ok) return false;
  *out = (T2Block){ .head = head, .last = last, .tr = tr, .nsucc = nsucc, .succ = { -1, -1 } };
  return true;
}

// The blocks of the loop at head: breadth-first over direct successors
// through eligible blocks, then only those from which head is reached again.
// Returns false when head is not on a cycle.
static bool t2_form(T2 *g, Decode *head) {
  g->nb = 0;
  g->dry = true;
  if (!t2_block_ok(g, head, &g->b[0])) return false;
  g->nb = 1;
  int instrs = g->b[0].last->idx_in_bb;
  for (int i = 0; i < g->nb; i++) {
    for (int w = 0; w < g->b[i].nsucc; w++) {
      Decode *s = w == 0 ? g->b[i].last->tnext : g->b[i].last->ntnext;
      int k = t2_index(g, s);
      if (k < 0 && g->nb < MIN_OF(jit_opt.t2_blocks, T2_MAX_BLOCKS) && t2_block_ok(g, s, &g->b[g->nb]) &&
          instrs + g->b[g->nb].last->idx_in_bb <= jit_opt.t2_instrs) {
        instrs += g->b[g->nb].last->idx_in_bb;
        k = g->nb++;
      }
      g->b[i].succ[w] = k;
    }
  }
  if (jit_opt.t2_units) {
    // A unit: every block found; worth compiling when it returns to its head,
    // or when it is long enough to run more instructions per entry than a
    // unit must to stay (see t2_prune()).
    bool loop = false;
    for (int i = 0; i < g->nb; i++)
      for (int w = 0; w < g->b[i].nsucc; w++) loop |= g->b[i].succ[w] == 0;
    return loop || instrs >= MAX_OF(jit_opt.t2_min_instrs, jit_opt.t2_residency);
  }
  // A loop region: the blocks that reach the head.
  g->b[0].keep = true;
  for (bool changed = true; changed; ) {
    changed = false;
    for (int i = 1; i < g->nb; i++) {
      if (g->b[i].keep) continue;
      for (int w = 0; w < g->b[i].nsucc; w++) {
        int s = g->b[i].succ[w];
        if (s >= 0 && g->b[s].keep) { g->b[i].keep = changed = true; break; }
      }
    }
  }
  int map[T2_MAX_BLOCKS], n = 0;
  for (int i = 0; i < g->nb; i++) map[i] = g->b[i].keep ? n++ : -1;
  bool loop = false;
  for (int i = 0; i < g->nb; i++) {
    if (!g->b[i].keep) continue;
    T2Block b = g->b[i];
    for (int w = 0; w < b.nsucc; w++) {
      b.succ[w] = b.succ[w] >= 0 ? map[b.succ[w]] : -1;
      if (b.succ[w] == 0) loop = true;
    }
    g->b[map[i]] = b;
  }
  g->nb = n;
  return loop;
}

// Registers used and written by the region's instructions (a conservative
// superset: every register operand), prepared in the entry block.
static void t2_prepare_regs(T2 *g) {
  bool gu[32] = {}, fu[32] = {};
  for (int i = 0; i < g->nb; i++) {
    for (Decode *d = g->b[i].head; ; d++) {
      const rtlreg_t *ops[3] = { d->dest.preg, d->src1.preg, d->src2.preg };
      for (int k = 0; k < 3; k++) {
        int r = reg_of(ops[k]), f = freg_of(ops[k]);
        if (r > 0) gu[r] = true;
        if (f >= 0) fu[f] = true;
      }
      int r = reg_of(d->dest.preg), f = freg_of(d->dest.preg);
      if (r > 0) g->gpr_w[r] = true;
      if (f >= 0) g->fpr_w[f] = true;
      switch (d->exec_id) {
        case EXEC_ID_p_jal: case EXEC_ID_c_jalr: gu[1] = g->gpr_w[1] = true; break;
        case EXEC_ID_p_ret: gu[1] = true; break;
#ifndef CONFIG_FPU_NONE
        case EXEC_ID_fmaddd: case EXEC_ID_fmsubd: case EXEC_ID_fnmsubd: case EXEC_ID_fnmaddd:
        case EXEC_ID_fmadds: case EXEC_ID_fmsubs: case EXEC_ID_fnmsubs: case EXEC_ID_fnmadds:
          fu[d->isa.instr.fp.funct5] = true;
          break;
#endif
      }
      if (d == g->b[i].last) break;
    }
  }
  for (int r = 1; r < 32; r++) {
    if (!gu[r] && !g->gpr_w[r]) continue;
    g->gpr[r] = LLVMBuildAlloca(g->B, g->i64, "");
    LLVMBuildStore(g->B, t2_ld(g, g->i64, P(g, &cpu.gpr[r]), g->tb_state, 8), g->gpr[r]);
  }
#ifndef CONFIG_FPU_NONE
  for (int f = 0; f < 32; f++) {
    if (!fu[f] && !g->fpr_w[f]) continue;
    g->fpr[f] = LLVMBuildAlloca(g->B, g->i64, "");
    LLVMBuildStore(g->B, t2_ld(g, g->i64, P(g, &cpu.fpr[f]), g->tb_state, 8), g->fpr[f]);
  }
#endif
}

static LLVMValueRef t2_tbaa_tag(T2 *g, LLVMMetadataRef root, const char *name) {
  LLVMMetadataRef zero = LLVMValueAsMetadata(LLVMConstInt(g->i64, 0, 0));
  LLVMMetadataRef tn[3] = { LLVMMDStringInContext2(g->C, name, strlen(name)), root, zero };
  LLVMMetadataRef type = LLVMMDNodeInContext2(g->C, tn, 3);
  LLVMMetadataRef tag[3] = { type, type, zero };
  return LLVMMetadataAsValue(g->C, LLVMMDNodeInContext2(g->C, tag, 3));
}

static LLVMValueRef t2_weights(T2 *g, unsigned a, unsigned b) {
  LLVMMetadataRef w[3] = { LLVMMDStringInContext2(g->C, "branch_weights", 14),
                           LLVMValueAsMetadata(LLVMConstInt(g->i32, a, 0)),
                           LLVMValueAsMetadata(LLVMConstInt(g->i32, b, 0)) };
  return LLVMMetadataAsValue(g->C, LLVMMDNodeInContext2(g->C, w, 3));
}

// Builds the region's function: entry (budget), then the blocks; the head's
// first entry is checked by the gate, the others' by the edges into them.
static void t2_build(T2 *g, const char *name) {
  LLVMContextRef C = g->C;
  g->i1 = LLVMInt1TypeInContext(C); g->i8 = LLVMInt8TypeInContext(C); g->i16 = LLVMInt16TypeInContext(C);
  g->i32 = LLVMInt32TypeInContext(C); g->i64 = LLVMInt64TypeInContext(C);
  g->f32 = LLVMFloatTypeInContext(C); g->f64 = LLVMDoubleTypeInContext(C);
  g->ptr = LLVMPointerTypeInContext(C, 0);
  g->md_tbaa = LLVMGetMDKindIDInContext(C, "tbaa", 4);
  g->md_prof = LLVMGetMDKindIDInContext(C, "prof", 4);
  LLVMMetadataRef rs = LLVMMDStringInContext2(C, "nemu tier 2", 11);
  LLVMMetadataRef root = LLVMMDNodeInContext2(C, &rs, 1);
  g->tb_guest = t2_tbaa_tag(g, root, "guest memory");
  g->tb_tlb = t2_tbaa_tag(g, root, "host tlb");
  g->tb_shadow = t2_tbaa_tag(g, root, "shadow");
  g->tb_csr = t2_tbaa_tag(g, root, "csr");
  g->tb_state = t2_tbaa_tag(g, root, "state");
  g->tb_bind = t2_tbaa_tag(g, root, "binding");
  g->likely = t2_weights(g, 2000, 1);
  g->unlikely = t2_weights(g, 1, 2000);

  g->F = LLVMAddFunction(g->M, name, LLVMFunctionType(g->ptr, &g->ptr, 1, 0));
  g->bind_arg = LLVMGetParam(g->F, 0);
  {
    // Laid out after the region: its address bounds the region's code.
    LLVMValueRef e = LLVMAddFunction(g->M, "t2_region_end", LLVMFunctionType(LLVMVoidTypeInContext(C), NULL, 0, 0));
    LLVMBuilderRef eb = LLVMCreateBuilderInContext(C);
    LLVMPositionBuilderAtEnd(eb, LLVMAppendBasicBlockInContext(C, e, ""));
    LLVMBuildRetVoid(eb);
    LLVMDisposeBuilder(eb);
  }
  g->B = LLVMCreateBuilderInContext(C);
  LLVMBasicBlockRef entry = g->entry = t2_new_bb(g);
  for (int i = 0; i < g->nb; i++) g->body[i] = t2_new_bb(g);
  LLVMPositionBuilderAtEnd(g->B, entry);
  g->write_through = false;
  g->block_wb = true;
  g->budget = LLVMBuildAlloca(g->B, g->i32, "");
  g->mem_out = LLVMBuildAlloca(g->B, g->i64, "");
  t2_prepare_regs(g);
  LLVMBuildStore(g->B, t2_ld(g, g->i32, P(g, jit_budget_slot()), g->tb_state, 4), g->budget);
  LLVMValueRef ep = t2_bound(g, &g->stats->entries);
  t2_st(g, LLVMBuildAdd(g->B, t2_ld(g, g->i64, ep, g->tb_state, 8), K(g, 1), ""), ep, g->tb_state, 8);
  g->budget0 = LLVMBuildLoad2(g->B, g->i32, g->budget, "");
  {
    LLVMValueRef epoch = t2_ld(g, g->i32, P(g, &tcache_uts_epoch), g->tb_shadow, 4);
    LLVMValueRef all = LLVMConstInt(g->i1, 1, 0);
    for (int i = 1; i < g->nb; i++) {
      LLVMValueRef stamp = t2_ld(g, g->i32, t2_bound(g, &jit_uts_stamp[g->b[i].tr->id]), g->tb_shadow, 4);
      all = LLVMBuildAnd(g->B, all, LLVMBuildICmp(g->B, LLVMIntEQ, stamp, epoch, ""), "");
    }
    g->all_charged = all;
  }
  LLVMBuildBr(g->B, g->body[0]);

  g->leave = t2_new_bb(g);
  LLVMPositionBuilderAtEnd(g->B, g->leave);
  g->leave_rec = LLVMBuildPhi(g->B, g->ptr, "");
  t2_publish(g);
  LLVMValueRef xp = t2_bound(g, &g->stats->executed);
  LLVMValueRef ran = zx64(g, LLVMBuildSub(g->B, g->budget0, LLVMBuildLoad2(g->B, g->i32, g->budget, ""), ""));
  t2_st(g, LLVMBuildAdd(g->B, t2_ld(g, g->i64, xp, g->tb_state, 8), ran, ""), xp, g->tb_state, 8);
  LLVMBuildRet(g->B, g->leave_rec);

  g->dry = false;
  for (int i = 0; i < g->nb; i++) {
    T2Block *b = &g->b[i];
    g->cur_tr = b->tr;
    g->cur_head = b->head;
    memset(g->dirty_g, 0, sizeof(g->dirty_g));
    memset(g->dirty_f, 0, sizeof(g->dirty_f));
    LLVMPositionBuilderAtEnd(g->B, g->body[i]);
    g->fs_dirty = false;
    t2_groups(g, b->head, b->last);
    g->cur_first = b->head;
    for (Decode *d = b->head; d != b->last; d++) {
      bool ok = t2_straight(g, d);
      Assert(ok, "JIT tier 2: instruction %s changed support", jit_exec_name(d->exec_id));
    }
    bool ok = t2_term(g, i, b->last);
    Assert(ok, "JIT tier 2: terminator %s changed support", jit_exec_name(b->last->exec_id));
  }
  LLVMDisposeBuilder(g->B);
}

// ---------------------------------------------------------------------------
// Compilation, installation, lifetime


static uint64_t t2_hash(const char *s, size_t n) {
  uint64_t h = 1469598103934665603ull;
  for (size_t i = 0; i < n; i++) h = (h ^ (unsigned char)s[i]) * 1099511628211ull;
  return h;
}

static T2Code *t2_code_find(const char *key, size_t klen, uint64_t hash) {
  for (T2Code *c = t2_codes; c; c = c->next)
    if (c->hash == hash && c->klen == klen && memcmp(c->key, key, klen) == 0) return c;
  return NULL;
}

static char *t2_dup(const char *s, size_t n) { return memcpy(malloc(n), s, n); }

// Takes the key (allocated with malloc()).
static T2Code *t2_code_add(char *key, size_t klen, uint64_t hash, LLVMOrcExecutorAddress fn,
                           LLVMOrcExecutorAddress end) {
  T2Code *c = calloc(1, sizeof(*c));
  c->key = key;
  c->klen = klen;
  c->hash = hash;
  c->fn = fn;
  c->next = t2_codes;
  t2_codes = c;
  jit_stats.t2_compiled++;
  if (end > fn) t2_profile_note(fn, end, T2P_UNIT, c);
  return c;
}

// Names the function (and its end marker) uniquely in the JIT.
static void t2_seq_name(uint32_t seq, char *name, size_t len) { snprintf(name, len, "t2_region_%u", seq); }

static void t2_name(LLVMModuleRef M, LLVMValueRef F, uint32_t seq, char *name, size_t len) {
  t2_seq_name(seq, name, len);
  LLVMSetValueName2(F, name, strlen(name));
  LLVMValueRef endf = LLVMGetNamedFunction(M, "t2_region_end");
  if (endf) {
    char end[48];
    snprintf(end, sizeof(end), "%s_end", name);
    LLVMSetValueName2(endf, end, strlen(end));
  }
}

static void t2_verify(LLVMModuleRef M) {
  char *msg = NULL;
  if (LLVMVerifyModule(M, LLVMReturnStatusAction, &msg)) {
    char *ir = LLVMPrintModuleToString(M);
    fprintf(stderr, "%s\n", ir);
    panic("JIT tier 2: invalid IR: %s", msg);
  }
  LLVMDisposeMessage(msg);
}

static void t2_optimise(LLVMModuleRef M) {
  LLVMPassBuilderOptionsRef po = LLVMCreatePassBuilderOptions();
  char pipeline[32];
  snprintf(pipeline, sizeof(pipeline), "default<O%d>", jit_opt.t2_opt);
  t2_check(LLVMRunPasses(M, pipeline, t2_tm, po), "optimisation");
  LLVMDisposePassBuilderOptions(po);
}

// The address of a function linked into the JIT, published for perf.
static LLVMOrcExecutorAddress t2_lookup(const char *name, vaddr_t pc, LLVMOrcExecutorAddress *end_out) {
  LLVMOrcExecutorAddress fn;
  t2_check(LLVMOrcLLJITLookup(t2_jit, &fn, name), "code generation");
  char end[48];
  snprintf(end, sizeof(end), "%s_end", name);
  LLVMOrcExecutorAddress e;
  t2_check(LLVMOrcLLJITLookup(t2_jit, &e, end), "code generation");
  *end_out = e;
  return fn;
}

// Compiles the module in this process.
static LLVMOrcExecutorAddress t2_compile_here(T2 *g, LLVMOrcThreadSafeContextRef tsc, Decode *head,
                                              LLVMOrcExecutorAddress *end) {
  t2_verify(g->M);
  t2_optimise(g->M);
  char name[32];
  t2_name(g->M, g->F, t2_seq++, name, sizeof(name));
  // Code lives until the end of the run.
  t2_check(LLVMOrcLLJITAddLLVMIRModule(t2_jit, LLVMOrcLLJITGetMainJITDylib(t2_jit),
                                        LLVMOrcCreateNewThreadSafeModule(g->M, tsc)), "adding the module");
  return t2_lookup(name, head->pc, end);
}

// ---------------------------------------------------------------------------
// Compilation in a separate process (t2-async)
//
// The leader stays single-threaded, so that forking followers stays safe:
// modules go as bitcode to a compile process forked by the leader, which
// optimises them and returns relocatable objects; the leader links them
// when they arrive and installs the region if the translations it was built
// from are still current. The process belongs to the leader: it is started
// at the first request, dies with the leader (and with its process group at
// a rollback), and followers drop their copy of the connection.

// Messages are small and carry the payload (bitcode or object) as a file
// descriptor of an in-memory file, so that neither side ever blocks writing
// while the other writes too.
typedef struct { uint32_t seq, len; uint64_t ns; } T2Msg;

typedef struct T2Pending {
  uint32_t seq;
  T2Region *r;          // NULL once the translations it was built for are gone
  Decode *head;
  char *key;
  size_t klen;
  uint64_t hash;
  char name[32];
  struct T2Pending *next;
} T2Pending;

#define T2_MAX_SERVERS 16
static int t2_srv[T2_MAX_SERVERS];     // connections to the compile processes
static int t2_srv_busy[T2_MAX_SERVERS]; // modules sent and not returned
static int t2_nsrv;
static T2Pending *t2_pending;

static bool t2_io(int fd, void *buf, size_t len, bool rd) {
  for (size_t done = 0; done < len; ) {
    ssize_t n = rd ? read(fd, (char *)buf + done, len - done) : write(fd, (char *)buf + done, len - done);
    if (n < 0 && errno == EINTR) continue;
    if (n <= 0) return false;
    done += n;
  }
  return true;
}

static bool t2_send(int sock, const T2Msg *m, const void *payload) {
  int fd = memfd_create("t2", MFD_CLOEXEC);
  if (fd < 0) return false;
  bool ok = t2_io(fd, (void *)payload, m->len, false);
  union { struct cmsghdr h; char buf[CMSG_SPACE(sizeof(int))]; } u;
  struct iovec iov = { .iov_base = (void *)m, .iov_len = sizeof(*m) };
  struct msghdr mh = { .msg_iov = &iov, .msg_iovlen = 1, .msg_control = u.buf, .msg_controllen = sizeof(u.buf) };
  struct cmsghdr *c = CMSG_FIRSTHDR(&mh);
  c->cmsg_level = SOL_SOCKET;
  c->cmsg_type = SCM_RIGHTS;
  c->cmsg_len = CMSG_LEN(sizeof(int));
  memcpy(CMSG_DATA(c), &fd, sizeof(int));
  ok = ok && sendmsg(sock, &mh, 0) == (ssize_t)sizeof(*m);
  close(fd);
  return ok;
}

// Receives a message and its payload (malloc'ed); 0 when none is waiting
// (dontwait), -1 when the connection is gone.
static int t2_recv(int sock, T2Msg *m, char **payload, bool dontwait) {
  union { struct cmsghdr h; char buf[CMSG_SPACE(sizeof(int))]; } u;
  struct iovec iov = { .iov_base = m, .iov_len = sizeof(*m) };
  struct msghdr mh = { .msg_iov = &iov, .msg_iovlen = 1, .msg_control = u.buf, .msg_controllen = sizeof(u.buf) };
  ssize_t n;
  do { n = recvmsg(sock, &mh, dontwait ? MSG_DONTWAIT : 0); } while (n < 0 && errno == EINTR);
  if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return 0;
  struct cmsghdr *c = CMSG_FIRSTHDR(&mh);
  if (n != (ssize_t)sizeof(*m) || c == NULL || c->cmsg_type != SCM_RIGHTS) return -1;
  int fd;
  memcpy(&fd, CMSG_DATA(c), sizeof(int));
  *payload = malloc(m->len ? m->len : 1);
  bool ok = pread(fd, *payload, m->len, 0) == (ssize_t)m->len;
  close(fd);
  if (!ok) { free(*payload); return -1; }
  return 1;
}


_Noreturn static void t2_server(int fd) {
  for (;;) {
    T2Msg m;
    char *bc;
    if (t2_recv(fd, &m, &bc, false) != 1) _exit(0);
    uint64_t start = t2_now_ns();
    LLVMContextRef C = LLVMContextCreate();
    LLVMMemoryBufferRef in = LLVMCreateMemoryBufferWithMemoryRange(bc, m.len, "t2", 0);
    LLVMModuleRef M;
    if (LLVMParseBitcodeInContext2(C, in, &M)) _exit(1);
    char name[32];
    t2_name(M, LLVMGetNamedFunction(M, "t2_region"), m.seq, name, sizeof(name));
    t2_verify(M);
    t2_optimise(M);
    char *err = NULL;
    LLVMMemoryBufferRef out;
    if (LLVMTargetMachineEmitToMemoryBuffer(t2_tm, M, LLVMObjectFile, &err, &out)) _exit(1);
    T2Msg r = { .seq = m.seq, .len = LLVMGetBufferSize(out), .ns = t2_now_ns() - start };
    if (!t2_send(fd, &r, LLVMGetBufferStart(out))) _exit(0);
    LLVMDisposeMemoryBuffer(out);
    LLVMDisposeModule(M);
    LLVMDisposeMemoryBuffer(in);
    LLVMContextDispose(C);
    free(bc);
  }
}

static bool t2_server_start(int k) {
  int sv[2];
  if (socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, sv) != 0) return false;
  fflush(NULL);
  pid_t pid = fork();
  if (pid < 0) { close(sv[0]); close(sv[1]); return false; }
  if (pid == 0) {
    prctl(PR_SET_PDEATHSIG, SIGKILL);
    // Compilation uses only time the leader and the followers leave idle.
    struct sched_param sp = { 0 };
    sched_setscheduler(0, SCHED_IDLE, &sp);
    // Nothing of the leader's but the connection: in particular not its
    // connection to the coordinator, whose end of file reports the leader.
    for (int fd = 3; fd < 4096; fd++) if (fd != sv[1]) close(fd);
    t2_server(sv[1]);
  }
  close(sv[1]);
  t2_srv[k] = sv[0];
  t2_srv_busy[k] = 0;
  return true;
}

static void t2_pending_free(T2Pending *p) {
  free(p->key);
  free(p);
}

// Frees a region that was not installed; its blocks may start another.
static void t2_region_drop(T2Region *r) {
  for (int i = 0; i < r->nmembers; i++) if (t2_member[r->members[i]] == r->gen) t2_member[r->members[i]] = 0;
  free(r->bind);
  free(r->recs);
  free(r);
}

void jit_t2_stop(void) {
  t2_profile_close();
  for (int k = 0; k < t2_nsrv; k++) close(t2_srv[k]);
  t2_nsrv = 0;
  while (t2_pending) {
    T2Pending *p = t2_pending;
    t2_pending = p->next;
    if (p->r) t2_region_drop(p->r);
    t2_pending_free(p);
  }
}

static void t2_install(T2Region *r, T2Code *c);

// Links the objects that arrived and installs their regions.
static void t2_poll(void) {
  for (int k = 0; k < t2_nsrv && t2_pending; ) {
    T2Msg m;
    char *obj;
    int rc = t2_recv(t2_srv[k], &m, &obj, true);
    if (rc == 0) { k++; continue; }
    if (rc < 0) { jit_t2_stop(); return; }   // a compile process is gone
    t2_srv_busy[k]--;
    T2Pending **pp = &t2_pending;
    while (*pp && (*pp)->seq != m.seq) pp = &(*pp)->next;
    Assert(*pp, "JIT tier 2: unknown compile result %u", m.seq);
    T2Pending *p = *pp;
    *pp = p->next;
    uint32_t mxcsr = _mm_getcsr();
    LLVMMemoryBufferRef buf = LLVMCreateMemoryBufferWithMemoryRangeCopy(obj, m.len, p->name);
    free(obj);
    t2_check(LLVMOrcLLJITAddObjectFile(t2_jit, LLVMOrcLLJITGetMainJITDylib(t2_jit), buf), "linking an object");
    LLVMOrcExecutorAddress end;
    LLVMOrcExecutorAddress fn = t2_lookup(p->name, p->head->pc, &end);
    _mm_setcsr(mxcsr);
    T2Code *c = t2_code_add(p->key, p->klen, p->hash, fn, end);
    p->key = NULL;
    jit_stats.t2_compile_ns += m.ns;
    if (p->r) t2_install(p->r, c);
    t2_pending_free(p);
  }
}

// Sends the module to the least busy compile process; false when there is
// none.
static bool t2_request(T2Region *r, Decode *head, const char *key, size_t klen, uint64_t hash) {
  if (t2_nsrv == 0) {
    int n = MIN_OF(MAX_OF(jit_opt.t2_servers, 1), T2_MAX_SERVERS);
    while (t2_nsrv < n && t2_server_start(t2_nsrv)) t2_nsrv++;
    if (t2_nsrv == 0) return false;
  }
  int k = 0;
  for (int i = 1; i < t2_nsrv; i++) if (t2_srv_busy[i] < t2_srv_busy[k]) k = i;
  T2Pending *p = calloc(1, sizeof(*p));
  p->seq = t2_seq++;
  p->r = r;
  p->head = head;
  p->key = t2_dup(key, klen);
  p->klen = klen;
  p->hash = hash;
  t2_seq_name(p->seq, p->name, sizeof(p->name));   // the compile process names it
  T2Msg m = { .seq = p->seq, .len = klen };
  bool ok = t2_send(t2_srv[k], &m, key);
  if (!ok) { t2_pending_free(p); jit_t2_stop(); return false; }
  t2_srv_busy[k]++;
  p->next = t2_pending;
  t2_pending = p;
  jit_stats.t2_requests++;
  return true;
}

static bool t2_compile(Decode *head) {
  static T2 tg;
  T2 *g = &tg;
  free(g->bind);
  // The group tables are reused from one compilation to the next.
  int *grp_of = g->grp_of, grp_cap = g->grp_cap, grp_cap2 = g->grp_cap2;
  struct T2Group *grp = g->grp;
  memset(g, 0, sizeof(*g));
  g->grp_of = grp_of; g->grp_cap = grp_cap; g->grp = grp; g->grp_cap2 = grp_cap2;
  if (!t2_form(g, head)) { jit_stats.t2_rejected_noloop++; return false; }

  LLVMOrcThreadSafeContextRef tsc = LLVMOrcCreateNewThreadSafeContext();
  g->C = LLVMOrcThreadSafeContextGetContext(tsc);
  g->M = LLVMModuleCreateWithNameInContext("t2", g->C);
  LLVMSetTarget(g->M, LLVMOrcLLJITGetTripleString(t2_jit));
  LLVMSetDataLayout(g->M, LLVMOrcLLJITGetDataLayoutStr(t2_jit));
  T2Region *r = calloc(1, sizeof(*r));
  size_t nrecs = 2 * (size_t)g->nb;
  for (int i = 0; i < g->nb; i++) nrecs += g->b[i].last->idx_in_bb;
  r->recs = g->recs = calloc(nrecs, sizeof(JitExitRec));
  // Statistics outlive the region, for the report.
  r->stats = g->stats = calloc(1, sizeof(T2Stats));
  g->stats->pc = head->pc;
  g->stats->nb = g->nb;
  for (int i = 0; i < g->nb; i++) {
    g->stats->instrs += g->b[i].last->idx_in_bb;
    g->stats->block_pc[i] = g->b[i].head->pc;
  }
  if (t2_nstats == t2_cap_stats) {
    t2_cap_stats = t2_cap_stats ? 2 * t2_cap_stats : 256;
    t2_all_stats = realloc(t2_all_stats, t2_cap_stats * sizeof(*t2_all_stats));
  }
  t2_all_stats[t2_nstats++] = g->stats;
  t2_build(g, "t2_region");
  for (int r2 = 0; r2 < 32; r2++) {
    g->stats->loaded += (g->gpr[r2] != NULL) + (g->fpr[r2] != NULL);
    g->stats->published += g->gpr_w[r2] + g->fpr_w[r2];
  }
  Assert((size_t)g->nrecs <= nrecs, "JIT tier 2: exit records overflow");

  r->bind = g->bind;
  g->bind = NULL;
  r->head = head;
  r->old = head->jit_code;
  r->gen = t2_gen;
  // The region's blocks start no other region while it is compiled.
  for (int i = 0; i < g->nb; i++) {
    r->mtr[r->nmembers] = g->b[i].tr;
    r->members[r->nmembers++] = g->b[i].tr->id;
    t2_member[g->b[i].tr->id] = t2_gen;
  }

  // Code with the same IR is reused; the function is named t2_region (and
  // t2_region_end) while its bitcode is the key.
  LLVMMemoryBufferRef bc = LLVMWriteBitcodeToMemoryBuffer(g->M);
  const char *key = LLVMGetBufferStart(bc);
  size_t klen = LLVMGetBufferSize(bc);
  uint64_t hash = t2_hash(key, klen);
  T2Code *c = t2_code_find(key, klen, hash);
  if (c) {
    jit_stats.t2_reused++;
    LLVMDisposeModule(g->M);
    t2_install(r, c);
  } else if (jit_opt.t2_async && t2_request(r, head, key, klen, hash)) {
    LLVMDisposeModule(g->M);
  } else {
    uint64_t start = t2_now_ns();
    LLVMOrcExecutorAddress end;
    LLVMOrcExecutorAddress fn = t2_compile_here(g, tsc, head, &end);   // takes the module
    jit_stats.t2_compile_ns += t2_now_ns() - start;   // also counted as leader time
    t2_install(r, t2_code_add(t2_dup(key, klen), klen, hash, fn, end));
  }
  LLVMDisposeMemoryBuffer(bc);
  LLVMOrcDisposeThreadSafeContext(tsc);
  return true;
}

// The gate takes the head from its tier-1 translation, if the translations
// the region was built from are still the current ones.
static void t2_install(T2Region *r, T2Code *code) {
  LLVMOrcExecutorAddress fn = code->fn;
  r->code = code;
  Decode *head = r->head;
  if (r->gen != t2_gen || head->jit_state != JIT_ACTIVE || head->jit_code != r->old || r->old->superseded) {
    t2_region_drop(r);
    return;
  }
  JitTrans *gate = calloc(1, sizeof(*gate));
  gate->id = r->old->id;
  if (!jit_translate_gate(head, gate, (const void *)fn, r->bind)) {
    free(gate);
    t2_region_drop(r);
    jit_stats.t2_rejected_fail++;
    return;
  }
  jit_install_gate(r->old, gate);
  r->gate = gate;
  r->next = t2_regions;
  t2_regions = r;
  // The tier-1 cost of the region's blocks until now, for the comparison.
  for (int i = 0; i < r->nmembers; i++) {
    uint32_t id = r->members[i];
    if (r->mtr[i] && !r->mtr[i]->gate) r->pre_time += r->mtr[i]->tsamples;
    if (t2_samples[id].gen == t2_gen) r->pre_instrs += t2_samples[id].n;
  }
  gate->t2 = r;
  t2_profile_note((uintptr_t)gate->code, (uintptr_t)gate->code + gate->code_size, T2P_TRANS, gate);
  r->code->current = r;
  jit_stats.t2_regions++;
  jit_stats.t2_blocks += r->stats->nb;
  jit_stats.t2_instrs += r->stats->instrs;
}

// ---------------------------------------------------------------------------
// Profile (t2-profile): where the leader spends its time, from hardware
// samples of its instruction pointer (perf events, no signals), attributed to
// tier-1 translations, gates and region code.

typedef struct { uintptr_t lo, hi; int kind; void *owner; } T2Range;
static T2Range *t2_ranges;
static int t2_nranges, t2_sorted, t2_cap_ranges;
static struct perf_event_mmap_page *t2_prof_page;
static size_t t2_prof_size;
static uint64_t t2_icount0;
#define T2_PROF_PAGES 64

static void t2_profile_note(uintptr_t lo, uintptr_t hi, int kind, void *owner) {
  if (!jit_opt.t2_profile) return;
  if (t2_nranges == t2_cap_ranges) {
    t2_cap_ranges = t2_cap_ranges ? 2 * t2_cap_ranges : 4096;
    t2_ranges = realloc(t2_ranges, t2_cap_ranges * sizeof(T2Range));
  }
  t2_ranges[t2_nranges++] = (T2Range){ lo, hi, kind, owner };
}

void jit_t2_note_trans(JitTrans *tr) {
  t2_profile_note((uintptr_t)tr->code, (uintptr_t)tr->code + tr->code_size, T2P_TRANS, tr);
}

static int t2_range_cmp(const void *a, const void *b) {
  uintptr_t x = ((const T2Range *)a)->lo, y = ((const T2Range *)b)->lo;
  return x < y ? -1 : x > y;
}

static T2Range *t2_range_of(uintptr_t ip) {
  if (t2_nranges - t2_sorted > 256) {
    qsort(t2_ranges, t2_nranges, sizeof(T2Range), t2_range_cmp);
    t2_sorted = t2_nranges;
  }
  int lo = 0, hi = t2_sorted - 1;
  while (lo <= hi) {
    int mid = (lo + hi) / 2;
    if (ip < t2_ranges[mid].lo) hi = mid - 1;
    else if (ip >= t2_ranges[mid].hi) lo = mid + 1;
    else return &t2_ranges[mid];
  }
  for (int i = t2_sorted; i < t2_nranges; i++) if (ip >= t2_ranges[i].lo && ip < t2_ranges[i].hi) return &t2_ranges[i];
  return NULL;
}

static void t2_profile_open(void) {
  struct perf_event_attr a = { .type = PERF_TYPE_HARDWARE, .size = sizeof(a), .config = PERF_COUNT_HW_CPU_CYCLES,
                               .sample_period = 1000000, .sample_type = PERF_SAMPLE_IP,
                               .exclude_kernel = 1, .exclude_hv = 1 };
  int fd = syscall(SYS_perf_event_open, &a, 0, -1, -1, PERF_FLAG_FD_CLOEXEC);
  if (fd < 0) { Log("JIT tier 2: no profile (%s)", strerror(errno)); jit_opt.t2_profile = false; return; }
  size_t size = (1 + T2_PROF_PAGES) * 4096;
  void *m = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (m == MAP_FAILED) { close(fd); jit_opt.t2_profile = false; return; }
  t2_prof_fd = fd;
  t2_prof_page = m;
  t2_prof_size = size;
}

static void t2_profile_close(void) {
  if (t2_prof_fd < 0) return;
  munmap(t2_prof_page, t2_prof_size);
  close(t2_prof_fd);
  t2_prof_fd = -1;
}

static void t2_profile_drain(void) {
  if (t2_prof_fd < 0) return;
  uint64_t head = __atomic_load_n(&t2_prof_page->data_head, __ATOMIC_ACQUIRE);
  uint64_t tail = t2_prof_page->data_tail;
  const char *data = (const char *)t2_prof_page + 4096;
  uint64_t size = T2_PROF_PAGES * 4096;
  while (tail < head) {
    struct { struct perf_event_header h; uint64_t ip; } rec;
    for (size_t k = 0; k < sizeof(rec); k++) ((char *)&rec)[k] = data[(tail + k) % size];
    if (rec.h.size == 0) break;
    if (rec.h.type == PERF_RECORD_SAMPLE) {
      jit_stats.t2_psamples++;
      T2Range *rg = t2_range_of(rec.ip);
      if (rg && rg->kind == T2P_TRANS) {
        JitTrans *tr = rg->owner;
        if (tr->gate && tr->t2) ((T2Region *)tr->t2)->time++;
        else tr->tsamples++;
      } else if (rg && ((T2Code *)rg->owner)->current) {
        ((T2Code *)rg->owner)->current->time++;
      }
    }
    tail += rec.h.size;
  }
  __atomic_store_n(&t2_prof_page->data_tail, tail, __ATOMIC_RELEASE);
}

// Regions that run too few instructions per entry cost more at their entry
// and exit than they save: the tier-1 translation takes the head back, and
// none of the region's blocks is tried again in this lifetime.
// The hotness a block needs to be compiled: higher after regions that did
// not pay off, lower again after regions that did, so that programs where
// the second tier loses spend little on compiling.
static double t2_threshold;
#define T2_BACKOFF 1.25

static void t2_remove(T2Region *r) {
  t2_threshold = MIN_OF(t2_threshold * T2_BACKOFF, 1e6);
  jit_uninstall_gate(r->old, r->gate);
  r->removed = true;
  if (r->code && r->code->current == r) r->code->current = NULL;
  for (int i = 0; i < r->nmembers; i++) {
    t2_member[r->members[i]] = 0;
    t2_tried[r->members[i]] = t2_gen;
  }
  jit_stats.t2_removed++;
}

static void t2_prune(void) {
  t2_profile_drain();
  // Instructions per batch-end sample, which relates instruction samples to
  // instructions.
  double per_sample = (double)(get_abs_instr_count() - t2_icount0) / MAX_OF(t2_nsampled, 1);
  for (T2Region *r = t2_regions; r; r = r->next) {
    T2Stats *s = r->stats;
    if (r->removed) continue;
    if (s->entries >= 4096 && s->executed < s->entries * (uint64_t)jit_opt.t2_residency) {
      t2_remove(r);
      continue;
    }
    // Profile samples per instruction in the region, and in its blocks in
    // tier 1 before; a region that is not clearly cheaper goes.
    if (r->decided || r->time < 64) continue;
    r->decided = true;
    if (r->pre_time < 16 || r->pre_instrs < 16) continue;
    double now = (double)r->time / s->executed;
    double before = (double)r->pre_time / (r->pre_instrs * per_sample);
    if (now < 0.97 * before) {
      jit_stats.t2_kept++;
      t2_threshold = MAX_OF(t2_threshold / T2_BACKOFF, (double)jit_opt.t2_samples);
    } else {
      jit_stats.t2_lost++;
      t2_remove(r);
    }
  }
}

// A batch ran out of budget in tr: a sample of where time goes. A block
// sampled often enough gets a region for a loop through it, unless it is in
// a region already.
static void t2_profile_open(void);
static void t2_profile_drain(void);

void jit_t2_sample(JitTrans *tr) {
  if (t2_nsampled == 0) t2_icount0 = get_abs_instr_count();
  if (jit_opt.t2_profile && t2_prof_fd < 0) t2_profile_open();
  if ((t2_nsampled & 15) == 15) t2_profile_drain();
  if ((++t2_nsampled & 15) == 0 && t2_pending) {
    uint64_t start = t2_now_ns();
    t2_poll();
    jit_stats.t2_leader_ns += t2_now_ns() - start;
  }
  if ((t2_nsampled & 1023) == 0) t2_prune();
  uint32_t id = tr->id;
  // Instruction samples of every block, for selection and for the profile.
  if (t2_samples[id].gen != t2_gen) t2_samples[id].gen = t2_gen, t2_samples[id].n = 0;
  t2_samples[id].n++;
  if (tr->superseded || tr->gate || t2_member[id] == t2_gen || t2_tried[id] == t2_gen) return;
  // Hot: by time spent in its translation when there is a profile, else by
  // instructions executed.
  if (t2_threshold < jit_opt.t2_samples) t2_threshold = jit_opt.t2_samples;
  if (jit_opt.t2_profile ? tr->tsamples < t2_threshold || t2_samples[id].n < 16
                         : t2_samples[id].n < t2_threshold) return;
  t2_tried[id] = t2_gen;
  Decode *head = tr->head;
  if (head->jit_state != JIT_ACTIVE || head->jit_code != tr) return;
  if (MUXDEF(CONFIG_RVH, cpu.v, false)) return;   // FS-dirty marking differs with V=1
  // LLVM may leave host floating-point flags that translated code accrues.
  uint32_t mxcsr = _mm_getcsr();
  uint64_t start = t2_now_ns();
  t2_compile(head);
  jit_stats.t2_leader_ns += t2_now_ns() - start;
  _mm_setcsr(mxcsr);
}

// Regions die with the translations they are built from; their code stays
// for the next region with the same IR.
void jit_t2_discard(void) {
  while (t2_regions != NULL) {
    T2Region *r = t2_regions;
    t2_regions = r->next;
    free(r->bind);   // stats stay for the report
    free(r->recs);
    free(r);
  }
  // Regions still being compiled are not installed; their code is kept.
  for (T2Pending *p = t2_pending; p; p = p->next) {
    if (p->r == NULL) continue;
    free(p->r->bind);
    free(p->r->recs);
    free(p->r);
    p->r = NULL;
  }
  t2_gen++;
  // Translations are gone; region code stays, without a region running it.
  int n = 0;
  for (int i = 0; i < t2_nranges; i++) if (t2_ranges[i].kind == T2P_UNIT) t2_ranges[n++] = t2_ranges[i];
  t2_nranges = n;
  qsort(t2_ranges, t2_nranges, sizeof(T2Range), t2_range_cmp);
  t2_sorted = t2_nranges;
  for (T2Code *c = t2_codes; c; c = c->next) c->current = NULL;
}

void jit_t2_report(void) {
  uint64_t entries = 0, executed = 0;
  for (int i = 0; i < t2_nstats; i++) { entries += t2_all_stats[i]->entries; executed += t2_all_stats[i]->executed; }
  Log("JIT tier 2: hotness threshold %.0f samples at the end", t2_threshold);
  Log("JIT tier 2: %lu guest instructions in regions (%.1f%% of %lu), %.0f per entry",
      executed, 100.0 * executed / MAX_OF(get_abs_instr_count(), 1), get_abs_instr_count(),
      entries ? (double)executed / entries : 0.0);
  Log("JIT tier 2: %lu regions (%lu blocks, %lu instructions), %lu compiled, %lu reused, %lu removed "
      "(profile: %lu kept, %lu lost, %lu samples); rejected: %lu not a loop, %lu failed; compile %lu ms, "
      "in the leader %lu ms",
      jit_stats.t2_regions, jit_stats.t2_blocks, jit_stats.t2_instrs, jit_stats.t2_compiled,
      jit_stats.t2_reused, jit_stats.t2_removed, jit_stats.t2_kept, jit_stats.t2_lost, jit_stats.t2_psamples,
      jit_stats.t2_rejected_noloop, jit_stats.t2_rejected_fail,
      jit_stats.t2_compile_ns / 1000000, jit_stats.t2_leader_ns / 1000000);
}

void jit_t2_init(void) {
  sljit_sw off;
  if (!jit_opt.spec_fp || !ISDEF(JIT_SLOT_CHAIN) || !cpu_offset(&jit_uts_entry[JIT_MAX_TRANS - 1], &off)) {
    // Regions are installed by redirecting entry slots, and follow the
    // speculative floating point of the leader.
    Log("JIT tier 2: off (needs spec-fp and chaining through entry slots)");
    jit_opt.tier2 = false;
    return;
  }
  _Static_assert(sizeof(cpu.gpr[0]) == 8, "guest registers are accessed as 64-bit words");
  IFNDEF(CONFIG_FPU_NONE, _Static_assert(sizeof(cpu.fpr[0]) == 8, "FP registers are accessed as 64-bit words"));
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  char *triple = LLVMGetDefaultTargetTriple();
  LLVMTargetRef target;
  char *err = NULL;
  if (LLVMGetTargetFromTriple(triple, &target, &err)) panic("JIT tier 2: %s", err);
  char *cpu_name = LLVMGetHostCPUName(), *features = LLVMGetHostCPUFeatures();
  static const LLVMCodeGenOptLevel levels[] = { LLVMCodeGenLevelNone, LLVMCodeGenLevelLess,
                                                LLVMCodeGenLevelDefault, LLVMCodeGenLevelAggressive };
  LLVMCodeGenOptLevel cg = levels[MIN_OF(MAX_OF(jit_opt.t2_cg, 0), 3)];
  t2_tm = LLVMCreateTargetMachine(target, triple, cpu_name, features, cg, LLVMRelocDefault, LLVMCodeModelJITDefault);
  // Code generation of the JIT uses a machine configured the same way.
  LLVMTargetMachineRef jit_tm = LLVMCreateTargetMachine(target, triple, cpu_name, features, cg,
                                                        LLVMRelocDefault, LLVMCodeModelJITDefault);
  LLVMOrcLLJITBuilderRef b = LLVMOrcCreateLLJITBuilder();
  LLVMOrcLLJITBuilderSetJITTargetMachineBuilder(b, LLVMOrcJITTargetMachineBuilderCreateFromTargetMachine(jit_tm));
  t2_check(LLVMOrcCreateLLJIT(&t2_jit, b), "creating the JIT");
  LLVMDisposeMessage(triple);
  unsigned major, minor, patch;
  LLVMGetVersion(&major, &minor, &patch);
  Log("JIT tier 2: LLVM %u.%u.%u for %s, %d samples, residency %d, up to %d blocks and %d instructions, O%d, "
      "code generation %d", major, minor, patch, cpu_name, jit_opt.t2_samples, jit_opt.t2_residency,
      jit_opt.t2_blocks, jit_opt.t2_instrs, jit_opt.t2_opt, jit_opt.t2_cg);
  LLVMDisposeMessage(cpu_name);
  LLVMDisposeMessage(features);
}

#endif
