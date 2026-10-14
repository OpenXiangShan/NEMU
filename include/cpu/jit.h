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

#ifndef __CPU_JIT_H__
#define __CPU_JIT_H__

#include <common.h>

#ifdef CONFIG_JIT_SLJIT

struct Decode;

// JIT_CHUNK: an instruction inside a long translated block where translated
// execution may also start (see jit-translate.c).
enum { JIT_NONE = 0, JIT_ACTIVE, JIT_REFUSED, JIT_CHUNK };

enum {
  // Blocks before `next` may have been executed (chained); nothing of the
  // translated block headed by `next` was. The interpreter runs `next`.
  JIT_EXIT_FALLBACK,
  // A prefix of a block was executed; the interpreter resumes at `next`,
  // which is the first unexecuted instruction of the same block. The batch
  // budget is unchanged, exactly as inside an interpreted block.
  JIT_EXIT_MIDBLOCK,
  // Whole blocks were executed and their lengths were subtracted from the
  // batch budget. `last` is the terminator executed last and `next` is its
  // successor, as the interpreter's jump helpers would have chosen it.
  JIT_EXIT_BLOCK_END,
};

typedef struct {
  int kind;
  struct Decode *next;
  struct Decode *last;
} JitExit;

void jit_init(void);
void jit_report(void);
void jit_parse_options(const char *opts);

// Hooks used by the interpreter.
void jit_set_labels(const void *enter_label, const void *decode_label);
void jit_decode_reset(struct Decode *s);
void jit_on_tcache_flush(void);
// A new epoch of the reference-capacity shadow; its epoch numbers wrapped.
void jit_uts_new_epoch(void);
void jit_uts_clear_stamps(void);
void jit_observe_block_head(struct Decode *s);
JitExit jit_run(struct Decode *s);
// Whether translated execution may start at s now, a block head or a chunk
// entry, while the reference-capacity shadow has a stop or a remainder.
bool jit_may_enter_shadowed(struct Decode *s);
// Set when translated code left a block whose code moved (its mappings
// changed); the interpreter discards the blocks then.
extern bool jit_block_moved;

// Reasons the block cache (and with it every translation) is discarded.
enum { JIT_FLUSH_SATP, JIT_FLUSH_SFENCE, JIT_FLUSH_FENCE_I, JIT_FLUSH_FULL, JIT_FLUSH_NR };
void jit_note_flush_reason(int reason);

// Privilege mode, MMU state or the address space may have changed.
void jit_on_context_change(void);

// Disables translation and returns every translated block to the interpreter
// (translated code does not publish block transitions for profiling).
void jit_stop(void);
void jit_set_leader_mode(bool on);
// Translation as configured again, after jit_stop().
void jit_resume(void);
// This process leads after a rollback: speculation that may have caused the
// divergence is made safer.
void jit_after_rollback(void);

// Interpreter state exported to the engine.
struct Decode *jit_jr_fetch(struct Decode *s, vaddr_t target);
int *jit_budget_slot(void);
struct Decode **jit_prev_s_slot(void);

#endif // CONFIG_JIT_SLJIT

#endif
