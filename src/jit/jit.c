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

// Glue between the interpreter and translated code: block heat, translation
// installation, dispatch and statistics.

#include "jit-internal.h"
#include <rtl/fp.h>
#include <string.h>
#include <time.h>

bool jit_leader_mode = false;
void jit_set_leader_mode(bool on) { jit_leader_mode = on; }

JitOptions jit_opt = {
  .enabled = true,
  .threshold = 16,
  .persistent = false,
  .chain = true,
  .ibcache = true,
  .ib_bits = 20,
  .pin = 4,
  .mem = true,
  .helpers = true,
  .inline_fp = true,
  .spec_fp = true,
  .min_prefix = 2,
  .tier2 = ISDEF(CONFIG_JIT_TIER2),
  .t2_samples = 16,
  .t2_residency = 256,
  .t2_blocks = 32,
  .t2_instrs = 4096,
  .t2_opt = 2,
  .t2_cg = 2,
  .t2_profile = true,
  .t2_groups = true,
  .t2_units = true,
  .t2_min_instrs = 32,
  .t2_utlb = true,
  .t2_async = true,
  .t2_servers = 4,
};

JitStats jit_stats;
static bool jit_configured = true;   // jit_opt.enabled as configured

static const void *label_enter;
static const void *label_decode;

typedef sljit_sw (SLJIT_FUNC *jit_add_fn)(sljit_sw a, sljit_sw b);

// Generate f(a, b) = a + b and check it, so a broken backend or a host that
// refuses executable memory is reported at start-up.
static void jit_selftest(void) {
  struct sljit_compiler *c = sljit_create_compiler(NULL);
  Assert(c != NULL, "JIT: cannot create SLJIT compiler");
  sljit_emit_enter(c, 0, SLJIT_ARGS2(W, W, W), 1, 2, 0);
  sljit_emit_op2(c, SLJIT_ADD, SLJIT_R0, 0, SLJIT_S0, 0, SLJIT_S1, 0);
  sljit_emit_return(c, SLJIT_MOV, SLJIT_R0, 0);
  void *code = sljit_generate_code(c, 0, NULL);
  Assert(code != NULL, "JIT: SLJIT code generation failed (error %d)",
         sljit_get_compiler_error(c));
  sljit_free_compiler(c);

  sljit_sw r = ((jit_add_fn)code)(40, 2);
  Assert(r == 42, "JIT: self-test returned %ld", (long)r);
  sljit_free_code(code, NULL);
  Log("JIT: SLJIT backend ready on %s", sljit_get_platform_name());
}

static bool parse_bool(const char *v) {
  return strcmp(v, "1") == 0 || strcmp(v, "on") == 0 || strcmp(v, "true") == 0;
}

// Options are a comma-separated list of key=value pairs.
void jit_parse_options(const char *opts) {
  char *buf = strdup(opts);
  for (char *tok = strtok(buf, ","); tok != NULL; tok = strtok(NULL, ",")) {
    char *eq = strchr(tok, '=');
    Assert(eq != NULL, "JIT option '%s' is not key=value", tok);
    *eq = '\0';
    const char *k = tok, *v = eq + 1;
    if (strcmp(k, "enable") == 0) jit_opt.enabled = parse_bool(v);
    else if (strcmp(k, "threshold") == 0) jit_opt.threshold = atoi(v);
    else if (strcmp(k, "lifetime") == 0) {
      Assert(strcmp(v, "tied") == 0 || strcmp(v, "persistent") == 0,
             "JIT lifetime must be tied or persistent");
      jit_opt.persistent = strcmp(v, "persistent") == 0;
    }
    else if (strcmp(k, "chain") == 0) jit_opt.chain = parse_bool(v);
    else if (strcmp(k, "ibcache") == 0) jit_opt.ibcache = parse_bool(v);
    else if (strcmp(k, "ib-bits") == 0) jit_opt.ib_bits = atoi(v);
    else if (strcmp(k, "pin") == 0) jit_opt.pin = atoi(v);
    else if (strcmp(k, "mem") == 0) jit_opt.mem = parse_bool(v);
    else if (strcmp(k, "helpers") == 0) jit_opt.helpers = parse_bool(v);
    else if (strcmp(k, "inline-fp") == 0) jit_opt.inline_fp = parse_bool(v);
    else if (strcmp(k, "spec-fp") == 0) jit_opt.spec_fp = parse_bool(v);
    else if (strcmp(k, "tier2") == 0) jit_opt.tier2 = parse_bool(v);
    else panic("unknown JIT option '%s'", k);
  }
  free(buf);
  Assert(jit_opt.threshold >= 1, "JIT threshold must be at least 1");
  Assert(jit_opt.t2_samples >= 1, "JIT t2-samples must be at least 1");
}

