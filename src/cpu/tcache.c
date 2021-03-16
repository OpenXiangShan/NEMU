#include <cpu/decode.h>
#include <cpu/cpu.h>
#include <stdlib.h>

#define TCACHE_SIZE (1024 * 8)

static Decode tcache_pool[TCACHE_SIZE] = {};
static int tc_idx = 0;
static const void **g_special_exec_table = NULL;

static inline bool in_tcache_pool(Decode *s) {
  return (s >= tcache_pool && s < &tcache_pool[TCACHE_SIZE]);
}

static inline Decode* tcache_entry_init(Decode *s, vaddr_t pc) {
  memset(s, 0, sizeof(*s));
  s->pc = pc;
  s->EHelper = g_special_exec_table[0];
  return s;
}

static inline Decode* tcache_new(vaddr_t pc) {
  if (tc_idx == TCACHE_SIZE) return NULL;
  assert(tc_idx < TCACHE_SIZE);
  tcache_entry_init(&tcache_pool[tc_idx], pc);
  return &tcache_pool[tc_idx ++];
}

static inline Decode* tcache_new_malloc(vaddr_t pc) {
  return tcache_entry_init(malloc(sizeof(Decode)), pc);
}

#define BB_INFO_SIZE 1024
static struct bbInfo {
  Decode *s;
  vaddr_t pc;
} bb_info[BB_INFO_SIZE];

static struct bbInfo* tcache_bb_find_slow_path(vaddr_t pc, Decode *fill) {
  int idx = pc % BB_INFO_SIZE;
  int i;
  for (i = 0; i < BB_INFO_SIZE; i ++) {
    if (bb_info[idx].pc == 0) {
      if (!fill) return NULL;
      else {
        bb_info[idx].pc = pc;
        bb_info[idx].s = fill;
        return &bb_info[idx];
      }
    }
    if (bb_info[idx].pc == pc) {
      assert(fill == NULL);
      return &bb_info[idx];
    }
    idx = (idx + 1) % BB_INFO_SIZE;
  }
  return NULL;
}

static struct bbInfo* tcache_bb_find(vaddr_t pc) {
  int idx = pc % BB_INFO_SIZE;
  if (likely(bb_info[idx].pc == pc)) return &bb_info[idx];
  return tcache_bb_find_slow_path(pc, NULL);
}

static void tcache_bb_fetch(Decode *this, int is_taken, vaddr_t jpc) {
  struct bbInfo* bb = tcache_bb_find(jpc);
  if (bb != NULL) {
    if (is_taken) { this->tnext = bb->s; }
    else { this->ntnext = bb->s; }
  } else {
    Decode *ret = tcache_new_malloc(jpc);
    if (is_taken) { ret->tnext = this; this->tnext = ret; }
    else { ret->ntnext = this; this->ntnext = ret; }
  }
}

static void tcache_flush() {
  tc_idx = 0;
  memset(bb_info, 0, sizeof(bb_info));
}

enum { TCACHE_BB_BUILDING, TCACHE_RUNNING };
static int tcache_state = TCACHE_RUNNING;
int fetch_decode(Decode *s, vaddr_t pc);

__attribute__((noinline))
Decode* tcache_jr_fetch(Decode *s, vaddr_t jpc) {
  s->ntnext = s->tnext;
  tcache_bb_fetch(s, true, jpc);
  return s->tnext;
}

__attribute__((noinline))
Decode* tcache_decode(Decode *s, const void **exec_table) {
  static int idx_in_bb = 0;
  if (tcache_state == TCACHE_RUNNING) {
    Decode *old = s;
    // first check whether this basic block is already decoded
    struct bbInfo *bb = tcache_bb_find(old->pc);
    bool already_decode = (bb != NULL);
    if (!already_decode) {
      s = tcache_new(old->pc);
      if (s == NULL) goto full;
      struct bbInfo *ret = tcache_bb_find_slow_path(old->pc, s);
      if (ret == NULL) { // basic block list is full
full:   save_globals(old);
        tcache_flush();
        longjmp_exec(NEMU_EXEC_AGAIN);
      }
    } else { s = bb->s; }
    if (old->tnext)  { old->tnext->tnext = s; }
    if (old->ntnext) { old->ntnext->ntnext = s; }
    if (!in_tcache_pool(old)) free(old);
    if (already_decode) return s;
    tcache_state = TCACHE_BB_BUILDING;
    idx_in_bb = 1;
  }
  save_globals(s);
  int idx = fetch_decode(s, s->pc);
  s->EHelper = exec_table[idx];
  s->idx_in_bb = idx_in_bb ++;
  if (s->type == INSTR_TYPE_N) {
    Decode *next = tcache_new(s->snpc);
    if (next == NULL) {
      tcache_flush();
      Decode *again = tcache_new(s->pc);
      save_globals(again); // decode this instruction again
      longjmp_exec(NEMU_EXEC_AGAIN);
    }
    assert(next == s + 1);
  } else {
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
  return s;
}

static Decode *ex = NULL;

void tcache_handle_exception(vaddr_t jpc) {
  ex->jnpc = jpc;
  tcache_bb_fetch(ex, true, jpc);
  save_globals(ex);
  tcache_state = TCACHE_RUNNING;
}

Decode* tcache_handle_flush(vaddr_t snpc) {
  tcache_flush();
  tcache_handle_exception(snpc);
  ex->pc = snpc;
  return ex;
}

Decode* tcache_init(const void **special_exec_table, vaddr_t reset_vector) {
  g_special_exec_table = special_exec_table;
  ex = tcache_new_malloc(0);
  ex->EHelper = special_exec_table[1];
  return tcache_new_malloc(reset_vector);
}
