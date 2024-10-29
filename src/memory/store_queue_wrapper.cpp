#include <memory/store_queue_wrapper.h>

#ifdef CONFIG_DIFFTEST_STORE_COMMIT

#include <queue>

std::queue<store_commit_t> cpp_store_event_queue;

void store_queue_reset() {
  cpp_store_event_queue = {};
}

void store_queue_push(store_commit_t store_commit) {
  cpp_store_event_queue.push(store_commit);
}

void store_queue_pop() {
  cpp_store_event_queue.pop();
}

store_commit_t store_queue_fornt() {
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