static jit_trampoline_fn trampoline;
JitIbEntry *jit_ibcache;
_Static_assert(sizeof(JitIbEntry) == (1 << JIT_IBCACHE_SHIFT), "translated code indexes the cache by shift");
sljit_uw jit_ctx_gen = 1, jit_ctx_mix;
// Entries filled since the cache was last cleared, so that clearing a large
// cache costs what was used of it.
static uint32_t *ib_used;
static uint32_t ib_nused;

// The indirect-jump cache is keyed by the lookup context of decoded blocks,
// so entries stay valid across changes that do not alter it (for example
// toggling SUM around user-memory accesses).
void jit_on_context_change(void) {
#ifdef CONFIG_TCACHE_KEEP_ON_TRAP
  uint64_t isa_tcache_context(void);
  jit_ctx_gen = isa_tcache_context() | 1;
#else
  jit_ctx_gen++;
#endif
  jit_ctx_mix = (jit_ctx_gen * 0x9e3779b97f4a7c15ull) >> (64 - jit_opt.ib_bits);
}

static void ibcache_clear(void) {
  // A zero pc never matches: guest code is never executed at address 0.
  for (uint32_t i = 0; i < ib_nused; i++) jit_ibcache[ib_used[i]].pc = 0;
  ib_nused = 0;
}
static JitTrans *trans_list;   // every live translation, for bulk release

void jit_init(void) {
  jit_configured = jit_opt.enabled;
  jit_selftest();
  jit_pin_init(jit_opt.pin);
  Assert(jit_opt.ib_bits >= 8 && jit_opt.ib_bits <= 24, "JIT ib-bits must be between 8 and 24");
  jit_ibcache = calloc(1ul << jit_opt.ib_bits, sizeof(JitIbEntry));
  ib_used = malloc(sizeof(*ib_used) << jit_opt.ib_bits);
  Assert(jit_ibcache && ib_used, "JIT: cannot allocate the indirect-jump cache");
  jit_on_context_change();
  trampoline = jit_build_trampoline();
#ifdef CONFIG_JIT_TIER2
  if (jit_opt.tier2) jit_t2_init();
#else
  jit_opt.tier2 = false;
#endif
  Log("JIT: tcache entry size %zu bytes", sizeof(Decode));
  Log("JIT: %s, threshold %d, lifetime %s, chain %d, ibcache %d, pin %d, mem %d",
      jit_opt.enabled ? "enabled" : "disabled", jit_opt.threshold,
      jit_opt.persistent ? "persistent" : "tied", jit_opt.chain,
      jit_opt.ibcache, jit_opt.pin, jit_opt.mem);}

void jit_set_labels(const void *enter_label, const void *decode_label) {
  label_enter = enter_label;
  label_decode = decode_label;
}

void jit_decode_reset(Decode *s) {
  s->jit_state = JIT_NONE;
  s->jit_heat = 0;
  s->jit_code = NULL;
  s->jit_saved_helper = NULL;
}

static uint64_t flush_reasons[JIT_FLUSH_NR];

void jit_note_flush_reason(int reason) {
  flush_reasons[reason]++;
}

static uint32_t trans_count;   // translations alive; they are released together
uint32_t jit_uts_stamp[JIT_MAX_TRANS];
sljit_uw jit_uts_entry[JIT_MAX_TRANS];
uint32_t jit_uts_accounted[JIT_MAX_TRANS];
uint32_t jit_uts_naccounted;
sljit_uw jit_uts_checked[JIT_MAX_TRANS];
static JitTrans *trans_by_id[JIT_MAX_TRANS];
#ifdef JIT_SLOT_CHAIN
sljit_uw jit_edge_slot[2 * JIT_MAX_TRANS];
#endif

