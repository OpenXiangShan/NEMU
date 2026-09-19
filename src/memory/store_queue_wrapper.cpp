#include <memory/store_queue_wrapper.h>
#include <algorithm>
#include <queue>
#include <stack>
#include <vector>

#ifdef CONFIG_STORE_LOG
std::stack<store_log_t> store_log_stack;
void store_log_stack_reset() { store_log_stack = {};}
void store_log_stack_push(store_log_t log) { store_log_stack.push(log);}
void store_log_stack_pop() { store_log_stack.pop();}
store_log_t store_log_stack_top() {return store_log_stack.top();}
bool store_log_stack_empty() { return store_log_stack.empty();}
std::vector<difftest_store_log_entry_t> store_effect_log;
void store_effect_log_reset() { store_effect_log.clear(); }
void store_effect_log_push(difftest_store_log_entry_t log) { store_effect_log.push_back(log); }
size_t store_effect_log_size() { return store_effect_log.size(); }
size_t store_effect_log_copy(difftest_store_log_entry_t *dest, size_t capacity) {
  size_t count = std::min(capacity, store_effect_log.size());
  std::copy_n(store_effect_log.begin(), count, dest);
  return count;
}
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

#endif
