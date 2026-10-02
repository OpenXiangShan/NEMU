#include <memory/store_queue_wrapper.h>
#include <queue>
#include <stack>

#ifdef CONFIG_STORE_LOG
std::stack<store_log_t> store_log_stack;
void store_log_stack_reset() { store_log_stack = {};}
void store_log_stack_push(store_log_t log) { store_log_stack.push(log);}
void store_log_stack_pop() { store_log_stack.pop();}
store_log_t store_log_stack_top() {return store_log_stack.top();}
bool store_log_stack_empty() { return store_log_stack.empty();}
#ifdef CONFIG_LIGHTQS
std::stack<store_log_t> spec_store_log_stack;
void spec_store_log_stack_reset() { spec_store_log_stack = {};}
void spec_store_log_stack_push(store_log_t log) { spec_store_log_stack.push(log);}
void spec_store_log_stack_pop() { spec_store_log_stack.pop();}
store_log_t spec_store_log_stack_top() { return spec_store_log_stack.top();}
bool spec_store_log_stack_empty() { return spec_store_log_stack.empty();}
void spec_store_log_stack_copy() { store_log_stack = spec_store_log_stack;}
#endif // CONFIG_LIGHTQS
#endif // CONFIG_STORE_LOG

#ifdef CONFIG_DIFFTEST_STORE_COMMIT

std::queue<store_commit_t> cpp_store_event_queue;
bool store_queue_has_overflow = false;

void store_queue_reset() {
  cpp_store_event_queue = {};
  store_queue_has_overflow = false;
}

void store_queue_push(store_commit_t store_commit) {
  if (store_queue_has_overflow) {
    return;
  }
  Logm("push store addr = " FMT_PADDR ", data = " FMT_WORD ", mask = 0x%x", store_commit.addr, store_commit.data, store_commit.mask);
  cpp_store_event_queue.push(store_commit);
  if (cpp_store_event_queue.size() >= CONFIG_DIFFTEST_STORE_QUEUE_SIZE) {
    store_queue_has_overflow = true;
  }
}

void store_queue_pop() {
  cpp_store_event_queue.pop();
}

store_commit_t store_queue_front() {
  auto store_commit = cpp_store_event_queue.front();
  return store_commit;
}

store_commit_t store_queue_back() {
  auto store_commit = cpp_store_event_queue.back();
  return store_commit;
}

size_t store_queue_size() {
  return cpp_store_event_queue.size();
}

bool store_queue_empty() {
  return cpp_store_event_queue.empty();
}

bool store_queue_overflow() {
  return store_queue_has_overflow;
}

#ifdef CONFIG_RV_AME
std::queue<matrix_store_commit_t> cpp_matrix_store_event_queue;

void matrix_store_queue_reset() {
  cpp_matrix_store_event_queue = {};
}

void matrix_store_queue_push(matrix_store_commit_t store_commit) {
  cpp_matrix_store_event_queue.push(store_commit);
}

void matrix_store_queue_pop() {
  cpp_matrix_store_event_queue.pop();
}

matrix_store_commit_t matrix_store_queue_front() {
  auto store_commit = cpp_matrix_store_event_queue.front();
  return store_commit;
}

matrix_store_commit_t matrix_store_queue_back() {
  auto store_commit = cpp_matrix_store_event_queue.back();
  return store_commit;
}

size_t matrix_store_queue_size() {
  return cpp_matrix_store_event_queue.size();
}

bool matrix_store_queue_empty() {
  return cpp_matrix_store_event_queue.empty();
}
#endif // CONFIG_RV_AME

void store_queue_discard() {
  store_queue_reset();
}

int store_queue_check_hash(uint64_t count, uint64_t hash_lo, uint64_t hash_hi, uint64_t group_id,
                           uint64_t instr_begin, uint64_t instr_end) {
  if (store_queue_has_overflow) {
    printf("[StoreHash] NEMU store commit queue overflow in group %lu.\n", group_id);
    return 1;
  }
  if (cpp_store_event_queue.size() < count) {
    printf("[StoreHash] NEMU queue underflow in group %lu: need %lu records, have %zu.\n", group_id, count,
           cpp_store_event_queue.size());
    return 1;
  }

  DifftestStoreHashState actual;
  difftest_store_hash_init(&actual);
  auto pending = cpp_store_event_queue;
  for (uint64_t i = 0; i < count; i++) {
    const store_commit_t store_commit = pending.front();
    pending.pop();
    difftest_store_hash_update(&actual, store_commit.addr, store_commit.data, store_commit.mask);
  }

  if (actual.h0 == hash_lo && actual.h1 == hash_hi && actual.count == count) {
    for (uint64_t i = 0; i < count; i++) {
      cpp_store_event_queue.pop();
    }
    return 0;
  }

  printf("[StoreHash] mismatch version=%d group=%lu instr=[%lu,%lu] records=%lu\n", DIFFTEST_STORE_HASH_VERSION,
         group_id, instr_begin, instr_end, count);
  printf("[StoreHash] expected DUT hash=(0x%016lx,0x%016lx), NEMU hash=(0x%016lx,0x%016lx)\n", hash_lo, hash_hi,
         actual.h0, actual.h1);
  auto to_print = cpp_store_event_queue;
  for (uint64_t i = 0; i < count; i++) {
    const store_commit_t store_commit = to_print.front();
    to_print.pop();
    printf("[StoreHash] NEMU store[%lu] pc=0x%016lx addr=0x%016lx data=0x%016lx mask=0x%02x\n", i,
           store_commit.pc, store_commit.addr, store_commit.data, store_commit.mask);
  }
  return 1;
}

#endif