// A new epoch of the reference block cache: every block is unaccounted.
void jit_uts_new_epoch(void) {
  for (uint32_t i = 0; i < jit_uts_naccounted; i++) {
    uint32_t id = jit_uts_accounted[i];
    jit_uts_entry[id] = jit_uts_checked[id];
  }
  jit_uts_naccounted = 0;
}

void jit_uts_clear_stamps(void) { memset(jit_uts_stamp, 0, sizeof(jit_uts_stamp)); }

// Release every translation. With restore_heads, the decoded blocks stay in
// use and their heads return to interpretation.
static void discard_translations(bool restore_heads) {
  IFDEF(CONFIG_JIT_TIER2, jit_t2_discard());
  ibcache_clear();
  trans_count = 0;
  jit_uts_naccounted = 0;
  while (trans_list != NULL) {
    JitTrans *tr = trans_list;
    trans_list = tr->next_alloc;
    // A superseded translation shares its head with the gate that replaced it.
    if (restore_heads && !tr->superseded) {
#ifdef CONFIG_TCACHE_KEEP_ON_TRAP
      tcache_set_helper(tr->head, tr->head->jit_saved_helper);
#else
      tr->head->EHelper = tr->head->jit_saved_helper;
#endif
      jit_decode_reset(tr->head);
    }
    for (int j = 1; restore_heads && j <= tr->nchunks; j++) {
      Decode *d = tr->head + j * JIT_CHUNK_LEN;
      if (d->jit_state != JIT_CHUNK || d->jit_code != tr) continue;
      IFDEF(CONFIG_TCACHE_KEEP_ON_TRAP, tcache_set_helper(d, d->jit_saved_helper));
      jit_decode_reset(d);
    }
    jit_free_chunks(tr);
    jit_free_code(tr->code);
    free(tr);
    jit_stats.discarded++;
  }
}

void jit_resume(void) {
  // The JIT charges the kept block cache's shadow; under the exact cache
  // (profiling / leader-follower) that shadow is absent, so translated and
  // interpreted counting diverge. Stay in the interpreter there.
  if (tcache_is_exact()) return;
  jit_opt.enabled = jit_configured;
}
void jit_after_rollback(void) { jit_opt.t2_canon_nan = true; }

void jit_stop(void) {
  if (!jit_opt.enabled && trans_list == NULL) return;
  jit_opt.enabled = false;
  discard_translations(true);
  // A follower drops the leader's compile process and pending work.
  IFDEF(CONFIG_JIT_TIER2, jit_t2_stop());
}

#ifdef CONFIG_TCACHE_KEEP_ON_TRAP
// The block's miss path could not charge it inline: the reference may run
// out of capacity in it. Returns where to continue: the block body, or the
// exit to the interpreter when a stop was placed in the block.
bool jit_block_moved;

sljit_uw jit_uts_miss_slow(uint32_t id) {
  JitTrans *tr = trans_by_id[id];
  // The block's code moved: the interpreter discards the blocks (see
  // tcache_block_invalid()) once translated code has left.
  if (!tcache_block_valid(tr->head)) {
    jit_block_moved = true;
    return tr->fallback;
  }
  // With a stop placed, a long block runs up to the chunk of the stop.
  if (tcache_uts_enter(tr->head)) return tr->nchunks ? tr->body : tr->fallback;
  jit_uts_stamp[id] = tcache_uts_epoch;
  jit_uts_entry[id] = tr->body;
  jit_uts_accounted[jit_uts_naccounted++] = id;
  return tr->body;
}
#endif

// Translations refer to tcache entries, so they die with them.
void jit_on_tcache_flush(void) {
  jit_stats.tcache_flushes++;
  discard_translations(false);
}

// A block is translatable once the interpreter has decoded every instruction
// up to and including its terminator.
static bool block_fully_decoded(const Decode *head) {
  const Decode *d = head;
  while (true) {
    if (d->EHelper == label_decode) return false;
    if (d->type != INSTR_TYPE_N) return true;
    d++;
  }
}

