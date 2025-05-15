#include <ext/amuctrl.h>
#include <ext/amu_ctrl_queue_wrapper.h>

#ifdef CONFIG_RVMATRIX
#include <queue>

std::queue<amu_ctrl_event_t> cpp_amu_ctrl_queue;

void amu_ctrl_queue_reset() {
  cpp_amu_ctrl_queue = {};
}

void amu_ctrl_queue_push(amu_ctrl_event_t store_commit) {
  cpp_amu_ctrl_queue.push(store_commit);
}

void amu_ctrl_queue_pop() {
  cpp_amu_ctrl_queue.pop();
}

amu_ctrl_event_t amu_ctrl_queue_front() {
  auto store_commit = cpp_amu_ctrl_queue.front();
  return store_commit;
}

amu_ctrl_event_t amu_ctrl_queue_back() {
  auto store_commit = cpp_amu_ctrl_queue.back();
  return store_commit;
}

size_t amu_ctrl_queue_size() {
  return cpp_amu_ctrl_queue.size();
}

bool amu_ctrl_queue_empty() {
  return cpp_amu_ctrl_queue.empty();
}

void amu_ctrl_queue_mma_emplace(uint8_t md, bool sat, uint8_t ms1, uint8_t ms2,
                                uint16_t mtilem, uint16_t mtilen, uint16_t mtilek,
                                uint8_t types, uint8_t typed) {
  amu_ctrl_event_t event;
  event.valid = true;
  event.op = 0;
  event.md = md;
  event.sat = sat;
  event.ms1 = ms1;
  event.ms2 = ms2;
  event.mtilem = mtilem;
  event.mtilen = mtilen;
  event.mtilek = mtilek;
  event.types = types;
  event.typed = typed;
  amu_ctrl_queue_push(event);
}

void amu_ctrl_queue_mls_emplace(uint8_t ms, bool ls, bool transpose, bool isacc,
                                uint64_t base, uint64_t stride,
                                uint16_t row, uint16_t column, uint8_t msew) {
  amu_ctrl_event_t event;
  event.valid = true;
  event.op = 1;
  event.md = ms;
  event.sat = ls;
  event.base = base;
  event.stride = stride;
  event.transpose = transpose;
  event.isacc = isacc;
  event.mtilem = row;
  event.mtilen = column;
  event.types = msew;
  amu_ctrl_queue_push(event);
}

#endif // CONFIG_RVMATRIX
