#include <cpu/decode.h>
#include <cpu/cpu.h>

#define TCACHE_SIZE (8 * 1024)
#define TCACHE_TMP_SIZE (TCACHE_SIZE / 4 + 2)
#define BB_POOL_SIZE 1024
#define BB_LIST_SIZE 1024

typedef struct bb_t {
  Decode *s;
  struct bb_t *next;
  vaddr_t pc;
} bb_t;

static Decode tcache_pool[TCACHE_SIZE] = {};
static int tc_idx = 0;
static Decode tcache_tmp_pool[TCACHE_TMP_SIZE] = {};
static Decode *tcache_tmp_freelist = NULL;
static bb_t bb_pool[BB_POOL_SIZE] = {};
static int bb_idx = 0;
static bb_t bb_list [BB_LIST_SIZE] = {};
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

#ifdef DEBUG
#define tcache_tmp_check(s) do { \
  int idx = s - tcache_tmp_pool; \
  Assert(idx >= 0 && idx < TCACHE_TMP_SIZE, "idx = %d, s = %p", idx, s); \
} while (0)
#else
#define tcache_tmp_check(s)
#endif

static inline Decode* tcache_tmp_new(vaddr_t pc) {
  Decode *s = tcache_tmp_freelist;
  assert(s != NULL);
  tcache_tmp_check(s);
  tcache_tmp_check(tcache_tmp_freelist->tnext);
  tcache_tmp_freelist = tcache_tmp_freelist->tnext;
  tcache_tmp_check(tcache_tmp_freelist);
  tcache_tmp_check(tcache_tmp_freelist->tnext);
  return tcache_entry_init(s, pc);
}

static inline void tcache_tmp_free(Decode *s) {
  tcache_tmp_check(s);
  tcache_tmp_check(tcache_tmp_freelist);
  s->tnext = tcache_tmp_freelist;
  tcache_tmp_freelist = s;
  tcache_tmp_check(tcache_tmp_freelist->tnext);
}


static inline bb_t* bb_new(Decode *s, vaddr_t pc, bb_t *next) {
  if (bb_idx == BB_POOL_SIZE) return NULL;
  assert(bb_idx < BB_POOL_SIZE);
  bb_t *bb = &bb_pool[bb_idx ++];
  bb->s = s;
  bb->pc = pc;
  bb->next = next;
  return bb;
}

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
    Decode *ret = tcache_tmp_new(jpc);
    if (is_taken) { ret->tnext = this; this->tnext = ret; }
    else { ret->ntnext = this; this->ntnext = ret; }
  }
}

static void tcache_flush() {
  tc_idx = 0;
  bb_idx = 0;
  memset(bb_list, -1, sizeof(bb_list));

  int i;
  for (i = 0; i < TCACHE_TMP_SIZE - 1; i ++) {
    tcache_tmp_pool[i].tnext = &tcache_tmp_pool[i + 1];
  }
  tcache_tmp_pool[TCACHE_TMP_SIZE - 1].tnext = NULL;
  tcache_tmp_freelist = &tcache_tmp_pool[0];
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
      if (old->tnext)  { old->tnext->tnext = s; }
      if (old->ntnext) { old->ntnext->ntnext = s; }
      tcache_tmp_free(old);
      return s;
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
    if (old->tnext)  { old->tnext->tnext = s; }
    if (old->ntnext) { old->ntnext->ntnext = s; }
    tcache_tmp_free(old);
  }

  s->idx_in_bb = idx_in_bb ++;

  if (s->type == INSTR_TYPE_N) {
    Decode *next = tcache_new(s->snpc);
    if (next == NULL) {
      vaddr_t thispc = s->pc;
      tcache_flush();
      idx_in_bb = 1;
      // decode this instruction again
      Decode *again = tcache_tmp_new(thispc);
      save_globals(again);
      tcache_state = TCACHE_RUNNING;
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

static Decode ex = {};

void tcache_handle_exception(vaddr_t jpc) {
  if (tcache_state == TCACHE_BB_BUILDING) {
    // When exception happens in the middle of the basic block,
    // the property `next == s + 1` may not hold when the exception returns.
    // This is hard to fix, therefore we just flush the tcache.
    tcache_flush();
  }
  tcache_bb_fetch(&ex, true, jpc);
  save_globals(ex.tnext);
  tcache_state = TCACHE_RUNNING;
}

Decode* tcache_handle_flush(vaddr_t snpc) {
  tcache_flush();
  tcache_handle_exception(snpc);
  return ex.tnext;
}

Decode* tcache_init(const void **special_exec_table, vaddr_t reset_vector) {
  tcache_flush();
  g_special_exec_table = special_exec_table;
  return tcache_tmp_new(reset_vector);
}