static uint64_t now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000000000ull + ts.tv_nsec;
}

static bool jit_translate(Decode *head) {
  if (trans_count == JIT_MAX_TRANS) return false;
  JitTrans *tr = calloc(1, sizeof(*tr));
  tr->id = trans_count;
  uint64_t t0 = now_ns();
  bool ok = jit_translate_block(head, tr);
  jit_stats.translate_ns += now_ns() - t0;
  if (!ok) {
    free(tr);
    return false;
  }
  tr->next_alloc = trans_list;
  trans_list = tr;
  trans_count++;
  trans_by_id[tr->id] = tr;
  IFDEF(CONFIG_JIT_TIER2, if (jit_opt.tier2) jit_t2_note_trans(tr));
  jit_uts_stamp[tr->id] = 0;   // epochs start at 1
  jit_uts_entry[tr->id] = jit_uts_checked[tr->id] = tr->entry;
  head->jit_code = tr;
  return true;
}

JitTrans *jit_trans_of(uint32_t id) { return id < trans_count ? trans_by_id[id] : NULL; }

// cur takes over the id, the head and every way into the other translation
// of the head: chained jumps read the id's entry slot, the interpreter and
// the trampoline the head's code, and the indirect-jump cache is refilled.
static void point_at(JitTrans *cur, JitTrans *other) {
  uint32_t id = cur->id;
  other->superseded = true;
  cur->superseded = false;
  trans_by_id[id] = cur;
  cur->head->jit_code = cur;
  jit_uts_checked[id] = cur->entry;
  jit_uts_entry[id] = jit_uts_stamp[id] == tcache_uts_epoch ? cur->body : cur->entry;
  ibcache_clear();
}

void jit_install_gate(JitTrans *old, JitTrans *gate) {
  gate->next_alloc = trans_list;
  trans_list = gate;
  point_at(gate, old);
}

void jit_reinstall_gate(JitTrans *old, JitTrans *gate) { point_at(gate, old); }

// The old translation takes its id and head back from the gate.
void jit_uninstall_gate(JitTrans *old, JitTrans *gate) { point_at(old, gate); }

void jit_observe_block_head(Decode *s) {
  if (!jit_opt.enabled || s->jit_state != JIT_NONE) return;
  if (s->EHelper == label_decode) return;   // placeholder, not decoded
  if (++s->jit_heat < jit_opt.threshold) return;
  if (!block_fully_decoded(s)) return;
  if (jit_translate(s)) {
    s->jit_saved_helper = s->EHelper;
    s->EHelper = label_enter;
    s->jit_state = JIT_ACTIVE;
    jit_stats.translated_blocks++;
    JitTrans *tr = s->jit_code;
    for (int j = 1; j <= tr->nchunks; j++) {
      Decode *d = s + j * JIT_CHUNK_LEN;
      d->jit_saved_helper = (void *)MUXDEF(CONFIG_TCACHE_KEEP_ON_TRAP, tcache_get_helper(d), d->EHelper);
      IFDEF(CONFIG_TCACHE_KEEP_ON_TRAP, tcache_set_helper(d, label_enter));
      d->jit_state = JIT_CHUNK;
      d->jit_code = tr;
    }
  } else {
    s->jit_state = JIT_REFUSED;
    jit_stats.refused_blocks++;
  }
}

#ifdef CONFIG_TCACHE_KEEP_ON_TRAP
bool jit_may_enter_shadowed(Decode *s) {
  JitTrans *tr = s->jit_code;
  if (tr == NULL || tr->nchunks == 0) return false;
  Decode *last = tr->head + tr->head->bb_len - 1;
  if (tcache_uts_rem_active &&
      (s->jit_state != JIT_CHUNK || s < tcache_uts_rem_first || s > tcache_uts_rem_last)) return false;
  // A stop must be ahead of s in the same block.
  if (tcache_uts_stop_at != NULL && (tcache_uts_stop_at < s || tcache_uts_stop_at > last)) return false;
  return true;
}
#endif

