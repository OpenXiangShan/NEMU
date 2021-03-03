#include <cpu/decode.h>
#include <stdlib.h>

#define TCACHE_SIZE (1024 * 8)

static Decode tcache_pool[TCACHE_SIZE] = {};
static int tc_idx = 0;
static const void *nemu_decode_helper;
//static void tcache_full();

static inline Decode* tcache_entry_init(Decode *s, vaddr_t pc) {
  memset(s, 0, sizeof(*s));
  s->pc = pc;
  s->EHelper = nemu_decode_helper;
  return s;
}

static inline Decode* tcache_new(vaddr_t pc) {
  //if (tc_idx == TCACHE_SIZE) tcache_full();
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
  panic("tcacbe bb full");
  return NULL;
}

static struct bbInfo* tcache_bb_find(vaddr_t pc) {
  int idx = pc % BB_INFO_SIZE;
  if (likely(bb_info[idx].pc == pc)) return &bb_info[idx];
  return tcache_bb_find_slow_path(pc, NULL);
}

// fetch the decode entry index by jpc, and fill it to
// the last instruction decode entry in a basic block
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
      tcache_bb_find_slow_path(old->pc, s);
    } else { s = bb->s; }
    if (old->tnext)  { old->tnext->tnext = s; }
    if (old->ntnext) { old->ntnext->ntnext = s; }
    free(old);
    if (already_decode) return s;
    tcache_state = TCACHE_BB_BUILDING;
    idx_in_bb = 1;
  }
  int idx = fetch_decode(s, s->pc);
  s->EHelper = exec_table[idx];
  s->idx_in_bb = idx_in_bb ++;
  if (s->type == INSTR_TYPE_N) {
    Decode *next = tcache_new(s->snpc);
    assert(next == s + 1);
  } else {
    if (s->type == INSTR_TYPE_J) {
      tcache_bb_fetch(s, true, s->jnpc);
    } else if (s->type == INSTR_TYPE_B) {
      tcache_bb_fetch(s, true, s->jnpc);
      tcache_bb_fetch(s, false, s->snpc);
    } else if (s->type == INSTR_TYPE_I) {
      s->tnext = s->ntnext = s; // update dynamically
    }
    tcache_state = TCACHE_RUNNING;
  }
  return s;
}

void tcache_flush() {
  tc_idx = 0;
  memset(bb_info, 0, sizeof(bb_info));
}

Decode* tcache_init(const void *nemu_decode, vaddr_t reset_vector) {
  nemu_decode_helper = nemu_decode;
  return tcache_new_malloc(reset_vector);
}

#if 0
static void tcache_full() {
  extern Decode *prev_s;
  vaddr_t thispc = prev_s->pc;
  tcache_flush();
  prev_s = tcache_new_malloc(thispc);
  longjmp_exec(123); // TCACHE_FULL
}
#endif
