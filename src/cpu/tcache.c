#include <cpu/decode.h>

#define TCACHE_SIZE (1024 * 8)

static Decode tcache_pool[TCACHE_SIZE] = {};
static int tc_idx = 0;
static const void *nemu_decode_helper;

static Decode* tcache_next() {
  return &tcache_pool[tc_idx];
}

static Decode* tcache_new(vaddr_t pc) {
  assert(tc_idx < TCACHE_SIZE);
  tcache_pool[tc_idx].pc = pc;
  tcache_pool[tc_idx].snpc = 0;
  tcache_pool[tc_idx].EHelper = nemu_decode_helper;
  return &tcache_pool[tc_idx ++];
}

#define BB_INFO_SIZE 1024
#define BB_INFO_BACKPATCH_SIZE 7
static struct bbInfo {
  Decode *s;
  Decode **backpatch_list[BB_INFO_BACKPATCH_SIZE];
  vaddr_t pc;
} bb_info[BB_INFO_SIZE];

static struct bbInfo* tcache_bb_find_slow_path(vaddr_t pc, bool is_fill) {
  int idx = pc % BB_INFO_SIZE;
  int i;
  for (i = 0; i < BB_INFO_SIZE; i ++) {
    if (bb_info[idx].pc == 0) {
      if (!is_fill) return NULL;
      else {
        bb_info[idx].pc = pc;
        bb_info[idx].s = tcache_new(pc);
        return &bb_info[idx];
      }
    }
    if (bb_info[idx].pc == pc) return &bb_info[idx];
    idx = (idx + 1) % BB_INFO_SIZE;
  }
  panic("bb_info is full");
  return NULL;
}

static struct bbInfo* tcache_bb_find(vaddr_t pc, bool is_fill) {
  int idx = pc % BB_INFO_SIZE;
  if (likely(bb_info[idx].pc == pc)) return &bb_info[idx];
  return tcache_bb_find_slow_path(pc, is_fill);
}

// fetch the decode entry index by jpc, and fill it to
// the last instruction decode entry in a basic block
static Decode* tcache_bb_fetch(Decode **fill_addr, vaddr_t jpc) {
  struct bbInfo* bb = tcache_bb_find(jpc, true);
  if (fill_addr) {
    if (bb->s->snpc == 0) {
      // still not decoded, record fill_addr to the backpatch list
      int i;
      for (i = 0; i < BB_INFO_BACKPATCH_SIZE; i ++) {
        if (bb->backpatch_list[i] == NULL) break;
      }
      Assert(i < BB_INFO_BACKPATCH_SIZE,
          "backpatch entry is full with jpc = " FMT_WORD, jpc);
      bb->backpatch_list[i] = fill_addr;
    }
    *fill_addr = bb->s;
  }
  return bb->s;
}

static void tcache_bb_backpatch(Decode *s, vaddr_t bb_pc) {
  struct bbInfo* bb = tcache_bb_find(bb_pc, false);
  Assert(bb != NULL, "bb_info is not allocated with basic block pc = " FMT_WORD, bb_pc);
  Assert(bb->s == NULL || bb->s->snpc == 0,
      "bb_info is already backpatched: bb pc = " FMT_WORD ", src pc = " FMT_WORD, bb_pc, s->pc);
  bb->s = s;
  int i;
  for (i = 0; i < BB_INFO_BACKPATCH_SIZE; i ++) {
    if (bb->backpatch_list[i] != NULL) {
      *(bb->backpatch_list[i]) = s;
      bb->backpatch_list[i] = NULL;
    }
  }
}

enum { TCACHE_BB_BUILDING, TCACHE_RUNNING };
static int tcache_state = TCACHE_BB_BUILDING;
int fetch_decode(Decode *s, vaddr_t pc);

__attribute__((noinline))
Decode* tcache_jr_fetch(Decode *s, vaddr_t jpc) {
  struct bbInfo* bb = tcache_bb_find(jpc, true);
  Decode *ret = bb->s;
  if (ret->snpc != 0) {
    // only cache the entry when it is decoded
    s->ntnext = s->tnext;
    s->tnext = ret;
  }
  return ret;
}

__attribute__((noinline))
Decode* tcache_decode(Decode *s, const void **exec_table) {
  if (tcache_state == TCACHE_RUNNING) {
    if (s + 1 != tcache_next()) {
      // Allocate a new tcache entry to keep the tcache
      // entries in the basic block continuous.
      // Note that this will waste some tcache entries in the pool.
      s = tcache_new(s->pc);
    }
    tcache_bb_backpatch(s, s->pc);
    tcache_state = TCACHE_BB_BUILDING;
  }
  assert(s->snpc == 0);
  int idx = fetch_decode(s, s->pc);
  s->EHelper = exec_table[idx];
  if (s->type == INSTR_TYPE_N) {
    Decode *next = tcache_new(s->snpc);
    assert(next == s + 1);
  } else {
    if (s->type == INSTR_TYPE_J) {
      tcache_bb_fetch(&s->tnext, s->jnpc);
    } else if (s->type == INSTR_TYPE_B) {
      tcache_bb_fetch(&s->ntnext, s->snpc);
      tcache_bb_fetch(&s->tnext, s->jnpc);
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
  return tcache_new(reset_vector);
}