// Once the successor of a direct exit is translated, jump to it directly.
// Placeholders are never chained past: the interpreter consumes them, so
// tcache metadata evolves exactly as without translation.
static void try_chain(JitTrans *from, int which, Decode *next) {
  if (!jit_opt.chain || from->chained[which]) return;
  if (from->chain_addr[which] == 0 && from->chain_disp[which] == 0) return;
  if (next->jit_state != JIT_ACTIVE) return;
  JitTrans *to = next->jit_code;
#ifdef JIT_SLOT_CHAIN
  if (from->chain_disp[which] != 0) {
    // The jump reads the successor's entry slot from now on.
    sljit_sw disp;
    bool ok = cpu_offset(&jit_uts_entry[to->id], &disp);
    Assert(ok, "JIT: entry slot out of reach");
    *(int32_t *)from->chain_disp[which] = (int32_t)disp;
    from->chained[which] = true;
    jit_stats.chains++;
    return;
  }
#endif
  sljit_set_jump_addr(from->chain_addr[which], to->entry, from->exec_offset);
  from->chained[which] = true;
  jit_stats.chains++;
}

// Translated floating-point code (spec_fp) leaves its exception flags in the
// host's sticky flags; they are accrued into fflags at every exit.
#if defined(__x86_64__)
#include <xmmintrin.h>
static inline void host_fp_flags_clear(void) { _mm_setcsr(_mm_getcsr() & ~0x3fu); }
static inline void host_fp_flags_accrue(void) {
  uint32_t m = _mm_getcsr();
  if ((m & 0x3d) == 0) return;   // no flag but denormal-operand
  void isa_fp_set_ex(uint32_t ex);
  isa_fp_set_ex(((m & 0x01) ? FPCALL_EX_NV : 0) | ((m & 0x04) ? FPCALL_EX_DZ : 0) |
                ((m & 0x08) ? FPCALL_EX_OF : 0) | ((m & 0x10) ? FPCALL_EX_UF : 0) |
                ((m & 0x20) ? FPCALL_EX_NX : 0));
  _mm_setcsr(m & ~0x3fu);
}
#else
#error "spec_fp needs the host floating-point status for this architecture"
#endif

JitExit jit_run(Decode *s) {
  jit_stats.entries++;
  JitTrans *tr = s->jit_code;
  if (tr == NULL) {
    jit_stats.exits_fallback++;
    return (JitExit){ .kind = JIT_EXIT_FALLBACK, .next = s };
  }
  int *budget = jit_budget_slot();
  sljit_uw entry = s->jit_state == JIT_CHUNK ? tr->chunk[(s - tr->head) / JIT_CHUNK_LEN] : tr->entry;
  if (jit_opt.spec_fp && jit_leader_mode) host_fp_flags_clear();
  *budget = (int)trampoline(entry, &cpu, *budget);
  if (jit_opt.spec_fp && jit_leader_mode) host_fp_flags_accrue();
#ifdef CONFIG_JIT_TIER2
  if (jit_opt.tier2 && *budget <= 0 && jit_exit_ptr->trans != NULL) jit_t2_sample(jit_exit_ptr->trans);
#endif

  const JitExitRec *r = jit_exit_ptr;
  switch (r->kind) {
    case JIT_REC_FALLBACK:
      jit_stats.exits_fallback++;
      // A block reached by chaining is entered from a block end, where the
      // interpreter points prev_s at the new head.
      if (r->next) *jit_prev_s_slot() = r->next;
      return (JitExit){ .kind = JIT_EXIT_FALLBACK, .next = r->next ? r->next : s };
    case JIT_REC_MIDBLOCK:
      jit_stats.exits_midblock++;
      jit_stats.guest_instrs += r->next->idx_in_bb - 1;
      return (JitExit){ .kind = JIT_EXIT_MIDBLOCK, .next = r->next };
    case JIT_REC_TAKEN:
    case JIT_REC_NTAKEN: {
      jit_stats.exits_block_end++;
      jit_stats.guest_instrs += r->last->idx_in_bb;
      int which = r->kind == JIT_REC_TAKEN ? 0 : 1;
      Decode *next = which == 0 ? r->last->tnext : r->last->ntnext;
      if (r->region) {
        // The gate chains through the successor's entry slot from now on
        // (never past a placeholder, as try_chain()).
        if (jit_opt.chain && r->slot == NULL && next->jit_state == JIT_ACTIVE) {
          ((JitExitRec *)r)->slot = &jit_uts_entry[((JitTrans *)next->jit_code)->id];
          jit_stats.chains++;
        }
      } else {
        try_chain(r->trans, which, next);
      }
      return (JitExit){ .kind = JIT_EXIT_BLOCK_END, .next = next, .last = r->last };
    }
    case JIT_REC_INDIRECT: {
      jit_stats.exits_indirect++;
      jit_stats.guest_instrs += r->last->idx_in_bb;
      Decode *next = jit_jr_fetch(r->last, jit_exit_target);
      // Only translated heads are cached; a hit then skips the interpreter's
      // fetch, which would find the same existing head without allocating.
      if (jit_opt.ibcache && next->jit_state == JIT_ACTIVE) {
        sljit_uw k = jit_ib_index(jit_exit_target);
        JitIbEntry *e = &jit_ibcache[k];
        if (e->pc == 0) { jit_stats.ib_empty++; ib_used[ib_nused++] = k; }
        else if (e->pc != jit_exit_target) jit_stats.ib_conflict++;
        else if (e->gen != jit_ctx_gen) jit_stats.ib_stale++;
        else jit_stats.ib_other++;
        e->pc = jit_exit_target;
        e->entry = ((JitTrans *)next->jit_code)->entry;
        e->gen = jit_ctx_gen;
        jit_stats.ibcache_fills++;
      }
      return (JitExit){ .kind = JIT_EXIT_BLOCK_END, .next = next, .last = r->last };
    }
    default:
      panic("JIT: bad exit kind %ld", (long)r->kind);
  }
}

