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

// Translation of one tcache basic block (or a prefix of it) into host code.
//
// Every translated block runs inside the frame created by the trampoline:
//   S0 = &cpu, S1 = batch budget (32-bit), S2 = host read-TLB base.
// Guest registers live in cpu.gpr. A block leaves through an exit stub that
// points jit_exit_ptr at a static exit record and returns the remaining
// budget through the shared epilogue in the trampoline.

#include <isa-all-instr.h>
#include <memory/vaddr.h>
#include "jit-internal.h"
#include "sljitLir.h"
#include <unistd.h>

#define JIT_SCRATCHES (7 | SLJIT_ENTER_FLOAT(3))
#define JIT_SAVEDS    6
#define JIT_LOCALS    64
#define JIT_ARGS      SLJIT_ARGS3(W, P_R, P, W)

#define REG_CPU    SLJIT_S0
#define REG_BUDGET SLJIT_S1
#define REG_RTLB   SLJIT_S2

const JitExitRec *jit_exit_ptr;
sljit_uw jit_exit_target;
static sljit_uw common_exit;         // shared epilogue in the trampoline
static JitExitRec fallback_exit = { .kind = JIT_REC_FALLBACK };
static const JitExitRec *gate_rec;   // the record of a region exit during a gate's lookup

#ifdef CONFIG_RVB
// Bitmanip intrinsics shared with the interpreter's instruction helpers.
long _rv_clz(long); long _rv_ctz(long); long _rv_cpop(long);
long _rv_rev8(long); long _rv_orc_b(long); long _rv_revb(long);
int32_t _rv32_clz(int32_t); int32_t _rv32_ctz(int32_t); int32_t _rv32_cpop(int32_t);
long _rv_rol(long, long); long _rv_ror(long, long);
int32_t _rv32_rol(int32_t, int32_t); int32_t _rv32_ror(int32_t, int32_t);
long _rv_bclr(long, long); long _rv_bset(long, long); long _rv_binv(long, long); long _rv_bext(long, long);
long _rv_clmul(long, long); long _rv_clmulh(long, long); long _rv_clmulr(long, long);
long _rv_pack(long, long); long _rv_packh(long, long); int32_t _rv32_pack(int32_t, int32_t);
#endif

static sljit_sw gpr_off(int r) {
  return (sljit_sw)offsetof(CPU_state, gpr) + r * (sljit_sw)sizeof(cpu.gpr[0]);
}

// Architectural register number of an operand pointer, or -1 when the
// pointer does not refer to cpu.gpr (a discarded write to x0).
static int reg_of(const rtlreg_t *p) {
  uintptr_t base = (uintptr_t)&cpu.gpr[0];
  uintptr_t a = (uintptr_t)p;
  if (a < base || a >= base + sizeof(cpu.gpr)) return -1;
  if ((a - base) % sizeof(cpu.gpr[0]) != 0) return -1;
  return (a - base) / sizeof(cpu.gpr[0]);
}

typedef struct {
  struct sljit_compiler *c;
  Decode *head;
  Decode *last;            // terminator of the block
  bool has_mem;
  bool fs_dirty;           // the block has already marked FP state dirty
  struct sljit_jump *uts_miss;        // first entry in an epoch of the reference cache
  struct sljit_label *uts_body;       // the block after its epoch check
  struct sljit_label *uts_fallback;   // leaves for the interpreter at the head
  struct sljit_jump *fp_fallback;     // to the same exit, from the floating-point guard
  struct sljit_label *chain_after[2];  // follows a jump through a slot (JIT_SLOT_CHAIN)
  struct sljit_label *chain_exit[2];   // the exit path of that jump
  // Guest registers holding a value known at translation time, from li,
  // lui/auipc and additions of immediates. Reset for every translation.
  bool known[32];
  sljit_sw kval[32];
  int fp_guard;            // fcsr mask checked at entry (0: no check)
  JitTrans *tr;
  // Patchable jumps to the taken and not-taken successors.
  struct sljit_jump *chain[2];
  // Out-of-line slow paths are emitted after the block body.
  int nslow;
  struct {
    struct sljit_jump *from[2];
    struct sljit_label *resume;
    Decode *d;
    int width;
    bool is_store;
    bool sign;
    int rf;          // register file of the data operand
  } slow[32768];
  // Out-of-line fallbacks of inline floating-point operations.
  int nfslow;
  struct {
    struct sljit_jump *from[16];
    int nfrom;
    struct sljit_label *resume;
    Decode *d;
    bool zero_path, fma, w64, na, nc;
    int fd;
  } fslow[32768];
} Tc;

// Guest registers kept in host registers for the whole run. Every
// translation uses the same mapping, so chained and indirect transfers keep
// them live; they are written back to cpu.gpr before any helper call and at
// every exit, and reloaded afterwards.
static const sljit_s32 pin_hosts[] = {
  SLJIT_S3, SLJIT_S4, SLJIT_S5, SLJIT_R3, SLJIT_R4, SLJIT_R5, SLJIT_R6,
};
enum { PIN_SAVED = 3 };   // pin_hosts starts with this many callee-saved registers
static const int pin_order[] = { 2, 15, 14, 10, 8, 1, 11, 12, 13, 9 };
static sljit_s32 pin_reg[32];
static int pin_count;

void jit_pin_init(int n) {
  memset(pin_reg, 0, sizeof(pin_reg));
  pin_count = MIN_OF(n, (int)ARRLEN(pin_hosts));
  for (int i = 0; i < pin_count; i++) pin_reg[pin_order[i]] = pin_hosts[i];
}

// Pinned registers from pin_hosts[first] on, to and from cpu.gpr.
static void emit_writeback_pinned_from(struct sljit_compiler *c, int first) {
  for (int i = first; i < pin_count; i++) {
    sljit_emit_op1(c, SLJIT_MOV, SLJIT_MEM1(REG_CPU), gpr_off(pin_order[i]), pin_hosts[i], 0);
  }
}

static void emit_reload_pinned_from(struct sljit_compiler *c, int first) {
  for (int i = first; i < pin_count; i++) {
    sljit_emit_op1(c, SLJIT_MOV, pin_hosts[i], 0, SLJIT_MEM1(REG_CPU), gpr_off(pin_order[i]));
  }
}

static void emit_writeback_pinned(struct sljit_compiler *c) { emit_writeback_pinned_from(c, 0); }
static void emit_reload_pinned(struct sljit_compiler *c) { emit_reload_pinned_from(c, 0); }

// Source operand: x0 reads as the immediate 0, pinned registers from their
// host register, the others from cpu.gpr.
static void src_op(int r, sljit_s32 *op, sljit_sw *opw) {
  if (r == 0) { *op = SLJIT_IMM; *opw = 0; }
  else if (pin_reg[r]) { *op = pin_reg[r]; *opw = 0; }
  else { *op = SLJIT_MEM1(REG_CPU); *opw = gpr_off(r); }
}

static void emit_store_reg(Tc *t, int rd, sljit_s32 src) {
  if (rd <= 0) return;
  t->known[rd] = false;
  if (pin_reg[rd]) {
    if (pin_reg[rd] != src) sljit_emit_op1(t->c, SLJIT_MOV, pin_reg[rd], 0, src, 0);
    return;
  }
  sljit_emit_op1(t->c, SLJIT_MOV, SLJIT_MEM1(REG_CPU), gpr_off(rd), src, 0);
}

static void emit_li(Tc *t, int rd, sljit_sw imm) {
  if (rd <= 0) return;
  t->known[rd] = true;
  t->kval[rd] = imm;
  if (pin_reg[rd]) { sljit_emit_op1(t->c, SLJIT_MOV, pin_reg[rd], 0, SLJIT_IMM, imm); return; }
  sljit_emit_op1(t->c, SLJIT_MOV, SLJIT_MEM1(REG_CPU), gpr_off(rd), SLJIT_IMM, imm);
}

// Calls clobber the scratch registers and helpers read guest state from cpu.
// A call that neither reads nor writes guest registers: only the pinned
// registers the callee may clobber are kept in cpu.gpr around it.
static void emit_icall_light(struct sljit_compiler *c, sljit_s32 arg_types, void *fn) {
  emit_writeback_pinned_from(c, PIN_SAVED);
  sljit_emit_icall(c, SLJIT_CALL, arg_types, SLJIT_IMM, SLJIT_FUNC_ADDR(fn));
  emit_reload_pinned_from(c, PIN_SAVED);
}

// Memory operand for a static variable, relative to REG_CPU when possible.
static void static_mem(const void *p, sljit_s32 *m, sljit_sw *mw) {
  if (cpu_offset(p, mw)) *m = SLJIT_MEM1(REG_CPU);
  else { *m = SLJIT_MEM0(); *mw = (sljit_sw)p; }
}

static void emit_icall(struct sljit_compiler *c, sljit_s32 arg_types, void *fn) {
  emit_writeback_pinned(c);
  sljit_emit_icall(c, SLJIT_CALL, arg_types, SLJIT_IMM, SLJIT_FUNC_ADDR(fn));
  emit_reload_pinned(c);
}

// rd = rs1 op rs2, with an optional 32-bit form sign-extended to 64 bits.
static void emit_rr(Tc *t, sljit_s32 op, bool word, int rd, int rs1, int rs2) {
  sljit_s32 a, b; sljit_sw aw, bw;
  src_op(rs1, &a, &aw);
  src_op(rs2, &b, &bw);
  sljit_emit_op2(t->c, op | (word ? SLJIT_32 : 0), SLJIT_R0, 0, a, aw, b, bw);
  if (word) sljit_emit_op1(t->c, SLJIT_MOV_S32, SLJIT_R0, 0, SLJIT_R0, 0);
  emit_store_reg(t, rd, SLJIT_R0);
}

static void emit_ri(Tc *t, sljit_s32 op, bool word, int rd, int rs1, sljit_sw imm) {
  if (op == SLJIT_ADD && rs1 > 0 && t->known[rs1]) {
    sljit_sw v = t->kval[rs1] + imm;
    if (word) v = (int32_t)v;
    emit_li(t, rd, v);
    return;
  }
  sljit_s32 a; sljit_sw aw;
  src_op(rs1, &a, &aw);
  sljit_emit_op2(t->c, op | (word ? SLJIT_32 : 0), SLJIT_R0, 0, a, aw, SLJIT_IMM, imm);
  if (word) sljit_emit_op1(t->c, SLJIT_MOV_S32, SLJIT_R0, 0, SLJIT_R0, 0);
  emit_store_reg(t, rd, SLJIT_R0);
}

// rd = (rs1 < rs2) with a signed or unsigned comparison.
static void emit_slt(Tc *t, bool sign, int rd, sljit_s32 b, sljit_sw bw, int rs1) {
  sljit_s32 a; sljit_sw aw;
  src_op(rs1, &a, &aw);
  sljit_emit_op2u(t->c, SLJIT_SUB | (sign ? SLJIT_SET_SIG_LESS : SLJIT_SET_LESS), a, aw, b, bw);
  sljit_emit_op_flags(t->c, SLJIT_MOV, SLJIT_R0, 0, sign ? SLJIT_SIG_LESS : SLJIT_LESS);
  emit_store_reg(t, rd, SLJIT_R0);
}

// rd = fn(rs1, rs2) through a C helper; used for rare or awkward operations.
static void emit_call2(Tc *t, void *fn, int rd, int rs1, int rs2) {
  sljit_s32 a, b; sljit_sw aw, bw;
  src_op(rs1, &a, &aw);
  src_op(rs2, &b, &bw);
  sljit_emit_op1(t->c, SLJIT_MOV, SLJIT_R0, 0, a, aw);
  sljit_emit_op1(t->c, SLJIT_MOV, SLJIT_R1, 0, b, bw);
  emit_icall(t->c, SLJIT_ARGS2(W, W, W), fn);
  emit_store_reg(t, rd, SLJIT_R0);
}

// rd = fn(rs1) or fn(rs1, imm) through a bitmanip intrinsic. Intrinsics on
// 32-bit values return int32_t, so their result is sign-extended.
static void emit_call_ri(Tc *t, void *fn, bool arg2_imm, sljit_sw imm, bool word, int rd, int rs1) {
  sljit_s32 a; sljit_sw aw;
  src_op(rs1, &a, &aw);
  sljit_emit_op1(t->c, SLJIT_MOV, SLJIT_R0, 0, a, aw);
  if (arg2_imm) {
    sljit_emit_op1(t->c, SLJIT_MOV, SLJIT_R1, 0, SLJIT_IMM, imm);
    emit_icall(t->c, SLJIT_ARGS2(W, W, W), fn);
  } else {
    emit_icall(t->c, SLJIT_ARGS1(W, W), fn);
  }
  if (word) sljit_emit_op1(t->c, SLJIT_MOV_S32, SLJIT_R0, 0, SLJIT_R0, 0);
  emit_store_reg(t, rd, SLJIT_R0);
}

static void emit_call_rr(Tc *t, void *fn, bool word, int rd, int rs1, int rs2) {
  emit_call2(t, fn, -1, rs1, rs2);
  if (word) sljit_emit_op1(t->c, SLJIT_MOV_S32, SLJIT_R0, 0, SLJIT_R0, 0);
  emit_store_reg(t, rd, SLJIT_R0);
}

