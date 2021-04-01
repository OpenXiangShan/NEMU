#include <cpu/decode.h>
#include <cpu/cpu.h>

#define TCACHE_TMP_SIZE (CONFIG_TCACHE_SIZE / 4 + 2)

typedef struct bb_t {
  Decode *s;
  struct bb_t *next;
  vaddr_t pc;
} bb_t;

static Decode tcache_pool[CONFIG_TCACHE_SIZE] = {};
static int tc_idx = 0;
static Decode tcache_tmp_pool[TCACHE_TMP_SIZE] = {};
static Decode *tcache_tmp_freelist = NULL;
static bb_t bb_pool[CONFIG_BB_POOL_SIZE] = {};
static int bb_idx = 0;
static bb_t bb_list [CONFIG_BB_LIST_SIZE] = {};
static const void **g_special_exec_table = NULL;

static const void* get_nemu_decode() {
  return g_special_exec_table[0];
}

static inline Decode* tcache_entry_init(Decode *s, vaddr_t pc) {
  s->tnext = s->ntnext = NULL;
  s->pc = pc;
  s->EHelper = get_nemu_decode();
  return s;
}

static inline Decode* tcache_new(vaddr_t pc) {
  if (tc_idx == CONFIG_TCACHE_SIZE) return NULL;
  assert(tc_idx < CONFIG_TCACHE_SIZE);
  Decode *s = &tcache_pool[tc_idx];
  tc_idx ++;
  return tcache_entry_init(s, pc);
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
  if (bb_idx == CONFIG_BB_POOL_SIZE) return NULL;
  assert(bb_idx < CONFIG_BB_POOL_SIZE);
  bb_t *bb = &bb_pool[bb_idx ++];
  bb->s = s;
  bb->pc = pc;
  bb->next = next;
  return bb;
}

static inline bb_t* bb_hash(vaddr_t pc) {
  int idx = (pc / CONFIG_ILEN_MIN) % CONFIG_BB_LIST_SIZE;
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
static Decode *bb_now = NULL, *bb_now_tmp = NULL;
int fetch_decode(Decode *s, vaddr_t pc);

__attribute__((noinline))
Decode* tcache_jr_fetch(Decode *s, vaddr_t jpc) {
  s->ntnext = s->tnext;
  tcache_bb_fetch(s, true, jpc);
  return s->tnext;
}

static inline void tcache_patch_and_free(Decode *bb_tmp, Decode *bb) {
  if (bb_tmp->tnext)  { bb_tmp->tnext->tnext = bb; }
  if (bb_tmp->ntnext) { bb_tmp->ntnext->ntnext = bb; }
  tcache_tmp_free(bb_tmp);
}

__attribute__((noinline))
Decode* tcache_decode(Decode *s, const void **exec_table) {
  static int idx_in_bb = 0;
  vaddr_t thispc = s->pc;

  if (tcache_state == TCACHE_RUNNING) {  // start of a basic block
    // first check whether this basic block is already decoded
    bb_t *bb = bb_find(thispc);
    if (bb != NULL) { // already decoded
      tcache_patch_and_free(s, bb->s);
      return bb->s;
    }

    Decode *old = s;
    s = tcache_new(thispc);
    if (s == NULL) { goto full; }

    bb_now_tmp = old;
    bb_now = s;
    idx_in_bb = 1;
    tcache_state = TCACHE_BB_BUILDING;
  }

  save_globals(s);
  int idx = fetch_decode(s, thispc); // note that exception may happen!
  s->EHelper = exec_table[idx];
  s->idx_in_bb = idx_in_bb ++;

  if (s->type == INSTR_TYPE_N) {
    Decode *next = tcache_new(s->snpc);
    if (next == NULL) { goto full; }
    assert(next == s + 1);
  } else {
    // the end of the basic block
    bb_t *ret = bb_insert(bb_now->pc, bb_now);
    if (ret == NULL) { goto full; } // basic block list is full
    tcache_patch_and_free(bb_now_tmp, bb_now);
    bb_now = bb_now_tmp = NULL;

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

full:
  tcache_flush();
  s = tcache_tmp_new(thispc); // decode this instruction again
  save_globals(s);
  bb_now = bb_now_tmp = NULL;
  tcache_state = TCACHE_RUNNING;
  longjmp_exec(NEMU_EXEC_AGAIN);
}

static Decode ex = {};

void tcache_handle_exception(vaddr_t jpc) {
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
