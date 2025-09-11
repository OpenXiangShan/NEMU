#include <ext/msync.h>
#include <ext/msync_queue_wrapper.h>
#include <cpu/decode.h>

#ifdef CONFIG_RVMATRIX
#include <queue>

std::queue<msync_event_t> cpp_msync_queue;
extern Decode *prev_s;

void msync_queue_reset() {
  cpp_msync_queue = {};
}

void msync_queue_push(msync_event_t msync_event) {
  cpp_msync_queue.push(msync_event);
}

void msync_queue_pop() {
  cpp_msync_queue.pop();
}

msync_event_t msync_queue_front() {
  auto msync_event = cpp_msync_queue.front();
  return msync_event;
}

msync_event_t msync_queue_back() {
  auto msync_event = cpp_msync_queue.back();
  return msync_event;
}

size_t msync_queue_size() {
  return cpp_msync_queue.size();
}

bool msync_queue_empty() {
  return cpp_msync_queue.empty();
}

void msync_queue_emplace(uint8_t op, uint8_t token) {
  msync_event_t msync_event;
  msync_event.valid = true;
  msync_event.op = op;
  msync_event.token = token;
  msync_event.pc = prev_s->pc;
  msync_queue_push(msync_event);
}

#endif // CONFIG_RVMATRIX
