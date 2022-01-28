#include <cpu/decode.h>
#include <cpu/cpu.h>

#ifdef CONFIG_PERF_OPT

#define TCACHE_BB_SIZE (CONFIG_TCACHE_SIZE / 4 + 2)

typedef struct bb_t {
  Decode *s;
  struct bb_t *next;
  vaddr_t pc;
} bb_t;

enum { BB_RECORD_TYPE_NTAKEN = 1, BB_RECORD_TYPE_TAKEN };

static Decode tcache_pool[CONFIG_TCACHE_SIZE] = {};
static ExtraInfo tcache_extra[CONFIG_TCACHE_SIZE] = {};
static int tc_idx = 0;
static Decode tcache_bb_pool[TCACHE_BB_SIZE] = {};
static ExtraInfo tcache_bb_extra[TCACHE_BB_SIZE] = {};
static Decode *tcache_bb_freelist = NULL;
static bb_t bb_pool[CONFIG_BB_POOL_SIZE] = {};
static int bb_idx = 0;
static bb_t bb_list [CONFIG_BB_LIST_SIZE] = {};
static const void *g_exec_nemu_decode;

#ifdef CONFIG_BB_COUNT
static int bb_list_count[CONFIG_BB_LIST_SIZE] = {};
static int bb_pool_count[CONFIG_BB_POOL_SIZE] = {};
static int bb_list_insts[CONFIG_BB_LIST_SIZE] = {};
static int bb_pool_insts[CONFIG_BB_POOL_SIZE] = {};
#endif
static Decode* tcache_entry_init(Decode *s, vaddr_t pc) {
  ExtraInfo* extra = s->extraInfo;
  s->tnext = s->ntnext = NULL;
  extra->type = 0;
  extra->pc = pc;
  s->EHelper = g_exec_nemu_decode;
  return s;
}

static Decode* tcache_new(vaddr_t pc) {
  if (tc_idx == CONFIG_TCACHE_SIZE) return NULL;
  assert(tc_idx < CONFIG_TCACHE_SIZE);
  Decode *s = &tcache_pool[tc_idx];
  tc_idx ++;
  return tcache_entry_init(s, pc);
}

#ifdef CONFIG_RT_CHECK
#define tcache_bb_check(s) do { \
  assert(s != NULL); \
  int idx = s - tcache_bb_pool; \
  Assert(idx >= 0 && idx < TCACHE_BB_SIZE, "idx = %d, s = %p", idx, s); \
} while (0)
#else
#define tcache_bb_check(s)
#endif

static Decode* tcache_bb_new(vaddr_t pc) {
  Decode *s = tcache_bb_freelist;
  tcache_bb_check(s);
  tcache_bb_check(tcache_bb_freelist->tnext);
  tcache_bb_freelist = tcache_bb_freelist->tnext;
  tcache_bb_check(tcache_bb_freelist);
  tcache_bb_check(tcache_bb_freelist->tnext);
  return tcache_entry_init(s, pc);
}

static void tcache_bb_free(Decode *s) {
  int idx = s - tcache_bb_pool;
  if (!(idx >= 0 && idx < TCACHE_BB_SIZE)) { return; }
  tcache_bb_check(s);
  tcache_bb_check(tcache_bb_freelist);
  s->tnext = tcache_bb_freelist;
  tcache_bb_freelist = s;
  tcache_bb_check(tcache_bb_freelist->tnext);
}


static bb_t* bb_new(Decode *s, vaddr_t pc, bb_t *next) {
  if (bb_idx == CONFIG_BB_POOL_SIZE) return NULL;
  assert(bb_idx < CONFIG_BB_POOL_SIZE);
  bb_t *bb = &bb_pool[bb_idx ++];
  bb->s = s;
  bb->pc = pc;
  bb->next = next;
  return bb;
}

static bb_t* bb_hash(vaddr_t pc) {
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
    if (bb == (void *)-1ul)
#if defined(CONFIG_ISA_riscv64) || defined(CONFIG_ISA_riscv32)
      return pc & 1 ? bb_find(pc & (~1ull)) : NULL;
#else
      return NULL;
#endif
    if (bb->pc == pc) {
      Decode *tmp_s = bb->s; vaddr_t tmp_pc = bb->pc;
      bb->s = head->s; bb->pc = head->pc;
      head->s = tmp_s; head->pc = tmp_pc;
      return head;
    }
  } while (1);
}
#ifdef CONFIG_BB_COUNT
void update_bb_count(vaddr_t pc, int decode_num){
  bb_t* bb = bb_find(pc);
  if(!bb) { return;}
  int pool_idx = bb - bb_pool;
  int list_idx = bb - bb_list;
  if(pool_idx >= 0 & pool_idx < CONFIG_BB_POOL_SIZE){
    bb_pool_count[pool_idx] ++;
    if(decode_num != 0) bb_pool_insts[pool_idx] = decode_num;
  }
  else if(list_idx >= 0 & list_idx < CONFIG_BB_LIST_SIZE){
    bb_list_count[list_idx] ++;
    if(decode_num != 0) bb_list_insts[list_idx] = decode_num;
  }
  else Assert(0, "pc=%lx pool_idx=%d list_idx=%d\n", pc, pool_idx, list_idx);
}
#endif

static void tcache_bb_fetch(Decode *_this, int is_taken, vaddr_t jpc) {
  bb_t* bb = bb_find(jpc);
  if (bb != NULL) {
    if (is_taken) {_this->tnext = bb->s; }
    else { _this->ntnext = bb->s; }
  } else {
    Decode *ret = tcache_bb_new(jpc);
    if (is_taken) { ret->extraInfo->type = BB_RECORD_TYPE_TAKEN; _this->tnext = ret; }
    else { ret->extraInfo->type = BB_RECORD_TYPE_NTAKEN; _this->ntnext = ret; }
    ret->bb_src = _this;
  }
}

