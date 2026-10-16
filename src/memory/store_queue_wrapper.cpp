#include <memory/store_queue_wrapper.h>
#include <utils.h>
#include <deque>
#include <cstdlib>
#include <queue>
#include <stack>

#ifdef CONFIG_STORE_LOG
#ifdef CONFIG_RV_AME
static void store_log_ref(store_log_t log) {
  if (log.is_matrix) {
    ++log.matrix->ref_count;
  }
}

static void store_log_unref(store_log_t log) {
  if (log.is_matrix) {
    assert(log.matrix->ref_count > 0);
    if (--log.matrix->ref_count == 0) {
      std::free(log.matrix);
    }
  }
}
#endif // CONFIG_RV_AME

// top() returns a borrowed view; only push/pop/reset/copy change ownership.
std::stack<store_log_t> store_log_stack;
void store_log_stack_reset() {
  while (!store_log_stack.empty()) { store_log_stack_pop(); }
}
void store_log_stack_push(store_log_t log) {
  store_log_stack.push(log);
  IFDEF(CONFIG_RV_AME, store_log_ref(log);)
}
void store_log_stack_pop() {
  IFDEF(CONFIG_RV_AME, store_log_unref(store_log_stack.top());)
  store_log_stack.pop();
}
store_log_t store_log_stack_top() {return store_log_stack.top();}
bool store_log_stack_empty() { return store_log_stack.empty();}
#ifdef CONFIG_LIGHTQS
std::stack<store_log_t> spec_store_log_stack;
void spec_store_log_stack_reset() {
  while (!spec_store_log_stack.empty()) { spec_store_log_stack_pop(); }
}
void spec_store_log_stack_push(store_log_t log) {
  spec_store_log_stack.push(log);
  IFDEF(CONFIG_RV_AME, store_log_ref(log);)
}
void spec_store_log_stack_pop() {
  IFDEF(CONFIG_RV_AME, store_log_unref(spec_store_log_stack.top());)
  spec_store_log_stack.pop();
}
store_log_t spec_store_log_stack_top() { return spec_store_log_stack.top();}
bool spec_store_log_stack_empty() { return spec_store_log_stack.empty();}
void spec_store_log_stack_copy() {
#ifdef CONFIG_RV_AME
  store_log_stack_reset();
#endif // CONFIG_RV_AME
  store_log_stack = spec_store_log_stack;
#ifdef CONFIG_RV_AME
  // This temporary copy is only a view used to visit the new owning entries.
  auto logs = store_log_stack;
  while (!logs.empty()) {
    store_log_ref(logs.top());
    logs.pop();
  }
#endif // CONFIG_RV_AME
}
#endif // CONFIG_LIGHTQS
#endif // CONFIG_STORE_LOG

#ifdef CONFIG_DIFFTEST_STORE_COMMIT

std::deque<store_commit_t> cpp_store_event_queue;
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
  cpp_store_event_queue.push_back(store_commit);
  if (cpp_store_event_queue.size() >= CONFIG_DIFFTEST_STORE_QUEUE_SIZE) {
    store_queue_has_overflow = true;
  }
}

void store_queue_pop() {
  cpp_store_event_queue.pop_front();
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

int store_queue_check_hash(uint64_t count, uint64_t hash_lo, uint64_t hash_hi,
                           uint64_t instr_begin, uint64_t instr_end) {
  if (store_queue_has_overflow) {
    printf("[StoreHash] NEMU store commit queue overflow.\n");
    return 1;
  }
  if (cpp_store_event_queue.size() < count) {
    printf("[StoreHash] NEMU queue underflow: need %lu records, have %zu.\n", count,
           cpp_store_event_queue.size());
    return 1;
  }

  uint64_t crc = 0;
  for (uint64_t i = 0; i < count; i++) {
    const store_commit_t &store_commit = cpp_store_event_queue[i];
    crc = difftest_store_hash_update(crc, store_commit.addr, store_commit.data, store_commit.mask);
  }

  if (crc == hash_lo && hash_hi == 0) {
    for (uint64_t i = 0; i < count; i++) {
      cpp_store_event_queue.pop_front();
    }
    return 0;
  }

  printf("[StoreHash] mismatch instr=[%lu,%lu] records=%lu\n", instr_begin, instr_end, count);
  printf("[StoreHash] expected DUT hash=(0x%016lx,0x%016lx), NEMU hash=(0x%016lx,0x%016lx)\n", hash_lo, hash_hi,
         crc, uint64_t(0));
  for (uint64_t i = 0; i < count; i++) {
    const store_commit_t &store_commit = cpp_store_event_queue[i];
    printf("[StoreHash] NEMU store[%lu] pc=0x%016lx addr=0x%016lx data=0x%016lx mask=0x%02x\n", i,
           store_commit.pc, store_commit.addr, store_commit.data, store_commit.mask);
  }
  return 1;
}

#endif