static void print_top(const char *what, const uint64_t *counts) {
  enum { N = ARRLEN(jit_stats.prefix_by_exec_id), TOP = 12 };
  bool used[N] = {};
  char line[1024];
  int len = snprintf(line, sizeof(line), "JIT: %s:", what);
  for (int k = 0; k < TOP; k++) {
    int best = -1;
    for (int i = 0; i < N; i++) {
      if (!used[i] && counts[i] && (best < 0 || counts[i] > counts[best])) best = i;
    }
    if (best < 0) break;
    used[best] = true;
    len += snprintf(line + len, sizeof(line) - len, " %s=%lu", jit_exec_name(best), counts[best]);
  }
  Log("%s", line);
}

void jit_report(void) {
  Log("JIT: translated %lu, refused %lu, discarded %lu, entries %lu, tcache flushes %lu",
      jit_stats.translated_blocks, jit_stats.refused_blocks, jit_stats.discarded,
      jit_stats.entries, jit_stats.tcache_flushes);
  Log("JIT: exits fallback %lu, midblock %lu, block-end %lu, indirect %lu, chains %lu, ibcache fills %lu",
      jit_stats.exits_fallback, jit_stats.exits_midblock, jit_stats.exits_block_end,
      jit_stats.exits_indirect, jit_stats.chains, jit_stats.ibcache_fills);
  Log("JIT: indirect-cache refills: empty %lu, conflict %lu, stale %lu, other %lu",
      jit_stats.ib_empty, jit_stats.ib_conflict, jit_stats.ib_stale, jit_stats.ib_other);
  Log("JIT: translation time %lu us, code %lu bytes, helper instructions translated %lu",
      jit_stats.translate_ns / 1000, jit_stats.code_bytes, jit_stats.helper_instrs);
  Log("JIT: flush requests: satp %lu, sfence.vma %lu, fence.i %lu, tcache full %lu",
      flush_reasons[JIT_FLUSH_SATP], flush_reasons[JIT_FLUSH_SFENCE],
      flush_reasons[JIT_FLUSH_FENCE_I], flush_reasons[JIT_FLUSH_FULL]);
  print_top("instructions ending a prefix", jit_stats.prefix_by_exec_id);
  print_top("instructions refusing a block", jit_stats.refused_by_exec_id);
  IFDEF(CONFIG_JIT_TIER2, if (jit_opt.tier2) jit_t2_report());
}