// rd = (zext32(rs1) or rs1) << sh, plus rs2 when add is set.
static void emit_shadd(Tc *t, int sh, bool uw, bool add, int rd, int rs1, int rs2) {
  sljit_s32 a, b; sljit_sw aw, bw;
  src_op(rs1, &a, &aw);
  src_op(rs2, &b, &bw);
  sljit_emit_op1(t->c, uw ? SLJIT_MOV_U32 : SLJIT_MOV, SLJIT_R0, 0, a, aw);
  if (sh) sljit_emit_op2(t->c, SLJIT_SHL, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, sh);
  if (add) sljit_emit_op2(t->c, SLJIT_ADD, SLJIT_R0, 0, SLJIT_R0, 0, b, bw);
  emit_store_reg(t, rd, SLJIT_R0);
}

// rd = rs1 op ~rs2
static void emit_notop(Tc *t, sljit_s32 op, int rd, int rs1, int rs2) {
  sljit_s32 a, b; sljit_sw aw, bw;
  src_op(rs1, &a, &aw);
  src_op(rs2, &b, &bw);
  sljit_emit_op2(t->c, SLJIT_XOR, SLJIT_R1, 0, b, bw, SLJIT_IMM, -1);
  sljit_emit_op2(t->c, op, SLJIT_R0, 0, a, aw, SLJIT_R1, 0);
  emit_store_reg(t, rd, SLJIT_R0);
}

static void emit_ext(Tc *t, sljit_s32 mov, int rd, int rs1) {
  sljit_s32 a; sljit_sw aw;
  src_op(rs1, &a, &aw);
  sljit_emit_op1(t->c, mov, SLJIT_R0, 0, a, aw);
  emit_store_reg(t, rd, SLJIT_R0);
}

// rd = cond(rs1, rs2) ? rs2 : rs1, which covers min and max.
static void emit_minmax(Tc *t, sljit_s32 cond, int rd, int rs1, int rs2) {
  sljit_s32 a, b; sljit_sw aw, bw;
  src_op(rs1, &a, &aw);
  src_op(rs2, &b, &bw);
  sljit_emit_op1(t->c, SLJIT_MOV, SLJIT_R0, 0, a, aw);
  sljit_emit_op1(t->c, SLJIT_MOV, SLJIT_R1, 0, b, bw);
  struct sljit_jump *keep = sljit_emit_cmp(t->c, cond ^ 1, SLJIT_R0, 0, SLJIT_R1, 0);
  sljit_emit_op1(t->c, SLJIT_MOV, SLJIT_R0, 0, SLJIT_R1, 0);
  sljit_set_label(keep, sljit_emit_label(t->c));
  emit_store_reg(t, rd, SLJIT_R0);
}

// rd = (rs2 == 0) == zero_when_eqz ? 0 : rs1
static void emit_czero(Tc *t, bool eqz, int rd, int rs1, int rs2) {
  sljit_s32 a, b; sljit_sw aw, bw;
  src_op(rs1, &a, &aw);
  src_op(rs2, &b, &bw);
  sljit_emit_op1(t->c, SLJIT_MOV, SLJIT_R0, 0, a, aw);
  struct sljit_jump *keep = sljit_emit_cmp(t->c, eqz ? SLJIT_NOT_EQUAL : SLJIT_EQUAL, b, bw, SLJIT_IMM, 0);
  sljit_emit_op1(t->c, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, 0);
  sljit_set_label(keep, sljit_emit_label(t->c));
  emit_store_reg(t, rd, SLJIT_R0);
}

// A chunk boundary d of a long block: another entry of the translation, and
// a check whether the reference runs out of capacity before an instruction
// of the chunk (a stop placed in the block, see tcache_uts_enter()); then
// the translation leaves at d, and the interpreter runs up to the stop.
static void emit_chunk_entry(Tc *t, Decode *d, struct sljit_label **label) {
  struct sljit_compiler *c = t->c;
  int j = (d - t->head) / JIT_CHUNK_LEN;
  if (j > 0) {
    label[j] = sljit_emit_label(c);
    // Entered from outside: nothing is known about the registers, and the
    // floating-point state is marked again.
    memset(t->known, 0, sizeof(t->known));
    t->fs_dirty = false;
  }
  sljit_sw span = (sljit_sw)MIN_OF(JIT_CHUNK_LEN, t->last - d + 1) * (sljit_sw)sizeof(Decode);
  sljit_emit_op1(c, SLJIT_MOV, SLJIT_R0, 0, SLJIT_MEM0(), (sljit_sw)&tcache_uts_stop_at);
  sljit_emit_op2(c, SLJIT_SUB, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, (sljit_sw)d);
  struct sljit_jump *go = sljit_emit_cmp(c, SLJIT_GREATER_EQUAL, SLJIT_R0, 0, SLJIT_IMM, span);
  JitExitRec *r = &t->tr->xexits[j];
  *r = (JitExitRec){ .kind = JIT_REC_MIDBLOCK, .next = d, .trans = t->tr };
  sljit_emit_op1(c, SLJIT_MOV_P, SLJIT_R0, 0, SLJIT_IMM, (sljit_sw)r);
  sljit_emit_ijump(c, SLJIT_JUMP, SLJIT_IMM, common_exit);
  sljit_set_label(go, sljit_emit_label(c));
}

void jit_free_chunks(JitTrans *tr) {
  free(tr->chunk);
  free(tr->xexits);
  tr->chunk = NULL;
  tr->xexits = NULL;
  tr->nchunks = 0;
}

// Leave translated code. The budget is returned; the record tells the glue
// how to continue.
static void emit_exit(Tc *t, sljit_sw kind, Decode *last, Decode *next) {
  struct sljit_compiler *c = t->c;
  Assert(t->tr->nexits < JIT_MAX_EXITS, "JIT: too many exits in one translation");
  JitExitRec *r = &t->tr->exits[t->tr->nexits++];
  r->kind = kind;
  r->last = last;
  r->next = next;
  r->trans = t->tr;
  sljit_emit_op1(c, SLJIT_MOV_P, SLJIT_R0, 0, SLJIT_IMM, (sljit_sw)r);
  sljit_emit_ijump(c, SLJIT_JUMP, SLJIT_IMM, common_exit);
}

static void emit_sync_state(Tc *t, Decode *d);

// Leave through a direct successor. While the budget is positive, control
// may continue in the successor's translation once the glue has patched the
// jump; the interpreter checks the budget at the same point.
static void emit_successor_exit(Tc *t, int which) {
  struct sljit_compiler *c = t->c;
  struct sljit_jump *stop = sljit_emit_cmp(c, SLJIT_SIG_LESS_EQUAL | SLJIT_32,
      REG_BUDGET, 0, SLJIT_IMM, 0);
#ifdef JIT_SLOT_CHAIN
  sljit_sw disp;
  if (cpu_offset(&jit_edge_slot[2 * t->tr->id + which], &disp) &&
      cpu_offset(&jit_uts_entry[JIT_MAX_TRANS - 1], &(sljit_sw){0})) {
    // jmp qword [REG_CPU + disp32]; chaining patches disp32 (see try_chain()).
    int base = sljit_get_register_index(SLJIT_GP_REGISTER, REG_CPU);
    uint8_t insn[8];
    int n = 0;
    if (base >= 8) insn[n++] = 0x41;
    insn[n++] = 0xff;
    insn[n++] = 0xa0 | (base & 7);
    if ((base & 7) == 4) insn[n++] = 0x24;
    memcpy(insn + n, &(int32_t){ (int32_t)disp }, 4);
    sljit_emit_op_custom(c, insn, n + 4);
    t->chain_after[which] = sljit_emit_label(c);
    sljit_set_label(stop, t->chain_exit[which] = sljit_emit_label(c));
    emit_exit(t, which == 0 ? JIT_REC_TAKEN : JIT_REC_NTAKEN, t->last, NULL);
    return;
  }
#endif
  t->chain[which] = sljit_emit_jump(c, SLJIT_JUMP | SLJIT_REWRITABLE_JUMP);
  struct sljit_label *exit = sljit_emit_label(c);
  sljit_set_label(t->chain[which], exit);
  sljit_set_label(stop, exit);
  emit_exit(t, which == 0 ? JIT_REC_TAKEN : JIT_REC_NTAKEN, t->last, NULL);
}

static void emit_block_end_budget(Tc *t) {
  sljit_emit_op2(t->c, SLJIT_SUB32, REG_BUDGET, 0, REG_BUDGET, 0, SLJIT_IMM, t->last->idx_in_bb);
}

// Conditional branch: compare, then leave through the taken or not-taken
// exit. The successor is read from the terminator's links at exit time.
static void emit_branch(Tc *t, sljit_s32 cond, int rs1, int rs2) {
  sljit_s32 a, b; sljit_sw aw, bw;
  src_op(rs1, &a, &aw);
  src_op(rs2, &b, &bw);
  emit_block_end_budget(t);
  struct sljit_jump *taken = sljit_emit_cmp(t->c, cond, a, aw, b, bw);
  emit_successor_exit(t, 1);
  sljit_set_label(taken, sljit_emit_label(t->c));
  emit_successor_exit(t, 0);
}

static void emit_jump(Tc *t) {
  emit_block_end_budget(t);
  emit_successor_exit(t, 0);
}

// Jumps to the translated entry cached for the guest target in R0; falls
// through the returned jumps on a miss or an entry of another lookup context.
static void emit_ibcache_lookup(struct sljit_compiler *c, struct sljit_jump **miss, struct sljit_jump **stale) {
  // R1 = &jit_ibcache[jit_ib_index(target)]
  sljit_emit_op2(c, SLJIT_LSHR, SLJIT_R1, 0, SLJIT_R0, 0, SLJIT_IMM, 1);
  sljit_emit_op2(c, SLJIT_LSHR, SLJIT_R2, 0, SLJIT_R0, 0, SLJIT_IMM, jit_opt.ib_bits + 1);
  sljit_emit_op2(c, SLJIT_XOR, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_R2, 0);
  sljit_emit_op2(c, SLJIT_XOR, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_MEM0(), (sljit_sw)&jit_ctx_mix);
  sljit_emit_op2(c, SLJIT_AND, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, (1l << jit_opt.ib_bits) - 1);
  sljit_emit_op2(c, SLJIT_SHL, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, JIT_IBCACHE_SHIFT);
  sljit_emit_op2(c, SLJIT_ADD, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, (sljit_sw)jit_ibcache);
  *miss = sljit_emit_cmp(c, SLJIT_NOT_EQUAL, SLJIT_MEM1(SLJIT_R1), offsetof(JitIbEntry, pc), SLJIT_R0, 0);
  sljit_emit_op1(c, SLJIT_MOV, SLJIT_R2, 0, SLJIT_MEM0(), (sljit_sw)&jit_ctx_gen);
  *stale = sljit_emit_cmp(c, SLJIT_NOT_EQUAL, SLJIT_MEM1(SLJIT_R1), offsetof(JitIbEntry, gen), SLJIT_R2, 0);
  sljit_emit_ijump(c, SLJIT_JUMP, SLJIT_MEM1(SLJIT_R1), offsetof(JitIbEntry, entry));
}

// Indirect jump to the value in R0. The interpreter's indirect-jump fetch
// resolves the successor at exit time.
static void emit_indirect(Tc *t) {
  struct sljit_compiler *c = t->c;
  emit_block_end_budget(t);
  if (jit_opt.ibcache) {
    struct sljit_jump *stop = sljit_emit_cmp(c, SLJIT_SIG_LESS_EQUAL | SLJIT_32, REG_BUDGET, 0, SLJIT_IMM, 0);
    struct sljit_jump *miss, *stale;
    emit_ibcache_lookup(c, &miss, &stale);
    struct sljit_label *exit = sljit_emit_label(c);
    sljit_set_label(miss, exit);
    sljit_set_label(stale, exit);
    sljit_set_label(stop, exit);
  }
  sljit_emit_op1(c, SLJIT_MOV, SLJIT_MEM0(), (sljit_sw)&jit_exit_target, SLJIT_R0, 0);
  emit_exit(t, JIT_REC_INDIRECT, t->last, NULL);
}

// Before a helper that may read the instruction count or raise an
// exception, publish the interpreter-visible state exactly as the
// interpreter would have it while executing d.
static void emit_sync_state(Tc *t, Decode *d) {
  struct sljit_compiler *c = t->c;
  sljit_emit_op1(c, SLJIT_MOV32, SLJIT_MEM0(), (sljit_sw)jit_budget_slot(), REG_BUDGET, 0);
  sljit_emit_op1(c, SLJIT_MOV_P, SLJIT_MEM0(), (sljit_sw)jit_prev_s_slot(), SLJIT_IMM, (sljit_sw)d);
}

enum { RF_GPR, RF_FPR64, RF_FPR32 };

// Floating-point register number of an operand pointer, or -1.
static int freg_of(const rtlreg_t *p) {
#ifndef CONFIG_FPU_NONE
  uintptr_t base = (uintptr_t)&cpu.fpr[0];
  uintptr_t a = (uintptr_t)p;
  if (a < base || a >= base + sizeof(cpu.fpr)) return -1;
  if ((a - base) % sizeof(cpu.fpr[0]) != 0) return -1;
  return (a - base) / sizeof(cpu.fpr[0]);
#else
  return -1;
#endif
}

static sljit_sw fpr_off(int r) {
  return (sljit_sw)offsetof(CPU_state, fpr) + r * (sljit_sw)sizeof(cpu.fpr[0]);
}

