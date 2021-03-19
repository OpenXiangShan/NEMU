#include <cpu/decode.h>
#include <cpu/cpu.h>
#include <stdlib.h>

#define TCACHE_SIZE (1024 * 8)

static Decode tcache_pool[TCACHE_SIZE] = {};
static int tc_idx = 0;
static const void **g_special_exec_table = NULL;

static const void* get_nemu_decode() {
  return g_special_exec_table[0];
}

static inline Decode* tcache_entry_init(Decode *s, vaddr_t pc) {
  memset(s, 0, sizeof(*s));
  s->pc = pc;
  s->EHelper = get_nemu_decode();
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


#define BB_POOL_SIZE 512

typedef struct bb_t {
  Decode *s;
  struct bb_t *next;
  vaddr_t pc;
} bb_t;

static bb_t bb_pool[BB_POOL_SIZE] = {};
static int bb_idx = 0;

static inline bb_t* bb_new(Decode *s, vaddr_t pc, bb_t *next) {
  if (bb_idx == BB_POOL_SIZE) return NULL;
  assert(bb_idx < BB_POOL_SIZE);
  bb_t *bb = &bb_pool[bb_idx ++];
  bb->s = s;
  bb->pc = pc;
  bb->next = next;
  return bb;
}

#define BB_LIST_SIZE 512
static bb_t bb_list [BB_LIST_SIZE] = {};

static inline bb_t* bb_hash(vaddr_t pc) {
  int idx = (pc / CONFIG_ILEN_MIN) % BB_LIST_SIZE;
  return &bb_list[idx];
}

static struct bb_t* bb_insert(vaddr_t pc, Decode *fill) {
  bb_t *head = bb_hash(pc);
  if (head->next == (void *)-1ul) {
    if (head->pc == (vaddr_t)-1ul) {
      // first time
      head->s = fill;
      head->pc = pc;
      return head;
    }
  }
  // second time
  bb_t *bb = bb_new(head->s, head->pc, head->next);
  if (bb == NULL) return NULL;
  head->s = fill;
  head->pc = pc;
  head->next = bb;
  return head;
}

static bb_t* bb_find(vaddr_t pc) {
  bb_t *bb = bb_hash(pc);
  if (likely(bb->pc == pc)) return bb;
  bb_t *head = bb;
  do {
    bb = bb->next;
    if (bb == (void *)-1ul) return NULL;
    if (bb->pc == pc) {
      Decode *tmp_s = bb->s; vaddr_t tmp_pc = bb->pc;
      bb->s = head->s; bb->pc = head->pc;
      head->s = tmp_s; head->pc = tmp_pc;
      return head;
    }
  } while (1);
}

static void tcache_bb_fetch(Decode *this, int is_taken, vaddr_t jpc) {
  bb_t* bb = bb_find(jpc);
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
  bb_idx = 0;
  memset(bb_list, -1, sizeof(bb_list));
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
  int bb_start = (tcache_state == TCACHE_RUNNING);
  Decode *old = s;

  if (bb_start) {
    // first check whether this basic block is already decoded
    bb_t *bb = bb_find(old->pc);
    if (bb != NULL) { // already decoded
      s = bb->s;
      goto bb_start_already_decode;
    }
  }

  save_globals(s);
  int idx = fetch_decode(s, s->pc); // note that exception may happen!
  s->EHelper = exec_table[idx];

  if (bb_start) {
    s = tcache_new(old->pc);
    if (s == NULL) goto full;
    bb_t *ret = bb_insert(old->pc, s);
    if (ret == NULL) { // basic block list is full
full: old->EHelper = get_nemu_decode(); // decode again
      save_globals(old);
      tcache_flush();
      longjmp_exec(NEMU_EXEC_AGAIN);
    }

    // now is safe to update states
    *s = *old;
    idx_in_bb = 1;
    tcache_state = TCACHE_BB_BUILDING;
  }

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

  if (bb_start) {
bb_start_already_decode:
    if (old->tnext)  { old->tnext->tnext = s; }
    if (old->ntnext) { old->ntnext->ntnext = s; }
    free(old);
  }
  return s;
}

static Decode *ex = NULL;

void tcache_handle_exception(vaddr_t jpc) {
  if (tcache_state == TCACHE_BB_BUILDING) {
    // When exception happens in the middle of the basic block,
    // the property `next == s + 1` may not hold when the exception returns.
    // This is hard to fix, therefore we just flush the tcache.
    tcache_flush();
  }
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
  tcache_flush();
  g_special_exec_table = special_exec_table;
  ex = tcache_new_malloc(0);
  ex->EHelper = special_exec_table[1];
  return tcache_new_malloc(reset_vector);
}