#ifdef CONFIG_BB_COUNT
extern FILE * bb_fp;
void save_bb_msg(){
  for(int i = 0; i < CONFIG_BB_POOL_SIZE; i++){
    if(bb_pool_count[i] != 0){
      fprintf(bb_fp, "0x%lx %d %d\n", bb_pool[i].extraInfo.pc, bb_pool_count[i], bb_pool_insts[i]);
      fflush(bb_fp);
    }
  }

  for(int i = 0; i < CONFIG_BB_LIST_SIZE; i++){
    if(bb_list_count[i] != 0){
      fprintf(bb_fp, "0x%lx %d %d\n", bb_list[i].extraInfo.pc, bb_list_count[i], bb_list_insts[i]);
      fflush(bb_fp);
    }
  }
}
#endif

static void tcache_flush() {
#ifdef CONFIG_BB_COUNT
  if(bb_idx != 0) save_bb_msg();
  memset(bb_list_count, 0, sizeof(bb_list_count));
  memset(bb_pool_count, 0, sizeof(bb_pool_count));
  memset(bb_list_insts, 0, sizeof(bb_list_insts));
  memset(bb_pool_insts, 0, sizeof(bb_pool_insts));
#endif
  tc_idx = 0;
  bb_idx = 0;
  memset(bb_list, -1, sizeof(bb_list));

  int i;
  for (i = 0; i < TCACHE_BB_SIZE - 1; i ++) {
    tcache_bb_pool[i].list_next = &tcache_bb_pool[i + 1];
  }
  tcache_bb_pool[TCACHE_BB_SIZE - 1].list_next = NULL;
  tcache_bb_freelist = &tcache_bb_pool[0];
}

enum { TCACHE_BB_BUILDING, TCACHE_RUNNING };
static int tcache_state = TCACHE_RUNNING;
static Decode *bb_now = NULL, *bb_now_record = NULL;

__attribute__((noinline))
Decode* tcache_jr_fetch(Decode *s, vaddr_t jpc) {
  s->ntnext = s->tnext;
  tcache_bb_fetch(s, true, jpc);
  return s->tnext;
}

static void tcache_patch_and_free(Decode *bb_record, Decode *bb) {
  Decode *src = bb_record->bb_src;
  if (bb_record->extraInfo->type == BB_RECORD_TYPE_TAKEN)  { src->tnext = bb; }
  if (bb_record->extraInfo->type == BB_RECORD_TYPE_NTAKEN) { src->ntnext = bb; }
  tcache_bb_free(bb_record);
}

__attribute__((noinline))
Decode* tcache_decode(Decode *s) {
  static int idx_in_bb = 0;
  vaddr_t thispc = s->extraInfo->pc;

  if (tcache_state == TCACHE_RUNNING) {  // start of a basic block
    // first check whether this basic block is already decoded
    bb_t *bb = bb_find(thispc);
    if (bb != NULL) { // already decoded
      tcache_patch_and_free(s, bb->s);
      return bb->s;
    }

    Decode *old = s;
    s = tcache_new(thispc);
    idx_in_bb = 1;
    if (s == NULL) { goto full; }

    bb_now_record = old;
    bb_now = s;
    tcache_state = TCACHE_BB_BUILDING;
  }

  save_globals(s);
  s->extraInfo->idx_in_bb = idx_in_bb;

  fetch_decode(s, thispc); // note that exception may happen!
  if (s->extraInfo->type == INSTR_TYPE_N) {
    Decode *next = tcache_new(s->extraInfo->snpc);
    if (next == NULL) { goto full; }
    assert(next == s + 1);
  } else {
    // the end of the basic block
    bb_t *ret = bb_insert(bb_now->extraInfo->pc, bb_now);
    if (ret == NULL) { goto full; } // basic block list is full
    tcache_patch_and_free(bb_now_record, bb_now);
    bb_now = bb_now_record = NULL;

    switch (s->extraInfo->type) {
      case INSTR_TYPE_J: tcache_bb_fetch(s, true, s->extraInfo->jnpc); break;
      case INSTR_TYPE_B:
        tcache_bb_fetch(s, true, s->extraInfo->jnpc);
        tcache_bb_fetch(s, false, s->extraInfo->snpc + MUXDEF(CONFIG_ISA_mips32, 4, 0));
        break;
      case INSTR_TYPE_I: s->tnext = s->ntnext = s; break; // update dynamically
      default: assert(0);
    }
    tcache_state = TCACHE_RUNNING;
  }

  idx_in_bb ++;
  return s;

full:
  tcache_flush();
  s = tcache_bb_new(thispc); // decode this instruction again
  s->extraInfo->idx_in_bb = idx_in_bb;
  save_globals(s);
  bb_now = bb_now_record = NULL;
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

Decode* tcache_init(const void *exec_nemu_decode, vaddr_t reset_vector) {
  for(int i = 0; i < CONFIG_TCACHE_SIZE; i++){
    tcache_pool[i].extraInfo = &tcache_extra[i];
  }
  for(int i = 0; i < TCACHE_BB_SIZE; i++){
    tcache_bb_pool[i].extraInfo = &tcache_bb_extra[i];
  }
  tcache_flush();
  g_exec_nemu_decode = exec_nemu_decode;
  return tcache_bb_new(reset_vector);
}

void tcache_check_and_flush(vaddr_t pc) {
  bool nearly_full = false;
  nearly_full |= (tc_idx > CONFIG_TCACHE_SIZE * 15 / 16);
  nearly_full |= (bb_idx > CONFIG_BB_POOL_SIZE * 15 / 16);
  if (nearly_full) { tcache_handle_flush(pc); }
}
#endif