// Floating-point state becomes dirty whenever an FP register is written.
// Blocks are translated per virtualization mode; with V=1 the helper also
// updates vsstatus.
// Emitted once per translation: control reaches later floating-point
// writes of the block only through the first one, whose fast and slow paths
// both leave the state dirty.
static void emit_fs_dirty(Tc *t) {
  if (t->fs_dirty) return;
  t->fs_dirty = true;
  if (MUXDEF(CONFIG_RVH, cpu.v, false)) {
    void fp_set_dirty(void);
    emit_icall(t->c, SLJIT_ARGS0V(), (void *)fp_set_dirty);
    return;
  }
  sljit_sw mstatus_addr = (sljit_sw)&csr_array[0x300];
  sljit_emit_op2(t->c, SLJIT_OR, SLJIT_MEM0(), mstatus_addr, SLJIT_MEM0(), mstatus_addr,
                 SLJIT_IMM, (sljit_sw)3 << 13);
}

// Load or store with the inline host-TLB fast path. R0 holds the address.
static bool emit_mem(Tc *t, Decode *d, int width, bool is_store, bool sign, int rf) {
  struct sljit_compiler *c = t->c;
  int base = reg_of(d->src1.preg);
  int rd = rf == RF_GPR ? reg_of(d->dest.preg) : freg_of(d->dest.preg);
  if (base < 0 || (is_store && rd < 0) || (rf != RF_GPR && rd < 0)) return false;
  if (t->nslow == ARRLEN(t->slow)) return false;
  t->has_mem = true;

  int k = t->nslow++;
  t->slow[k].d = d;
  t->slow[k].width = width;
  t->slow[k].is_store = is_store;
  t->slow[k].sign = sign;
  t->slow[k].rf = rf;
  t->slow[k].from[0] = t->slow[k].from[1] = NULL;

  bool const_addr = base == 0 || t->known[base];
  sljit_sw addr = 0;
  if (const_addr) {
    addr = (base == 0 ? 0 : t->kval[base]) + (sljit_sw)d->src2.imm;
    sljit_emit_op1(c, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, addr);
  } else {
    sljit_s32 a; sljit_sw aw;
    src_op(base, &a, &aw);
    if ((sljit_sw)d->src2.imm == 0) sljit_emit_op1(c, SLJIT_MOV, SLJIT_R0, 0, a, aw);
    else sljit_emit_op2(c, SLJIT_ADD, SLJIT_R0, 0, a, aw, SLJIT_IMM, (sljit_sw)d->src2.imm);
  }

  // The entry tag is the page-aligned address with the low alignment bits
  // kept, so one compare checks both the page and the alignment (aligned
  // accesses cannot cross a page).
  sljit_uw tag_mask = ~(sljit_uw)PAGE_MASK | (sljit_uw)(width - 1);
  sljit_sw slot_mask = (jit_hosttlb_size() - 1) << 4;
  sljit_sw table = is_store ? jit_hosttlb_write_delta() : 0;
  if (const_addr) {
    // The entry of a constant address is known: R2 = its host offset.
    sljit_sw slot = table + (((sljit_uw)addr >> (PAGE_SHIFT - 4)) & slot_mask);
    t->slow[k].from[1] = sljit_emit_cmp(c, SLJIT_NOT_EQUAL,
        SLJIT_MEM1(REG_RTLB), slot + jit_hosttlb_gvpn_offset(), SLJIT_IMM, (sljit_sw)(addr & tag_mask));
    sljit_emit_op1(c, SLJIT_MOV, SLJIT_R2, 0, SLJIT_MEM1(REG_RTLB), slot + jit_hosttlb_offset_offset());
  } else {
    // R1 = tag of the address, R2 = &tlb[vpn % size].
    sljit_emit_op2(c, SLJIT_AND, SLJIT_R1, 0, SLJIT_R0, 0, SLJIT_IMM, (sljit_sw)tag_mask);
    sljit_emit_op2(c, SLJIT_LSHR, SLJIT_R2, 0, SLJIT_R0, 0, SLJIT_IMM, PAGE_SHIFT - 4);
    sljit_emit_op2(c, SLJIT_AND, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_IMM, slot_mask);
    if (table) sljit_emit_op2(c, SLJIT_ADD, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_IMM, table);
    sljit_emit_op2(c, SLJIT_ADD, SLJIT_R2, 0, SLJIT_R2, 0, REG_RTLB, 0);
    t->slow[k].from[1] = sljit_emit_cmp(c, SLJIT_NOT_EQUAL,
        SLJIT_MEM1(SLJIT_R2), jit_hosttlb_gvpn_offset(), SLJIT_R1, 0);
    sljit_emit_op1(c, SLJIT_MOV, SLJIT_R2, 0, SLJIT_MEM1(SLJIT_R2), jit_hosttlb_offset_offset());
  }

  static const sljit_s32 load_u[] = { [1] = SLJIT_MOV_U8, [2] = SLJIT_MOV_U16, [4] = SLJIT_MOV_U32, [8] = SLJIT_MOV };
  static const sljit_s32 load_s[] = { [1] = SLJIT_MOV_S8, [2] = SLJIT_MOV_S16, [4] = SLJIT_MOV_S32, [8] = SLJIT_MOV };
  if (is_store) {
    sljit_s32 v; sljit_sw vw;
    if (rf == RF_GPR) src_op(rd, &v, &vw);
    else { v = SLJIT_MEM1(REG_CPU); vw = fpr_off(rd); }
    sljit_emit_op1(c, SLJIT_MOV, SLJIT_R1, 0, v, vw);
    sljit_emit_op1(c, load_u[width], SLJIT_MEM2(SLJIT_R2, SLJIT_R0), 0, SLJIT_R1, 0);
  } else if (rf == RF_GPR) {
    sljit_emit_op1(c, (sign ? load_s : load_u)[width], SLJIT_R1, 0, SLJIT_MEM2(SLJIT_R2, SLJIT_R0), 0);
    emit_store_reg(t, rd, SLJIT_R1);
  } else {
    sljit_emit_op1(c, load_u[width], SLJIT_R1, 0, SLJIT_MEM2(SLJIT_R2, SLJIT_R0), 0);
    if (rf == RF_FPR32) {
      sljit_emit_op2(c, SLJIT_OR, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, (sljit_sw)0xffffffff00000000ull);
    }
    sljit_emit_op1(c, SLJIT_MOV, SLJIT_MEM1(REG_CPU), fpr_off(rd), SLJIT_R1, 0);
    emit_fs_dirty(t);
  }
  t->slow[k].resume = sljit_emit_label(c);
  return true;
}

static void emit_slow_paths(Tc *t) {
  struct sljit_compiler *c = t->c;
  for (int k = 0; k < t->nslow; k++) {
    struct sljit_label *l = sljit_emit_label(c);
    for (int j = 0; j < 2; j++) if (t->slow[k].from[j]) sljit_set_label(t->slow[k].from[j], l);
    Decode *d = t->slow[k].d;
    emit_sync_state(t, d);
    if (t->slow[k].rf != RF_GPR) {
      // Floating-point accesses take the interpreter's helper as a whole.
      sljit_emit_op1(c, SLJIT_MOV_P, SLJIT_R0, 0, SLJIT_IMM, (sljit_sw)d);
      emit_icall(c, SLJIT_ARGS1V(P), (void *)jit_eh_lookup(d->exec_id));
      struct sljit_jump *back = sljit_emit_jump(c, SLJIT_JUMP);
      sljit_set_label(back, t->slow[k].resume);
      continue;
    }
    // R0 still holds the address.
    sljit_emit_op1(c, SLJIT_MOV, SLJIT_R1, 0, SLJIT_R0, 0);
    sljit_emit_op1(c, SLJIT_MOV_P, SLJIT_R0, 0, SLJIT_IMM, (sljit_sw)d);
    int rd = reg_of(d->dest.preg);
    if (t->slow[k].is_store) {
      sljit_s32 v; sljit_sw vw;
      src_op(rd, &v, &vw);
      sljit_emit_op1(c, SLJIT_MOV, SLJIT_R2, 0, v, vw);
      emit_writeback_pinned(c);
      sljit_emit_op1(c, SLJIT_MOV, SLJIT_R3, 0, SLJIT_IMM, t->slow[k].width);
      sljit_emit_icall(c, SLJIT_CALL, SLJIT_ARGS4V(P, W, W, 32), SLJIT_IMM,
                       SLJIT_FUNC_ADDR(jit_helper_store));
      emit_reload_pinned(c);
    } else {
      sljit_emit_op1(c, SLJIT_MOV, SLJIT_R2, 0, SLJIT_IMM,
                     t->slow[k].width | (t->slow[k].sign ? 0x100 : 0));
      emit_icall(c, SLJIT_ARGS3(W, P, W, 32), (void *)jit_helper_load);
      emit_store_reg(t, rd, SLJIT_R0);
    }
    struct sljit_jump *back = sljit_emit_jump(c, SLJIT_JUMP);
    sljit_set_label(back, t->slow[k].resume);
  }
}

// Translate one instruction. Returns false when the instruction is not
// supported, so the block prefix ends before it.
static void emit_helper(Tc *t, Decode *d);
#if defined(CONFIG_FPU_SOFT_HOST_FASTPATH) && !defined(CONFIG_FPU_NONE)
static bool emit_fp_arith(Tc *t, Decode *d, int kind, bool w64, bool na, bool nc);
enum { FPK_ADD, FPK_SUB, FPK_MUL, FPK_DIV, FPK_FMA };
static bool emit_fp_cmp(Tc *t, Decode *d, sljit_s32 set, sljit_s32 cond, bool w64);
static void emit_helper(Tc *t, Decode *d);
static bool emit_fcvt_d_int(Tc *t, Decode *d, sljit_s32 conv, int range);
static bool emit_fcvt_d_s(Tc *t, Decode *d);
#endif

