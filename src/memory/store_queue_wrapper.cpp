#include <memory/store_queue_wrapper.h>
#include <cstdio>
#include <cstdlib>
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

static bool store_diff_trace_enabled() {
  static int enabled = -1;
  if (enabled < 0) {
    const char *env = getenv("STORE_DIFF_TRACE");
    enabled = env == nullptr || env[0] != '0';
  }
  return enabled;
}

static uint64_t nemu_store_queue_push_seq = 0;

void store_queue_reset() {
  if (store_diff_trace_enabled()) {
    printf("STORE_DIFF_TRACE side=NEMU stage=queue_reset q_size_before=%zu\n", cpp_store_event_queue.size());
    fflush(stdout);
  }
  cpp_store_event_queue = {};
}

void store_queue_push(store_commit_t store_commit) {
  size_t q_size_before = cpp_store_event_queue.size();
  Logm("push store addr = " FMT_PADDR ", data = " FMT_WORD ", mask = 0x%x", store_commit.addr, store_commit.data, store_commit.mask);
  cpp_store_event_queue.push(store_commit);
  if (store_diff_trace_enabled()) {
    printf("STORE_DIFF_TRACE side=NEMU stage=queue_push seq=%llu q_size_before=%zu q_size_after=%zu pc=0x%016llx addr=0x%016llx data=0x%016llx mask=0x%02x\n",
        (unsigned long long)++nemu_store_queue_push_seq,
        q_size_before,
        cpp_store_event_queue.size(),
        (unsigned long long)store_commit.pc,
        (unsigned long long)store_commit.addr,
        (unsigned long long)store_commit.data,
        store_commit.mask);
    fflush(stdout);
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

#endif