static bool emit_instr(Tc *t, Decode *d) {
  int rd  = reg_of(d->dest.preg);
  int rs1 = reg_of(d->src1.preg);
  int rs2 = reg_of(d->src2.preg);
  sljit_sw i1 = (sljit_sw)d->src1.imm;
  sljit_sw i2 = (sljit_sw)d->src2.imm;

#define RR(op, w)  do { if (rs1 < 0 || rs2 < 0) return false; emit_rr(t, op, w, rd, rs1, rs2); } while (0)
#define RI(op, w)  do { if (rs1 < 0) return false; emit_ri(t, op, w, rd, rs1, i2); } while (0)
// Compressed forms read the destination as their first source. A
// destination of x0 (HINT encodings such as c.nop) discards the result.
#define CRR(op, w) do { if (rs2 < 0) return false; if (rd >= 0) emit_rr(t, op, w, rd, rd, rs2); } while (0)
#define CRI(op, w) do { if (rd >= 0) emit_ri(t, op, w, rd, rd, i2); } while (0)

  switch (d->exec_id) {
    case EXEC_ID_add:  RR(SLJIT_ADD, false); return true;
    case EXEC_ID_sub:  RR(SLJIT_SUB, false); return true;
    case EXEC_ID_and:  RR(SLJIT_AND, false); return true;
    case EXEC_ID_or:   RR(SLJIT_OR,  false); return true;
    case EXEC_ID_xor:  RR(SLJIT_XOR, false); return true;
    case EXEC_ID_sll:  RR(SLJIT_MSHL,  false); return true;
    case EXEC_ID_srl:  RR(SLJIT_MLSHR, false); return true;
    case EXEC_ID_sra:  RR(SLJIT_MASHR, false); return true;
    case EXEC_ID_addw: RR(SLJIT_ADD, true); return true;
    case EXEC_ID_subw: RR(SLJIT_SUB, true); return true;
    case EXEC_ID_sllw: RR(SLJIT_MSHL,  true); return true;
    case EXEC_ID_srlw: RR(SLJIT_MLSHR, true); return true;
    case EXEC_ID_sraw: RR(SLJIT_MASHR, true); return true;
    case EXEC_ID_mul:  RR(SLJIT_MUL, false); return true;
    case EXEC_ID_mulw: RR(SLJIT_MUL, true); return true;
    case EXEC_ID_slt:
    case EXEC_ID_sltu: {
      if (rs1 < 0 || rs2 < 0) return false;
      sljit_s32 b; sljit_sw bw; src_op(rs2, &b, &bw);
      emit_slt(t, d->exec_id == EXEC_ID_slt, rd, b, bw, rs1);
      return true;
    }

    case EXEC_ID_addi:  RI(SLJIT_ADD, false); return true;
    case EXEC_ID_andi:  RI(SLJIT_AND, false); return true;
    case EXEC_ID_ori:   RI(SLJIT_OR,  false); return true;
    case EXEC_ID_xori:  RI(SLJIT_XOR, false); return true;
    case EXEC_ID_slli:  RI(SLJIT_MSHL,  false); return true;
    case EXEC_ID_srli:  RI(SLJIT_MLSHR, false); return true;
    case EXEC_ID_srai:  RI(SLJIT_MASHR, false); return true;
    case EXEC_ID_addiw: RI(SLJIT_ADD, true); return true;
    case EXEC_ID_slliw: RI(SLJIT_MSHL,  true); return true;
    case EXEC_ID_srliw: RI(SLJIT_MLSHR, true); return true;
    case EXEC_ID_sraiw: RI(SLJIT_MASHR, true); return true;
    case EXEC_ID_slti:
    case EXEC_ID_sltui:
      if (rs1 < 0) return false;
      emit_slt(t, d->exec_id == EXEC_ID_slti, rd, SLJIT_IMM, i2, rs1);
      return true;

    case EXEC_ID_lui:
    case EXEC_ID_auipc: emit_li(t, rd, i1); return true;
    case EXEC_ID_c_li:  emit_li(t, rd, i2); return true;
    case EXEC_ID_p_li_0: emit_li(t, rd, 0); return true;
    case EXEC_ID_p_li_1: emit_li(t, rd, 1); return true;
    case EXEC_ID_c_mv:
      if (rs1 < 0) return false;
      emit_ri(t, SLJIT_ADD, false, rd, rs1, 0);
      return true;
    case EXEC_ID_p_sext_w:
      if (rs1 < 0) return false;
      emit_ri(t, SLJIT_ADD, true, rd, rs1, 0);
      return true;
    case EXEC_ID_c_addi:  CRI(SLJIT_ADD, false); return true;
    case EXEC_ID_c_addiw: CRI(SLJIT_ADD, true); return true;
    case EXEC_ID_c_andi:  CRI(SLJIT_AND, false); return true;
    case EXEC_ID_c_slli:  CRI(SLJIT_MSHL,  false); return true;
    case EXEC_ID_c_srli:  CRI(SLJIT_MLSHR, false); return true;
    case EXEC_ID_c_srai:  CRI(SLJIT_MASHR, false); return true;
    case EXEC_ID_p_inc: if (rd >= 0) emit_ri(t, SLJIT_ADD, false, rd, rd, 1); return true;
    case EXEC_ID_p_dec: if (rd >= 0) emit_ri(t, SLJIT_SUB, false, rd, rd, 1); return true;
    case EXEC_ID_c_add:  CRR(SLJIT_ADD, false); return true;
    case EXEC_ID_c_and:  CRR(SLJIT_AND, false); return true;
    case EXEC_ID_c_or:   CRR(SLJIT_OR,  false); return true;
    case EXEC_ID_c_xor:  CRR(SLJIT_XOR, false); return true;
    case EXEC_ID_c_sub:  CRR(SLJIT_SUB, false); return true;
    case EXEC_ID_c_addw: CRR(SLJIT_ADD, true); return true;
    case EXEC_ID_c_subw: CRR(SLJIT_SUB, true); return true;

    case EXEC_ID_div:    if (rs1 < 0 || rs2 < 0) return false; emit_call2(t, jit_helper_div,   rd, rs1, rs2); return true;
    case EXEC_ID_divu:   if (rs1 < 0 || rs2 < 0) return false; emit_call2(t, jit_helper_divu,  rd, rs1, rs2); return true;
    case EXEC_ID_rem:    if (rs1 < 0 || rs2 < 0) return false; emit_call2(t, jit_helper_rem,   rd, rs1, rs2); return true;
    case EXEC_ID_remu:   if (rs1 < 0 || rs2 < 0) return false; emit_call2(t, jit_helper_remu,  rd, rs1, rs2); return true;
    case EXEC_ID_divw:   if (rs1 < 0 || rs2 < 0) return false; emit_call2(t, jit_helper_divw,  rd, rs1, rs2); return true;
    case EXEC_ID_divuw:  if (rs1 < 0 || rs2 < 0) return false; emit_call2(t, jit_helper_divuw, rd, rs1, rs2); return true;
    case EXEC_ID_remw:   if (rs1 < 0 || rs2 < 0) return false; emit_call2(t, jit_helper_remw,  rd, rs1, rs2); return true;
    case EXEC_ID_remuw:  if (rs1 < 0 || rs2 < 0) return false; emit_call2(t, jit_helper_remuw, rd, rs1, rs2); return true;
    case EXEC_ID_mulh:   if (rs1 < 0 || rs2 < 0) return false; emit_call2(t, jit_helper_mulh,  rd, rs1, rs2); return true;
    case EXEC_ID_mulhu:  if (rs1 < 0 || rs2 < 0) return false; emit_call2(t, jit_helper_mulhu, rd, rs1, rs2); return true;
    case EXEC_ID_mulhsu: if (rs1 < 0 || rs2 < 0) return false; emit_call2(t, jit_helper_mulhsu, rd, rs1, rs2); return true;

    // Memory: only the translated (*_mmu) variants run in host code.
    case EXEC_ID_ld_mmu:  return jit_opt.mem && emit_mem(t, d, 8, false, true, RF_GPR);
    case EXEC_ID_lw_mmu:  return jit_opt.mem && emit_mem(t, d, 4, false, true, RF_GPR);
    case EXEC_ID_lh_mmu:  return jit_opt.mem && emit_mem(t, d, 2, false, true, RF_GPR);
    case EXEC_ID_lb_mmu:  return jit_opt.mem && emit_mem(t, d, 1, false, true, RF_GPR);
    case EXEC_ID_lwu_mmu: return jit_opt.mem && emit_mem(t, d, 4, false, false, RF_GPR);
    case EXEC_ID_lhu_mmu: return jit_opt.mem && emit_mem(t, d, 2, false, false, RF_GPR);
    case EXEC_ID_lbu_mmu: return jit_opt.mem && emit_mem(t, d, 1, false, false, RF_GPR);
    case EXEC_ID_sd_mmu:  return jit_opt.mem && emit_mem(t, d, 8, true, false, RF_GPR);
    case EXEC_ID_sw_mmu:  return jit_opt.mem && emit_mem(t, d, 4, true, false, RF_GPR);
    case EXEC_ID_sh_mmu:  return jit_opt.mem && emit_mem(t, d, 2, true, false, RF_GPR);
    case EXEC_ID_sb_mmu:  return jit_opt.mem && emit_mem(t, d, 1, true, false, RF_GPR);
#ifndef CONFIG_FPU_NONE
    case EXEC_ID_fld_mmu: return jit_opt.mem && emit_mem(t, d, 8, false, false, RF_FPR64);
    case EXEC_ID_flw_mmu: return jit_opt.mem && emit_mem(t, d, 4, false, false, RF_FPR32);
    case EXEC_ID_fsd_mmu: return jit_opt.mem && emit_mem(t, d, 8, true, false, RF_FPR64);
    case EXEC_ID_fsw_mmu: return jit_opt.mem && emit_mem(t, d, 4, true, false, RF_FPR64);
    case EXEC_ID_fmv_x_d: {
      int fs1 = freg_of(d->src1.preg);
      if (fs1 < 0) return false;
      if (rd >= 0) {
        sljit_emit_op1(t->c, SLJIT_MOV, SLJIT_R0, 0, SLJIT_MEM1(REG_CPU), fpr_off(fs1));
        emit_store_reg(t, rd, SLJIT_R0);
      }
      return true;
    }
    case EXEC_ID_fmv_d_x: {
      int fd = freg_of(d->dest.preg);
      if (fd < 0 || rs1 < 0) return false;
      sljit_s32 a; sljit_sw aw; src_op(rs1, &a, &aw);
      sljit_emit_op1(t->c, SLJIT_MOV, SLJIT_R0, 0, a, aw);
      sljit_emit_op1(t->c, SLJIT_MOV, SLJIT_MEM1(REG_CPU), fpr_off(fd), SLJIT_R0, 0);
      emit_fs_dirty(t);
      return true;
    }
#ifdef CONFIG_FPU_SOFT_HOST_FASTPATH
    case EXEC_ID_faddd: return jit_opt.inline_fp && emit_fp_arith(t, d, FPK_ADD, true, false, false);
    case EXEC_ID_fsubd: return jit_opt.inline_fp && emit_fp_arith(t, d, FPK_SUB, true, false, false);
    case EXEC_ID_fmuld: return jit_opt.inline_fp && emit_fp_arith(t, d, FPK_MUL, true, false, false);
    case EXEC_ID_fdivd: return jit_opt.inline_fp && emit_fp_arith(t, d, FPK_DIV, true, false, false);
    case EXEC_ID_fmaddd:  return jit_opt.inline_fp && emit_fp_arith(t, d, FPK_FMA, true, false, false);
    case EXEC_ID_fmsubd:  return jit_opt.inline_fp && emit_fp_arith(t, d, FPK_FMA, true, false, true);
    case EXEC_ID_fnmsubd: return jit_opt.inline_fp && emit_fp_arith(t, d, FPK_FMA, true, true, false);
    case EXEC_ID_fnmaddd: return jit_opt.inline_fp && emit_fp_arith(t, d, FPK_FMA, true, true, true);
    case EXEC_ID_fadds: return jit_opt.inline_fp && emit_fp_arith(t, d, FPK_ADD, false, false, false);
    case EXEC_ID_fsubs: return jit_opt.inline_fp && emit_fp_arith(t, d, FPK_SUB, false, false, false);
    case EXEC_ID_fmuls: return jit_opt.inline_fp && emit_fp_arith(t, d, FPK_MUL, false, false, false);
    case EXEC_ID_fdivs: return jit_opt.inline_fp && emit_fp_arith(t, d, FPK_DIV, false, false, false);
    case EXEC_ID_fmadds:  return jit_opt.inline_fp && emit_fp_arith(t, d, FPK_FMA, false, false, false);
    case EXEC_ID_fmsubs:  return jit_opt.inline_fp && emit_fp_arith(t, d, FPK_FMA, false, false, true);
    case EXEC_ID_fnmsubs: return jit_opt.inline_fp && emit_fp_arith(t, d, FPK_FMA, false, true, false);
    case EXEC_ID_fnmadds: return jit_opt.inline_fp && emit_fp_arith(t, d, FPK_FMA, false, true, true);
    case EXEC_ID_fltd: return jit_opt.inline_fp && emit_fp_cmp(t, d, SLJIT_SET_F_LESS, SLJIT_F_LESS, true);
    case EXEC_ID_fled: return jit_opt.inline_fp && emit_fp_cmp(t, d, SLJIT_SET_F_LESS_EQUAL, SLJIT_F_LESS_EQUAL, true);
    case EXEC_ID_feqd: return jit_opt.inline_fp && emit_fp_cmp(t, d, SLJIT_SET_F_EQUAL, SLJIT_F_EQUAL, true);
    case EXEC_ID_flts: return jit_opt.inline_fp && emit_fp_cmp(t, d, SLJIT_SET_F_LESS, SLJIT_F_LESS, false);
    case EXEC_ID_fles: return jit_opt.inline_fp && emit_fp_cmp(t, d, SLJIT_SET_F_LESS_EQUAL, SLJIT_F_LESS_EQUAL, false);
    case EXEC_ID_feqs: return jit_opt.inline_fp && emit_fp_cmp(t, d, SLJIT_SET_F_EQUAL, SLJIT_F_EQUAL, false);
    case EXEC_ID_fcvt_d_w:  return jit_opt.inline_fp && emit_fcvt_d_int(t, d, SLJIT_CONV_F64_FROM_S32, 0);
    case EXEC_ID_fcvt_d_wu: return jit_opt.inline_fp && emit_fcvt_d_int(t, d, SLJIT_CONV_F64_FROM_U32, 0);
    case EXEC_ID_fcvt_d_l:  return jit_opt.inline_fp && emit_fcvt_d_int(t, d, SLJIT_CONV_F64_FROM_SW, 1);
    case EXEC_ID_fcvt_d_lu: return jit_opt.inline_fp && emit_fcvt_d_int(t, d, SLJIT_CONV_F64_FROM_UW, 2);
    case EXEC_ID_fcvt_d_s:  return jit_opt.inline_fp && emit_fcvt_d_s(t, d);
#endif
    case EXEC_ID_fmv_w_x: {
      int fd = freg_of(d->dest.preg);
      if (fd < 0 || rs1 < 0) return false;
      sljit_s32 a; sljit_sw aw; src_op(rs1, &a, &aw);
      sljit_emit_op2(t->c, SLJIT_OR, SLJIT_R0, 0, a, aw, SLJIT_IMM, (sljit_sw)0xffffffff00000000ull);
      sljit_emit_op1(t->c, SLJIT_MOV, SLJIT_MEM1(REG_CPU), fpr_off(fd), SLJIT_R0, 0);
      emit_fs_dirty(t);
      return true;
    }
    case EXEC_ID_fmv_x_w: {
      int fs1 = freg_of(d->src1.preg);
      if (fs1 < 0) return false;
      if (rd >= 0) {
        sljit_emit_op1(t->c, SLJIT_MOV_S32, SLJIT_R0, 0, SLJIT_MEM1(REG_CPU), fpr_off(fs1));
        emit_store_reg(t, rd, SLJIT_R0);
      }
      return true;
    }
    case EXEC_ID_fsgnjs:
    case EXEC_ID_fsgnjns:
    case EXEC_ID_fsgnjxs: {
      // Only properly NaN-boxed operands are handled inline.
      int fd = freg_of(d->dest.preg), f1 = freg_of(d->src1.preg), f2 = freg_of(d->src2.preg);
      if (fd < 0 || f1 < 0 || f2 < 0) return false;
      struct sljit_compiler *c = t->c;
      struct sljit_jump *fail[2];
      sljit_emit_op1(c, SLJIT_MOV, SLJIT_R0, 0, SLJIT_MEM1(REG_CPU), fpr_off(f1));
      sljit_emit_op1(c, SLJIT_MOV, SLJIT_R1, 0, SLJIT_MEM1(REG_CPU), fpr_off(f2));
      sljit_emit_op2(c, SLJIT_LSHR, SLJIT_R2, 0, SLJIT_R0, 0, SLJIT_IMM, 32);
      fail[0] = sljit_emit_cmp(c, SLJIT_NOT_EQUAL, SLJIT_R2, 0, SLJIT_IMM, 0xffffffff);
      sljit_emit_op2(c, SLJIT_LSHR, SLJIT_R2, 0, SLJIT_R1, 0, SLJIT_IMM, 32);
      fail[1] = sljit_emit_cmp(c, SLJIT_NOT_EQUAL, SLJIT_R2, 0, SLJIT_IMM, 0xffffffff);
      sljit_sw sign = 1u << 31;
      if (d->exec_id == EXEC_ID_fsgnjns) sljit_emit_op2(c, SLJIT_XOR, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, -1);
      if (d->exec_id == EXEC_ID_fsgnjxs) sljit_emit_op2(c, SLJIT_XOR, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_R0, 0);
      sljit_emit_op2(c, SLJIT_AND, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, sign);
      sljit_emit_op2(c, SLJIT_AND, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, ~sign);
      sljit_emit_op2(c, SLJIT_OR, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
      sljit_emit_op1(c, SLJIT_MOV, SLJIT_MEM1(REG_CPU), fpr_off(fd), SLJIT_R0, 0);
      emit_fs_dirty(t);
      struct sljit_jump *done = sljit_emit_jump(c, SLJIT_JUMP);
      struct sljit_label *slow = sljit_emit_label(c);
      sljit_set_label(fail[0], slow);
      sljit_set_label(fail[1], slow);
      emit_helper(t, d);
      sljit_set_label(done, sljit_emit_label(c));
      return true;
    }
    case EXEC_ID_fsgnjd:
    case EXEC_ID_fsgnjnd:
    case EXEC_ID_fsgnjxd: {
      int fd = freg_of(d->dest.preg), f1 = freg_of(d->src1.preg), f2 = freg_of(d->src2.preg);
      if (fd < 0 || f1 < 0 || f2 < 0) return false;
      struct sljit_compiler *c = t->c;
      sljit_sw sign = (sljit_sw)(1ull << 63);
      sljit_emit_op1(c, SLJIT_MOV, SLJIT_R0, 0, SLJIT_MEM1(REG_CPU), fpr_off(f1));
      sljit_emit_op1(c, SLJIT_MOV, SLJIT_R1, 0, SLJIT_MEM1(REG_CPU), fpr_off(f2));
      if (d->exec_id == EXEC_ID_fsgnjnd) sljit_emit_op2(c, SLJIT_XOR, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, -1);
      if (d->exec_id == EXEC_ID_fsgnjxd) sljit_emit_op2(c, SLJIT_XOR, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_R0, 0);
      sljit_emit_op2(c, SLJIT_AND, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, sign);
      sljit_emit_op2(c, SLJIT_AND, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, ~sign);
      sljit_emit_op2(c, SLJIT_OR, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
      sljit_emit_op1(c, SLJIT_MOV, SLJIT_MEM1(REG_CPU), fpr_off(fd), SLJIT_R0, 0);
      emit_fs_dirty(t);
      return true;
    }
#endif

    // Control transfer: only as the block terminator.
    case EXEC_ID_jal:
      emit_li(t, rd, i2);
      emit_jump(t);
      return true;
    case EXEC_ID_p_jal:
      emit_li(t, 1, i2);
      emit_jump(t);
      return true;
    case EXEC_ID_c_j:
      emit_jump(t);
      return true;
    case EXEC_ID_beq:  if (rs1 < 0 || rs2 < 0) return false; emit_branch(t, SLJIT_EQUAL, rs1, rs2); return true;
    case EXEC_ID_bne:  if (rs1 < 0 || rs2 < 0) return false; emit_branch(t, SLJIT_NOT_EQUAL, rs1, rs2); return true;
    case EXEC_ID_blt:  if (rs1 < 0 || rs2 < 0) return false; emit_branch(t, SLJIT_SIG_LESS, rs1, rs2); return true;
    case EXEC_ID_bge:  if (rs1 < 0 || rs2 < 0) return false; emit_branch(t, SLJIT_SIG_GREATER_EQUAL, rs1, rs2); return true;
    case EXEC_ID_bltu: if (rs1 < 0 || rs2 < 0) return false; emit_branch(t, SLJIT_LESS, rs1, rs2); return true;
    case EXEC_ID_bgeu: if (rs1 < 0 || rs2 < 0) return false; emit_branch(t, SLJIT_GREATER_EQUAL, rs1, rs2); return true;
    case EXEC_ID_c_beqz: if (rs1 < 0) return false; emit_branch(t, SLJIT_EQUAL, rs1, 0); return true;
    case EXEC_ID_c_bnez: if (rs1 < 0) return false; emit_branch(t, SLJIT_NOT_EQUAL, rs1, 0); return true;
    case EXEC_ID_p_blez: if (rs2 < 0) return false; emit_branch(t, SLJIT_SIG_GREATER_EQUAL, 0, rs2); return true;
    case EXEC_ID_p_bgtz: if (rs2 < 0) return false; emit_branch(t, SLJIT_SIG_LESS, 0, rs2); return true;
    case EXEC_ID_p_bltz: if (rs1 < 0) return false; emit_branch(t, SLJIT_SIG_LESS, rs1, 0); return true;
    case EXEC_ID_p_bgez: if (rs1 < 0) return false; emit_branch(t, SLJIT_SIG_GREATER_EQUAL, rs1, 0); return true;
    case EXEC_ID_jalr: {
      if (rs1 < 0) return false;
      sljit_s32 a; sljit_sw aw; src_op(rs1, &a, &aw);
      sljit_emit_op2(t->c, SLJIT_ADD, SLJIT_R0, 0, a, aw, SLJIT_IMM, i2);
      sljit_emit_op2(t->c, SLJIT_AND, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, ~(sljit_sw)1);
      emit_li(t, rd, (sljit_sw)d->snpc);
      emit_indirect(t);
      return true;
    }
    case EXEC_ID_c_jr: {
      if (rs1 < 0) return false;
      sljit_s32 a; sljit_sw aw; src_op(rs1, &a, &aw);
      sljit_emit_op1(t->c, SLJIT_MOV, SLJIT_R0, 0, a, aw);
      emit_indirect(t);
      return true;
    }
    case EXEC_ID_c_jalr: {
      // The interpreter writes the link register before reading rs1.
      if (rs1 < 0) return false;
      emit_li(t, 1, (sljit_sw)d->snpc);
      sljit_s32 a; sljit_sw aw; src_op(rs1, &a, &aw);
      sljit_emit_op1(t->c, SLJIT_MOV, SLJIT_R0, 0, a, aw);
      emit_indirect(t);
      return true;
    }
    case EXEC_ID_p_ret:
      { sljit_s32 a; sljit_sw aw; src_op(1, &a, &aw); sljit_emit_op1(t->c, SLJIT_MOV, SLJIT_R0, 0, a, aw); }
      emit_indirect(t);
      return true;

#ifdef CONFIG_RVB
    case EXEC_ID_sh1add:   if (rs1 < 0 || rs2 < 0) return false; emit_shadd(t, 1, false, true, rd, rs1, rs2); return true;
    case EXEC_ID_sh2add:   if (rs1 < 0 || rs2 < 0) return false; emit_shadd(t, 2, false, true, rd, rs1, rs2); return true;
    case EXEC_ID_sh3add:   if (rs1 < 0 || rs2 < 0) return false; emit_shadd(t, 3, false, true, rd, rs1, rs2); return true;
    case EXEC_ID_sh1adduw: if (rs1 < 0 || rs2 < 0) return false; emit_shadd(t, 1, true, true, rd, rs1, rs2); return true;
    case EXEC_ID_sh2adduw: if (rs1 < 0 || rs2 < 0) return false; emit_shadd(t, 2, true, true, rd, rs1, rs2); return true;
    case EXEC_ID_sh3adduw: if (rs1 < 0 || rs2 < 0) return false; emit_shadd(t, 3, true, true, rd, rs1, rs2); return true;
    case EXEC_ID_adduw:    if (rs1 < 0 || rs2 < 0) return false; emit_shadd(t, 0, true, true, rd, rs1, rs2); return true;
    case EXEC_ID_slliuw:   if (rs1 < 0) return false; emit_shadd(t, i2 & 0x3f, true, false, rd, rs1, 0); return true;
    case EXEC_ID_andn: if (rs1 < 0 || rs2 < 0) return false; emit_notop(t, SLJIT_AND, rd, rs1, rs2); return true;
    case EXEC_ID_orn:  if (rs1 < 0 || rs2 < 0) return false; emit_notop(t, SLJIT_OR,  rd, rs1, rs2); return true;
    case EXEC_ID_xnor: if (rs1 < 0 || rs2 < 0) return false; emit_notop(t, SLJIT_XOR, rd, rs1, rs2); return true;
    case EXEC_ID_min:  if (rs1 < 0 || rs2 < 0) return false; emit_minmax(t, SLJIT_SIG_GREATER, rd, rs1, rs2); return true;
    case EXEC_ID_max:  if (rs1 < 0 || rs2 < 0) return false; emit_minmax(t, SLJIT_SIG_LESS,    rd, rs1, rs2); return true;
    case EXEC_ID_minu: if (rs1 < 0 || rs2 < 0) return false; emit_minmax(t, SLJIT_GREATER,     rd, rs1, rs2); return true;
    case EXEC_ID_maxu: if (rs1 < 0 || rs2 < 0) return false; emit_minmax(t, SLJIT_LESS,        rd, rs1, rs2); return true;
    case EXEC_ID_sext_b: if (rs1 < 0) return false; emit_ext(t, SLJIT_MOV_S8,  rd, rs1); return true;
    case EXEC_ID_sext_h: if (rs1 < 0) return false; emit_ext(t, SLJIT_MOV_S16, rd, rs1); return true;
    case EXEC_ID_zext_h: if (rs1 < 0) return false; emit_ext(t, SLJIT_MOV_U16, rd, rs1); return true;
#define UNARY(id, fn, w) case EXEC_ID_##id: if (rs1 < 0) return false; emit_call_ri(t, (void *)fn, false, 0, w, rd, rs1); return true;
#define IMMOP(id, fn, w) case EXEC_ID_##id: if (rs1 < 0) return false; emit_call_ri(t, (void *)fn, true, i2, w, rd, rs1); return true;
#define BINOP(id, fn, w) case EXEC_ID_##id: if (rs1 < 0 || rs2 < 0) return false; emit_call_rr(t, (void *)fn, w, rd, rs1, rs2); return true;
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
    case EXEC_ID_czero_eqz: if (rs1 < 0 || rs2 < 0) return false; emit_czero(t, true,  rd, rs1, rs2); return true;
    case EXEC_ID_czero_nez: if (rs1 < 0 || rs2 < 0) return false; emit_czero(t, false, rd, rs1, rs2); return true;
#endif

#ifdef CONFIG_RV_ZCB
    case EXEC_ID_c_zext_b: if (rs1 < 0) return false; emit_ext(t, SLJIT_MOV_U8,  rd, rs1); return true;
    case EXEC_ID_c_sext_b: if (rs1 < 0) return false; emit_ext(t, SLJIT_MOV_S8,  rd, rs1); return true;
    case EXEC_ID_c_zext_h: if (rs1 < 0) return false; emit_ext(t, SLJIT_MOV_U16, rd, rs1); return true;
    case EXEC_ID_c_sext_h: if (rs1 < 0) return false; emit_ext(t, SLJIT_MOV_S16, rd, rs1); return true;
    case EXEC_ID_c_zext_w: if (rs1 < 0) return false; emit_ext(t, SLJIT_MOV_U32, rd, rs1); return true;
    case EXEC_ID_c_not:    if (rs1 < 0) return false; emit_ri(t, SLJIT_XOR, false, rd, rs1, -1); return true;
    case EXEC_ID_c_mul:    RR(SLJIT_MUL, false); return true;
#endif

    default:
      return false;
  }
#undef RR
#undef RI
#undef CRR
#undef CRI
}

static bool is_terminator(const Decode *d) { return d->type != INSTR_TYPE_N; }
static void emit_helper(Tc *t, Decode *d);

#if defined(CONFIG_FPU_SOFT_HOST_FASTPATH) && !defined(CONFIG_FPU_NONE)
// Inline floating-point arithmetic under the conditions of the interpreter's
// host fast path; any other case runs the instruction's helper.

// Fast-path windows, as bounds on the unbiased exponent. Sums of operands
// in the wide window (or zeros) are exact or normal and cannot overflow.
// For double precision, products, quotients and fused multiply-adds whose
// factors lie in the factor window (and whose addend lies in the wide one,
// or is zero) are zero or normal and finite as well: an exact product or
// fused sum is a multiple of 2^-1004, a quotient exceeds 2^-902, and all lie
// below 2^903 in magnitude. Such results need no check; single-precision
// products are checked against the single window.
#define FP_WIN_D        900
#define FP_WIN_D_FACTOR 450
#define FP_WIN_S        100

// Jump to `fail` unless the value in `bits` is a finite number whose
// unbiased exponent lies in [-bound, bound] (or zero, when allowed).
static void emit_window_check(Tc *t, sljit_s32 bits, bool w64, bool allow_zero, int bound,
                              struct sljit_jump **fail, int *nfail) {
  struct sljit_compiler *c = t->c;
  // Shifting out the sign (and the NaN box) leaves the exponent on top and
  // makes the value zero exactly for a signed zero.
  sljit_s32 sh = w64 ? 1 : 33;
  struct sljit_jump *zero = NULL;
  if (allow_zero) {
    zero = sljit_emit_op2cmpz(c, SLJIT_SHL | SLJIT_JUMP_IF_ZERO, SLJIT_R2, 0, bits, 0, SLJIT_IMM, sh);
  } else {
    sljit_emit_op2(c, SLJIT_SHL, SLJIT_R2, 0, bits, 0, SLJIT_IMM, sh);
  }
  sljit_emit_op2(c, SLJIT_LSHR, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_IMM, w64 ? 53 : 56);
  sljit_emit_op2(c, SLJIT_SUB, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_IMM, (w64 ? 1023 : 127) - bound);
  fail[(*nfail)++] = sljit_emit_cmp(c, SLJIT_GREATER, SLJIT_R2, 0, SLJIT_IMM, 2 * bound);
  if (zero) sljit_set_label(zero, sljit_emit_label(c));
}

// Raw floating-point register access for speculative code (spec_fp).
static void emit_fp_load(Tc *t, sljit_s32 fr, int f, bool w64, bool negate) {
  struct sljit_compiler *c = t->c;
  sljit_emit_fop1(c, w64 ? SLJIT_MOV_F64 : SLJIT_MOV_F32, fr, 0, SLJIT_MEM1(REG_CPU), fpr_off(f));
  if (negate) sljit_emit_fop1(c, w64 ? SLJIT_NEG_F64 : SLJIT_NEG_F32, fr, 0, fr, 0);
}

static void emit_fp_store(Tc *t, int f, sljit_s32 fr, bool w64) {
  struct sljit_compiler *c = t->c;
  sljit_emit_fop1(c, w64 ? SLJIT_MOV_F64 : SLJIT_MOV_F32, SLJIT_MEM1(REG_CPU), fpr_off(f), fr, 0);
  if (!w64) sljit_emit_op1(c, SLJIT_MOV32, SLJIT_MEM1(REG_CPU), fpr_off(f) + 4, SLJIT_IMM, -1);  // NaN box
  emit_fs_dirty(t);
}

// Load a floating-point source into `reg`, checking NaN boxing for single
// precision and optionally flipping its sign.
static void emit_fp_src(Tc *t, sljit_s32 reg, int f, bool w64, bool negate,
                        struct sljit_jump **fail, int *nfail) {
  struct sljit_compiler *c = t->c;
  sljit_emit_op1(c, SLJIT_MOV, reg, 0, SLJIT_MEM1(REG_CPU), fpr_off(f));
  if (!w64) {
    sljit_emit_op2(c, SLJIT_LSHR, SLJIT_R2, 0, reg, 0, SLJIT_IMM, 32);
    fail[(*nfail)++] = sljit_emit_cmp(c, SLJIT_NOT_EQUAL, SLJIT_R2, 0, SLJIT_IMM, 0xffffffff);
  }
  if (negate) {
    sljit_emit_op2(c, SLJIT_XOR, reg, 0, reg, 0, SLJIT_IMM,
                   w64 ? (sljit_sw)(1ull << 63) : (sljit_sw)(1u << 31));
  }
}

static bool host_has_fma(void) {
  static int cached = -1;
  if (cached < 0) cached = __builtin_cpu_supports("fma");
  return cached;
}

// vfmadd231s{d,s} a, b, c: a = b * c + a (x86-64 VEX encoding).
static void emit_x86_fma231(struct sljit_compiler *c, bool w64, sljit_s32 fa, sljit_s32 fb, sljit_s32 fc) {
  int a = sljit_get_register_index(SLJIT_FLOAT_REGISTER, fa);
  int b = sljit_get_register_index(SLJIT_FLOAT_REGISTER, fb);
  int x = sljit_get_register_index(SLJIT_FLOAT_REGISTER, fc);
  uint8_t insn[5] = {
    0xc4,
    (uint8_t)(((a < 8) << 7) | (1 << 6) | ((x < 8) << 5) | 0x02),
    (uint8_t)(((w64 ? 1 : 0) << 7) | ((~b & 0xf) << 3) | 0x01),
    0xb9,
    (uint8_t)(0xc0 | ((a & 7) << 3) | (x & 7)),
  };
  sljit_emit_op_custom(c, insn, sizeof(insn));
}

// Jump to `fail` when the value in `bits` is a NaN or an infinity.
static void emit_finite_check(Tc *t, sljit_s32 bits, bool w64, struct sljit_jump **fail, int *nfail) {
  struct sljit_compiler *c = t->c;
  sljit_emit_op2(c, SLJIT_LSHR, SLJIT_R2, 0, bits, 0, SLJIT_IMM, w64 ? 52 : 23);
  sljit_emit_op2(c, SLJIT_AND, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_IMM, w64 ? 0x7ff : 0xff);
  fail[(*nfail)++] = sljit_emit_cmp(c, SLJIT_EQUAL, SLJIT_R2, 0, SLJIT_IMM, w64 ? 0x7ff : 0xff);
}

// Jump to `fail` unless the rounding mode of d is valid (a dynamic mode
// must hold a defined value in frm); invalid modes raise an exception.
static bool emit_rm_valid_check(Tc *t, Decode *d, struct sljit_jump **fail, int *nfail) {
  int rm = d->isa.instr.fp.rm;
  if (rm == 5 || rm == 6) return false;
  if (rm == 7) {
    struct sljit_compiler *c = t->c;
    sljit_emit_op1(c, SLJIT_MOV, SLJIT_R2, 0, SLJIT_MEM0(), (sljit_sw)&csr_array[0x003]);
    sljit_emit_op2(c, SLJIT_AND, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_IMM, 0xe0);
    fail[(*nfail)++] = sljit_emit_cmp(c, SLJIT_GREATER, SLJIT_R2, 0, SLJIT_IMM, 4 << 5);
  }
  return true;
}

// A product with a zero factor and a finite other factor is an exact signed
// zero under every rounding mode, so fmul gives that zero and a fused
// multiply-add with a finite, non-zero addend gives the addend. No flag is
// raised. Returns the jump taken after writing the result; control falls
// through to the helper when the case does not apply.
static struct sljit_jump *emit_fp_zero_factor(Tc *t, Decode *d, bool fma, bool w64,
                                              bool na, bool nc, int fd) {
  struct sljit_compiler *c = t->c;
  struct sljit_jump *fail[16];
  int nfail = 0;
  int f1 = freg_of(d->src1.preg), f2 = freg_of(d->src2.preg);
  if (!emit_rm_valid_check(t, d, fail, &nfail)) return NULL;
  emit_fp_src(t, SLJIT_R0, f1, w64, fma && na, fail, &nfail);
  emit_finite_check(t, SLJIT_R0, w64, fail, &nfail);
  emit_fp_src(t, SLJIT_R1, f2, w64, false, fail, &nfail);
  emit_finite_check(t, SLJIT_R1, w64, fail, &nfail);
  // One factor must be zero.
  sljit_s32 sh = w64 ? 1 : 33;
  struct sljit_jump *a_zero = sljit_emit_op2cmpz(c, SLJIT_SHL | SLJIT_JUMP_IF_ZERO, SLJIT_R2, 0, SLJIT_R0, 0, SLJIT_IMM, sh);
  fail[nfail++] = sljit_emit_op2cmpz(c, SLJIT_SHL | SLJIT_JUMP_IF_NON_ZERO, SLJIT_R2, 0, SLJIT_R1, 0, SLJIT_IMM, sh);
  sljit_set_label(a_zero, sljit_emit_label(c));
  if (fma) {
    emit_fp_src(t, SLJIT_R0, d->isa.instr.fp.funct5, w64, nc, fail, &nfail);
    emit_finite_check(t, SLJIT_R0, w64, fail, &nfail);
    fail[nfail++] = sljit_emit_op2cmpz(c, SLJIT_SHL | SLJIT_JUMP_IF_ZERO, SLJIT_R2, 0, SLJIT_R0, 0, SLJIT_IMM, sh);
    if (!w64) sljit_emit_op1(c, SLJIT_MOV_U32, SLJIT_R0, 0, SLJIT_R0, 0);
  } else {
    sljit_emit_op2(c, SLJIT_XOR, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
    sljit_emit_op2(c, SLJIT_AND, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM,
                   w64 ? (sljit_sw)(1ull << 63) : (sljit_sw)(1u << 31));
  }
  if (!w64) sljit_emit_op2(c, SLJIT_OR, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, (sljit_sw)0xffffffff00000000ull);
  sljit_emit_op1(c, SLJIT_MOV, SLJIT_MEM1(REG_CPU), fpr_off(fd), SLJIT_R0, 0);
  emit_fs_dirty(t);
  struct sljit_jump *done = sljit_emit_jump(c, SLJIT_JUMP);
  struct sljit_label *slow = sljit_emit_label(c);
  for (int i = 0; i < nfail; i++) sljit_set_label(fail[i], slow);
  return done;
}

// flt/fle/feq on non-NaN operands raise no flag. Infinities also take the
// helper, which keeps the check to one compare.
static bool emit_fp_cmp(Tc *t, Decode *d, sljit_s32 set, sljit_s32 cond, bool w64) {
  struct sljit_compiler *c = t->c;
  int f1 = freg_of(d->src1.preg), f2 = freg_of(d->src2.preg);
  int rd = reg_of(d->dest.preg);
  if (f1 < 0 || f2 < 0) return false;
  if ((jit_opt.spec_fp && jit_leader_mode)) {
    // Any NaN operand makes the comparison false, as in RISC-V.
    set = cond == SLJIT_F_EQUAL ? SLJIT_SET_ORDERED_EQUAL : cond == SLJIT_F_LESS ? SLJIT_SET_ORDERED_LESS
                                                                                 : SLJIT_SET_ORDERED_LESS_EQUAL;
    cond = cond == SLJIT_F_EQUAL ? SLJIT_ORDERED_EQUAL : cond == SLJIT_F_LESS ? SLJIT_ORDERED_LESS
                                                                              : SLJIT_ORDERED_LESS_EQUAL;
    emit_fp_load(t, SLJIT_FR0, f1, w64, false);
    emit_fp_load(t, SLJIT_FR1, f2, w64, false);
    sljit_emit_fop1(c, (w64 ? SLJIT_CMP_F64 : SLJIT_CMP_F32) | set, SLJIT_FR0, 0, SLJIT_FR1, 0);
    sljit_emit_op_flags(c, SLJIT_MOV, SLJIT_R0, 0, cond);
    emit_store_reg(t, rd, SLJIT_R0);
    return true;
  }
  struct sljit_jump *fail[8];
  int nfail = 0;
  emit_fp_src(t, SLJIT_R0, f1, w64, false, fail, &nfail);
  emit_finite_check(t, SLJIT_R0, w64, fail, &nfail);
  emit_fp_src(t, SLJIT_R1, f2, w64, false, fail, &nfail);
  emit_finite_check(t, SLJIT_R1, w64, fail, &nfail);
  sljit_s32 cp = w64 ? SLJIT_COPY_TO_F64 : SLJIT_COPY32_TO_F32;
  sljit_emit_fcopy(c, cp, SLJIT_FR0, SLJIT_R0);
  sljit_emit_fcopy(c, cp, SLJIT_FR1, SLJIT_R1);
  sljit_emit_fop1(c, (w64 ? SLJIT_CMP_F64 : SLJIT_CMP_F32) | set, SLJIT_FR0, 0, SLJIT_FR1, 0);
  sljit_emit_op_flags(c, SLJIT_MOV, SLJIT_R0, 0, cond);
  emit_store_reg(t, rd, SLJIT_R0);
  emit_fs_dirty(t);
  struct sljit_jump *done = sljit_emit_jump(c, SLJIT_JUMP);
  struct sljit_label *slow = sljit_emit_label(c);
  for (int i = 0; i < nfail; i++) sljit_set_label(fail[i], slow);
  emit_helper(t, d);
  sljit_set_label(done, sljit_emit_label(c));
  return true;
}

// Integer to double conversions that are exact. range: 0 for 32-bit
// sources, 1 for signed 64-bit and 2 for unsigned 64-bit sources, which are
// exact below 2^53 in magnitude.
static bool emit_fcvt_d_int(Tc *t, Decode *d, sljit_s32 conv, int range) {
  struct sljit_compiler *c = t->c;
  int fd = freg_of(d->dest.preg), rs1 = reg_of(d->src1.preg);
  if (fd < 0 || rs1 < 0) return false;
  struct sljit_jump *fail[4];
  int nfail = 0;
  if (!emit_rm_valid_check(t, d, fail, &nfail)) return false;
  sljit_s32 a; sljit_sw aw;
  src_op(rs1, &a, &aw);
  sljit_emit_op1(c, SLJIT_MOV, SLJIT_R0, 0, a, aw);
  if ((jit_opt.spec_fp && jit_leader_mode)) range = 0;   // inexact conversions raise NX on the host
  if (range == 1) {
    sljit_emit_op2(c, SLJIT_ADD, SLJIT_R1, 0, SLJIT_R0, 0, SLJIT_IMM, (sljit_sw)1 << 53);
    fail[nfail++] = sljit_emit_cmp(c, SLJIT_GREATER_EQUAL, SLJIT_R1, 0, SLJIT_IMM, (sljit_sw)1 << 54);
  } else if (range == 2) {
    fail[nfail++] = sljit_emit_cmp(c, SLJIT_GREATER_EQUAL, SLJIT_R0, 0, SLJIT_IMM, (sljit_sw)1 << 53);
  }
  sljit_emit_fop1(c, conv, SLJIT_FR0, 0, SLJIT_R0, 0);
  sljit_emit_fcopy(c, SLJIT_COPY_FROM_F64, SLJIT_FR0, SLJIT_R0);
  sljit_emit_op1(c, SLJIT_MOV, SLJIT_MEM1(REG_CPU), fpr_off(fd), SLJIT_R0, 0);
  emit_fs_dirty(t);
  struct sljit_jump *done = sljit_emit_jump(c, SLJIT_JUMP);
  struct sljit_label *slow = sljit_emit_label(c);
  for (int i = 0; i < nfail; i++) sljit_set_label(fail[i], slow);
  emit_helper(t, d);
  sljit_set_label(done, sljit_emit_label(c));
  return true;
}

// Widening single to double is exact for every non-NaN value.
static bool emit_fcvt_d_s(Tc *t, Decode *d) {
  struct sljit_compiler *c = t->c;
  int fd = freg_of(d->dest.preg), f1 = freg_of(d->src1.preg);
  if (fd < 0 || f1 < 0) return false;
  if ((jit_opt.spec_fp && jit_leader_mode)) {
    emit_fp_load(t, SLJIT_FR0, f1, false, false);
    sljit_emit_fop1(c, SLJIT_CONV_F64_FROM_F32, SLJIT_FR0, 0, SLJIT_FR0, 0);
    emit_fp_store(t, fd, SLJIT_FR0, true);
    return true;
  }
  struct sljit_jump *fail[4];
  int nfail = 0;
  if (!emit_rm_valid_check(t, d, fail, &nfail)) return false;
  emit_fp_src(t, SLJIT_R0, f1, false, false, fail, &nfail);
  emit_finite_check(t, SLJIT_R0, false, fail, &nfail);
  sljit_emit_fcopy(c, SLJIT_COPY32_TO_F32, SLJIT_FR0, SLJIT_R0);
  sljit_emit_fop1(c, SLJIT_CONV_F64_FROM_F32, SLJIT_FR0, 0, SLJIT_FR0, 0);
  sljit_emit_fcopy(c, SLJIT_COPY_FROM_F64, SLJIT_FR0, SLJIT_R0);
  sljit_emit_op1(c, SLJIT_MOV, SLJIT_MEM1(REG_CPU), fpr_off(fd), SLJIT_R0, 0);
  emit_fs_dirty(t);
  struct sljit_jump *done = sljit_emit_jump(c, SLJIT_JUMP);
  struct sljit_label *slow = sljit_emit_label(c);
  for (int i = 0; i < nfail; i++) sljit_set_label(fail[i], slow);
  emit_helper(t, d);
  sljit_set_label(done, sljit_emit_label(c));
  return true;
}

// kind selects the operation; for FMA, na/nc negate the product and addend.
// Speculative arithmetic (spec_fp): the host operation on the raw operands,
// with the host's flags accrued at exit (see jit_run()). A NaN result is
// replaced by RISC-V's canonical NaN (the host returns its own default NaN or
// propagates an operand's payload). Where the host still differs from
// RISC-V (a rounding mode other than nearest-even in frm, NaN-boxing), the
// slice fails validation and is rolled back.
static void emit_fp_arith_spec(Tc *t, Decode *d, int kind, bool w64, bool na, bool nc,
                               int fd, int f1, int f2) {
  struct sljit_compiler *c = t->c;
  emit_fp_load(t, SLJIT_FR0, f1, w64, kind == FPK_FMA && na);
  emit_fp_load(t, SLJIT_FR1, f2, w64, false);
  sljit_s32 res = SLJIT_FR0;
  if (kind == FPK_FMA) {
    emit_fp_load(t, SLJIT_FR2, d->isa.instr.fp.funct5, w64, nc);
    emit_x86_fma231(c, w64, SLJIT_FR2, SLJIT_FR0, SLJIT_FR1);
    res = SLJIT_FR2;
  } else {
    static const sljit_s32 ops[] = { SLJIT_ADD_F64, SLJIT_SUB_F64, SLJIT_MUL_F64, SLJIT_DIV_F64 };
    sljit_emit_fop2(c, ops[kind] | (w64 ? 0 : SLJIT_32), SLJIT_FR0, 0, SLJIT_FR0, 0, SLJIT_FR1, 0);
  }
  struct sljit_jump *number = sljit_emit_fcmp(c, SLJIT_ORDERED | (w64 ? 0 : SLJIT_32), res, 0, res, 0);
  sljit_emit_op1(c, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, w64 ? (sljit_sw)0x7ff8000000000000ll : 0x7fc00000);
  sljit_emit_fcopy(c, w64 ? SLJIT_COPY_TO_F64 : SLJIT_COPY32_TO_F32, res, SLJIT_R0);
  sljit_set_label(number, sljit_emit_label(c));
  emit_fp_store(t, fd, res, w64);
}

static bool emit_fp_arith(Tc *t, Decode *d, int kind, bool w64, bool na, bool nc) {
  struct sljit_compiler *c = t->c;
  int fd = freg_of(d->dest.preg), f1 = freg_of(d->src1.preg), f2 = freg_of(d->src2.preg);
  int rm = d->isa.instr.fp.rm;
  if (fd < 0 || f1 < 0 || f2 < 0 || (rm != 0 && rm != 7)) return false;
  if (kind == FPK_FMA && !host_has_fma()) return false;
  if ((jit_opt.spec_fp && jit_leader_mode)) {
    emit_fp_arith_spec(t, d, kind, w64, na, nc, fd, f1, f2);
    return true;
  }
  if (t->nfslow == (int)ARRLEN(t->fslow)) return false;
  struct sljit_jump *fail[16];
  int nfail = 0;

  // Inexact already set, and round to nearest even: checked once at block
  // entry (see fp_guard_mask).
  Assert(t->fp_guard & (rm == 7 ? 0xe0 : 0x01), "JIT: floating-point guard missing");

  bool zero_ok = kind == FPK_ADD || kind == FPK_SUB;
  int wide = w64 ? FP_WIN_D : FP_WIN_S;
  int factor = !w64 ? FP_WIN_S : zero_ok ? FP_WIN_D : FP_WIN_D_FACTOR;
  emit_fp_src(t, SLJIT_R0, f1, w64, kind == FPK_FMA && na, fail, &nfail);
  emit_window_check(t, SLJIT_R0, w64, zero_ok, factor, fail, &nfail);
  emit_fp_src(t, SLJIT_R1, f2, w64, false, fail, &nfail);
  emit_window_check(t, SLJIT_R1, w64, zero_ok, factor, fail, &nfail);
  sljit_s32 cp = w64 ? SLJIT_COPY_TO_F64 : SLJIT_COPY32_TO_F32;
  sljit_emit_fcopy(c, cp, SLJIT_FR0, SLJIT_R0);
  sljit_emit_fcopy(c, cp, SLJIT_FR1, SLJIT_R1);
  sljit_s32 res = SLJIT_FR0;
  if (kind == FPK_FMA) {
    int f3 = d->isa.instr.fp.funct5;
    emit_fp_src(t, SLJIT_R0, f3, w64, nc, fail, &nfail);
    emit_window_check(t, SLJIT_R0, w64, true, wide, fail, &nfail);
    sljit_emit_fcopy(c, cp, SLJIT_FR2, SLJIT_R0);
    emit_x86_fma231(c, w64, SLJIT_FR2, SLJIT_FR0, SLJIT_FR1);
    res = SLJIT_FR2;
  } else {
    static const sljit_s32 ops[] = { SLJIT_ADD_F64, SLJIT_SUB_F64, SLJIT_MUL_F64, SLJIT_DIV_F64 };
    sljit_emit_fop2(c, ops[kind] | (w64 ? 0 : SLJIT_32), SLJIT_FR0, 0, SLJIT_FR0, 0, SLJIT_FR1, 0);
  }
  sljit_emit_fcopy(c, w64 ? SLJIT_COPY_FROM_F64 : SLJIT_COPY32_FROM_F32, res, SLJIT_R0);
  if (!w64) sljit_emit_op1(c, SLJIT_MOV_U32, SLJIT_R0, 0, SLJIT_R0, 0);
  if (!w64 && !zero_ok) emit_window_check(t, SLJIT_R0, w64, false, wide, fail, &nfail);
  if (!w64) sljit_emit_op2(c, SLJIT_OR, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, (sljit_sw)0xffffffff00000000ull);
  sljit_emit_op1(c, SLJIT_MOV, SLJIT_MEM1(REG_CPU), fpr_off(fd), SLJIT_R0, 0);
  emit_fs_dirty(t);
  // The fallbacks are emitted after the block body, keeping hot code dense.
  typeof(t->fslow[0]) *e = &t->fslow[t->nfslow++];
  memcpy(e->from, fail, sizeof(fail[0]) * nfail);
  e->nfrom = nfail;
  e->resume = sljit_emit_label(c);
  e->d = d;
  e->zero_path = kind == FPK_MUL || kind == FPK_FMA;
  e->fma = kind == FPK_FMA;
  e->w64 = w64; e->na = na; e->nc = nc; e->fd = fd;
  return true;
}

static void emit_fp_slow_paths(Tc *t) {
  struct sljit_compiler *c = t->c;
  for (int k = 0; k < t->nfslow; k++) {
    typeof(t->fslow[0]) *e = &t->fslow[k];
    struct sljit_label *l = sljit_emit_label(c);
    for (int i = 0; i < e->nfrom; i++) sljit_set_label(e->from[i], l);
    struct sljit_jump *done = NULL;
    if (e->zero_path) done = emit_fp_zero_factor(t, e->d, e->fma, e->w64, e->na, e->nc, e->fd);
    emit_helper(t, e->d);
    struct sljit_jump *back = sljit_emit_jump(c, SLJIT_JUMP);
    sljit_set_label(back, e->resume);
    if (done) sljit_set_label(done, e->resume);
  }
}
#endif

// Straight-line instructions that run through the interpreter's own helper.
// They may raise exceptions and read the instruction count, but never
// transfer control or change the lookup context of decoded blocks.
#define JIT_HELPER_ENTRY(name) [concat(EXEC_ID_, name)] = true,
static const bool helper_ok[TOTAL_INSTR] = {
#ifndef CONFIG_FPU_NONE
  MAP(FLOAT_INSTR_BINARY, JIT_HELPER_ENTRY)
  MAP(FLOAT_INSTR_TERNARY, JIT_HELPER_ENTRY)
#endif
  MAP(AMO_INSTR_TERNARY, JIT_HELPER_ENTRY)
  [EXEC_ID_fence] = true,
};

static void emit_helper(Tc *t, Decode *d) {
  // The helper may write any guest register.
  memset(t->known, 0, sizeof(t->known));
  void *fn = (void *)jit_fp_lookup(d->exec_id);
  if (fn == NULL) fn = (void *)jit_eh_lookup(d->exec_id);
  emit_sync_state(t, d);
  sljit_emit_op1(t->c, SLJIT_MOV_P, SLJIT_R0, 0, SLJIT_IMM, (sljit_sw)d);
  emit_icall(t->c, SLJIT_ARGS1V(P), fn);
}

void *jit_helper_fn(const Decode *d) {
  if (!jit_opt.helpers || !helper_ok[d->exec_id]) return NULL;
  void *fn = (void *)jit_fp_lookup(d->exec_id);
  return fn ? fn : (void *)jit_eh_lookup(d->exec_id);
}

#if defined(CONFIG_FPU_SOFT_HOST_FASTPATH) && !defined(CONFIG_FPU_NONE)
// fcsr bits that must read as 0x01 (inexact set, and frm = RNE when a
// dynamic rounding mode is used) for the inline arithmetic of the block.
static int fp_guard_mask(Tc *t) {
  int mask = 0;
  if (!jit_opt.inline_fp || (jit_opt.spec_fp && jit_leader_mode)) return 0;
  for (Decode *d = t->head; ; d++) {
    switch (d->exec_id) {
      case EXEC_ID_faddd: case EXEC_ID_fsubd: case EXEC_ID_fmuld: case EXEC_ID_fdivd:
      case EXEC_ID_fmaddd: case EXEC_ID_fmsubd: case EXEC_ID_fnmsubd: case EXEC_ID_fnmaddd:
      case EXEC_ID_fadds: case EXEC_ID_fsubs: case EXEC_ID_fmuls: case EXEC_ID_fdivs:
      case EXEC_ID_fmadds: case EXEC_ID_fmsubs: case EXEC_ID_fnmsubs: case EXEC_ID_fnmadds:
        if (d->isa.instr.fp.rm == 0) mask |= 0x01;
        else if (d->isa.instr.fp.rm == 7) mask |= 0xe1;
        break;
    }
    if (d == t->last) break;
  }
  return mask;
}
#endif

// The body of a gate: the region runs as an ordinary function with guest
// state in cpu and the budget in its slot, and returns the exit record.
static void emit_gate_body(Tc *t, const void *region, void *bind) {
  struct sljit_compiler *c = t->c;
  emit_writeback_pinned(c);
  sljit_emit_op1(c, SLJIT_MOV32, SLJIT_MEM0(), (sljit_sw)jit_budget_slot(), REG_BUDGET, 0);
  sljit_emit_op1(c, SLJIT_MOV_P, SLJIT_R0, 0, SLJIT_IMM, (sljit_sw)bind);
  sljit_emit_icall(c, SLJIT_CALL, SLJIT_ARGS1(P, P), SLJIT_IMM, SLJIT_FUNC_ADDR(region));
  emit_reload_pinned(c);
  sljit_emit_op1(c, SLJIT_MOV_S32, REG_BUDGET, 0, SLJIT_MEM0(), (sljit_sw)jit_budget_slot());
  // R0 = the exit record. The gate continues as tier-1 code continues after
  // a block, while the budget lasts: a taken or not-taken exit whose
  // successor is translated through the successor's entry slot, an indirect
  // exit through the indirect-jump cache; anything else leaves.
  struct sljit_jump *stop = sljit_emit_cmp(c, SLJIT_SIG_LESS_EQUAL | SLJIT_32, REG_BUDGET, 0, SLJIT_IMM, 0);
  struct sljit_jump *not_indirect = NULL, *miss = NULL, *stale = NULL;
  if (jit_opt.ibcache) {
    not_indirect = sljit_emit_cmp(c, SLJIT_NOT_EQUAL, SLJIT_MEM1(SLJIT_R0), offsetof(JitExitRec, kind),
                                  SLJIT_IMM, JIT_REC_INDIRECT);
    sljit_emit_op1(c, SLJIT_MOV_P, SLJIT_MEM0(), (sljit_sw)&gate_rec, SLJIT_R0, 0);
    sljit_emit_op1(c, SLJIT_MOV, SLJIT_R0, 0, SLJIT_MEM0(), (sljit_sw)&jit_exit_target);
    emit_ibcache_lookup(c, &miss, &stale);
    struct sljit_label *restore = sljit_emit_label(c);
    sljit_set_label(miss, restore);
    sljit_set_label(stale, restore);
    sljit_emit_op1(c, SLJIT_MOV_P, SLJIT_R0, 0, SLJIT_MEM0(), (sljit_sw)&gate_rec);
    struct sljit_jump *leave = sljit_emit_jump(c, SLJIT_JUMP);
    sljit_set_label(not_indirect, sljit_emit_label(c));
    sljit_emit_op1(c, SLJIT_MOV_P, SLJIT_R1, 0, SLJIT_MEM1(SLJIT_R0), offsetof(JitExitRec, slot));
    struct sljit_jump *no_slot = sljit_emit_cmp(c, SLJIT_EQUAL, SLJIT_R1, 0, SLJIT_IMM, 0);
    sljit_emit_ijump(c, SLJIT_JUMP, SLJIT_MEM1(SLJIT_R1), 0);
    struct sljit_label *out = sljit_emit_label(c);
    sljit_set_label(no_slot, out);
    sljit_set_label(leave, out);
    sljit_set_label(stop, out);
  } else {
    sljit_emit_op1(c, SLJIT_MOV_P, SLJIT_R1, 0, SLJIT_MEM1(SLJIT_R0), offsetof(JitExitRec, slot));
    struct sljit_jump *no_slot = sljit_emit_cmp(c, SLJIT_EQUAL, SLJIT_R1, 0, SLJIT_IMM, 0);
    sljit_emit_ijump(c, SLJIT_JUMP, SLJIT_MEM1(SLJIT_R1), 0);
    struct sljit_label *out = sljit_emit_label(c);
    sljit_set_label(no_slot, out);
    sljit_set_label(stop, out);
  }
  sljit_emit_ijump(c, SLJIT_JUMP, SLJIT_IMM, common_exit);
}

static bool translate(Decode *head, JitTrans *tr, const void *region, void *bind);

bool jit_translate_block(Decode *head, JitTrans *tr) { return translate(head, tr, NULL, NULL); }

bool jit_translate_gate(Decode *head, JitTrans *tr, const void *region, void *bind) {
  tr->gate = true;
  return translate(head, tr, region, bind);
}

static bool translate(Decode *head, JitTrans *tr, const void *region, void *bind) {
  static Tc tc;
  Tc *t = &tc;
  memset(t, 0, offsetof(Tc, slow));
  t->nfslow = 0;
  t->head = head;
  t->tr = tr;
  tr->head = head;
  t->c = sljit_create_compiler(NULL);
  if (t->c == NULL) return false;
  struct sljit_compiler *c = t->c;

  sljit_set_context(c, 0, JIT_ARGS, JIT_SCRATCHES, JIT_SAVEDS, JIT_LOCALS);

  Decode *d = head;
  while (!is_terminator(d)) d++;
  t->last = d;

  struct sljit_label *entry = sljit_emit_label(c);
#ifdef CONFIG_TCACHE_KEEP_ON_TRAP
  {
    // The first entry in an epoch of the reference block cache is charged to
    // its shadow, out of line (see the end of the translation).
    sljit_s32 epoch, stamp; sljit_sw epochw, stampw;
    static_mem(&tcache_uts_epoch, &epoch, &epochw);
    static_mem(&jit_uts_stamp[tr->id], &stamp, &stampw);
    sljit_emit_op1(c, SLJIT_MOV_U32, SLJIT_R0, 0, epoch, epochw);
    t->uts_miss = sljit_emit_cmp(c, SLJIT_NOT_EQUAL | SLJIT_32, SLJIT_R0, 0, stamp, stampw);
    t->uts_body = sljit_emit_label(c);
  }
#endif
  IFDEF(CONFIG_FPU_SOFT_HOST_FASTPATH, t->fp_guard = fp_guard_mask(t));
  if (t->fp_guard) {
    // Floating-point state bits that only change through CSR writes (never
    // translated) or by being set: checked once for the whole block.
    sljit_emit_op1(c, SLJIT_MOV, SLJIT_R0, 0, SLJIT_MEM0(), (sljit_sw)&csr_array[0x003]);
    sljit_emit_op2(c, SLJIT_AND, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, t->fp_guard);
    t->fp_fallback = sljit_emit_cmp(c, SLJIT_NOT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, 1);
  }

  int n = 0;
  bool complete = region != NULL;
  struct sljit_label **chunk_label = NULL;
  tr->nchunks = 0;
  if (region == NULL && MUXDEF(CONFIG_TCACHE_KEEP_ON_TRAP, true, false) && t->last - head + 1 > JIT_CHUNK_MIN) {
    tr->nchunks = (t->last - head) / JIT_CHUNK_LEN;
    chunk_label = calloc(tr->nchunks + 1, sizeof(*chunk_label));
    tr->chunk = calloc(tr->nchunks + 1, sizeof(*tr->chunk));
    tr->xexits = calloc(tr->nchunks + 1, sizeof(*tr->xexits));
  }
  if (region) emit_gate_body(t, region, bind);
  else for (d = head; ; d++) {
    if (chunk_label && (d - head) % JIT_CHUNK_LEN == 0) emit_chunk_entry(t, d, chunk_label);
    if (!emit_instr(t, d)) {
      if (is_terminator(d) || !jit_opt.helpers || !helper_ok[d->exec_id]) break;
      emit_helper(t, d);
      jit_stats.helper_instrs++;
    }
    n++;
    if (d == t->last) { complete = true; break; }
  }

  if (!complete) {
    if (n < jit_opt.min_prefix) {
      jit_stats.refused_by_exec_id[d->exec_id % ARRLEN(jit_stats.refused_by_exec_id)]++;
      sljit_free_compiler(c);
      free(chunk_label);
      jit_free_chunks(tr);
      return false;
    }
    jit_stats.prefix_by_exec_id[d->exec_id % ARRLEN(jit_stats.prefix_by_exec_id)]++;
    // The prefix ends before d; the interpreter resumes there.
    emit_exit(t, JIT_REC_MIDBLOCK, NULL, d);
  }

  emit_slow_paths(t);
  IFDEF(CONFIG_FPU_SOFT_HOST_FASTPATH, emit_fp_slow_paths(t));
#ifdef CONFIG_TCACHE_KEEP_ON_TRAP
  // The block head is charged by the interpreter's tcache_uts_enter(), which
  // updates tcache_uts_tc, the pc/head epochs and the pool and inserts the
  // block through uts_insert(). Charging the head here would miss that insert
  // and make the reference's block-list capacity accounting diverge, landing
  // the kept-cache stop at a different instruction, so the entry always calls
  // the interpreter's helper.
  {
    sljit_set_label(t->uts_miss, sljit_emit_label(c));
    sljit_emit_op1(c, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, tr->id);
    emit_icall_light(c, SLJIT_ARGS1(W, 32), (void *)jit_uts_miss_slow);
    sljit_emit_ijump(c, SLJIT_JUMP, SLJIT_R0, 0);
  }
#endif
  // Leaves for the interpreter at the block head: the floating-point guard
  // failed, or the reference runs out of capacity inside the block.
  if (ISDEF(CONFIG_TCACHE_KEEP_ON_TRAP) || t->fp_fallback != NULL) {
    t->uts_fallback = sljit_emit_label(c);
    if (t->fp_fallback) sljit_set_label(t->fp_fallback, t->uts_fallback);
    emit_exit(t, JIT_REC_FALLBACK, NULL, head);
  }

  void *code = sljit_generate_code(c, 0, NULL);
  if (code == NULL) {
    sljit_free_compiler(c);
    free(chunk_label);
    jit_free_chunks(tr);
    return false;
  }
  tr->code = code;
  tr->entry = sljit_get_label_addr(entry);
  tr->exec_offset = sljit_get_executable_offset(c);
  tr->complete = complete;
  tr->code_size = sljit_get_generated_code_size(c);
  for (int k = 0; k < 2; k++) {
    tr->chain_addr[k] = t->chain[k] ? sljit_get_jump_addr(t->chain[k]) : 0;
#ifdef JIT_SLOT_CHAIN
    tr->chain_disp[k] = 0;
    if (t->chain_after[k] != NULL) {
      tr->chain_disp[k] = sljit_get_label_addr(t->chain_after[k]) - 4;
      jit_edge_slot[2 * tr->id + k] = sljit_get_label_addr(t->chain_exit[k]);
    }
#endif
  }
#ifdef CONFIG_TCACHE_KEEP_ON_TRAP
  tr->body = sljit_get_label_addr(t->uts_body);
  if (chunk_label != NULL) {
    // Entries only where the whole block is translated.
    if (!complete) tr->nchunks = 0;
    for (int j = 1; j <= tr->nchunks; j++) tr->chunk[j] = sljit_get_label_addr(chunk_label[j]);
    tr->chunk[0] = tr->body;
    free(chunk_label);
  }
  tr->fallback = sljit_get_label_addr(t->uts_fallback);
#endif
  jit_stats.code_bytes += sljit_get_generated_code_size(c);
  sljit_free_compiler(c);
  return true;
}

#define JIT_NAME_ENTRY(name) [concat(EXEC_ID_, name)] = #name,
static const char *exec_names[TOTAL_INSTR] = { MAP(INSTR_LIST, JIT_NAME_ENTRY) };

const char *jit_exec_name(int id) {
  return (id >= 0 && id < TOTAL_INSTR && exec_names[id]) ? exec_names[id] : "?";
}

jit_trampoline_fn jit_build_trampoline(void) {
  struct sljit_compiler *c = sljit_create_compiler(NULL);
  Assert(c, "JIT: cannot create compiler for the trampoline");
  sljit_emit_enter(c, 0, JIT_ARGS, JIT_SCRATCHES, JIT_SAVEDS, JIT_LOCALS);
  // Translated memory accesses assume identity effective addresses and
  // translated data accesses. Any chain entered here keeps that state,
  // because instructions that change it are never translated.
  sljit_emit_op1(c, SLJIT_MOV_U8, SLJIT_R1, 0, SLJIT_MEM0(), (sljit_sw)jit_mmu_identity_fast_slot());
  struct sljit_jump *j1 = sljit_emit_cmp(c, SLJIT_EQUAL, SLJIT_R1, 0, SLJIT_IMM, 0);
  struct sljit_jump *j2 = sljit_emit_cmp(c, SLJIT_NOT_EQUAL | SLJIT_32,
      SLJIT_MEM0(), (sljit_sw)jit_data_mmu_state_slot(), SLJIT_IMM, MMU_TRANSLATE);
  sljit_emit_op1(c, SLJIT_MOV, REG_RTLB, 0, SLJIT_IMM, (sljit_sw)jit_hosttlb_base());
  // R0 holds the entry and is not a pinned host register.
  emit_reload_pinned(c);
  sljit_emit_ijump(c, SLJIT_JUMP, SLJIT_R0, 0);
  struct sljit_label *fail = sljit_emit_label(c);
  sljit_set_label(j1, fail);
  sljit_set_label(j2, fail);
  sljit_emit_op1(c, SLJIT_MOV_P, SLJIT_MEM0(), (sljit_sw)&jit_exit_ptr, SLJIT_IMM, (sljit_sw)&fallback_exit);
  sljit_emit_return(c, SLJIT_MOV_S32, REG_BUDGET, 0);
  // Shared exit: R0 points to the exit record.
  struct sljit_label *exit = sljit_emit_label(c);
  emit_writeback_pinned(c);
  sljit_emit_op1(c, SLJIT_MOV_P, SLJIT_MEM0(), (sljit_sw)&jit_exit_ptr, SLJIT_R0, 0);
  sljit_emit_return(c, SLJIT_MOV_S32, REG_BUDGET, 0);
  void *code = sljit_generate_code(c, 0, NULL);
  Assert(code, "JIT: trampoline generation failed");
  common_exit = sljit_get_label_addr(exit);
  sljit_free_compiler(c);
  return (jit_trampoline_fn)code;
}

void jit_free_code(void *code) {
  sljit_free_code(code, NULL);
}
