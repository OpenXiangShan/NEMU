#include <cstdint>
#include <ame/mstore.h>
#include <ame/mstore_queue_wrapper.h>
#include <cpu/decode.h>

#ifdef CONFIG_RV_AME
#include <deque>
#include <cstring>

std::deque<mstore_info_t> cpp_mstore_queue;
extern Decode *prev_s;

void mstore_queue_reset() {
  cpp_mstore_queue = {};
}

void mstore_queue_push(mstore_info_t mstore_info) {
  cpp_mstore_queue.push_back(mstore_info);
}

void mstore_queue_pop() {
  cpp_mstore_queue.pop_front();
}

mstore_info_t mstore_queue_front() {
  auto mstore_info = cpp_mstore_queue.front();
  return mstore_info;
}

mstore_info_t mstore_queue_back() {
  auto mstore_info = cpp_mstore_queue.back();
  return mstore_info;
}

size_t mstore_queue_size() {
  return cpp_mstore_queue.size();
}

bool mstore_queue_empty() {
  return cpp_mstore_queue.empty();
}

#ifdef CONFIG_RV_AME_FP4
static bool ranges_overlap(uint64_t lhs_start, uint64_t lhs_len,
                           uint64_t rhs_start, uint64_t rhs_len) {
  if (lhs_start <= rhs_start) {
    return rhs_start - lhs_start < lhs_len;
  }
  return lhs_start - rhs_start < rhs_len;
}
#endif

void mstore_queue_emplace(uint64_t base_vaddr, uint64_t stride,
                          uint32_t row, uint32_t column, uint32_t msew,
                          bool transpose) {
  mstore_info_t mstore_info;
  mstore_info.base_vaddr = base_vaddr;
  mstore_info.stride = stride;
  mstore_info.pc = prev_s->pc;
  mstore_info.row = row;
  mstore_info.column = column;
  mstore_info.msew = msew;
  mstore_info.transpose = transpose;
  memset(mstore_info.valid, 0, sizeof(mstore_info.valid));
  mstore_queue_push(mstore_info);
}

void mstore_queue_update_mrelease(uint8_t tok_i, uint64_t mtokr_value) {
  assert(tok_i < MSYNC);
  if (cpp_mstore_queue.empty()) {
    return;
  }

  // Traverse from back to front using reverse iterator
  for (auto it = cpp_mstore_queue.rbegin(); it != cpp_mstore_queue.rend(); ++it) {
    if (!it->valid[tok_i]) {
      it->mrelease[tok_i] = mtokr_value;
      it->valid[tok_i] = true;
    } else {
      break;
    }
  }
}

void mstore_queue_update_acquire(uint8_t tok_i, uint64_t threshold) {
  assert(tok_i < MSYNC);
  // Traverse from front to back, pop items that satisfy the condition
  while (!cpp_mstore_queue.empty()) {
    if (cpp_mstore_queue.front().valid[tok_i] && cpp_mstore_queue.front().mrelease[tok_i] <= threshold) {
      cpp_mstore_queue.pop_front();
    } else {
      break;
    }
  }
}

bool mstore_queue_check_addr_conflict(uint64_t addr, int len) {
  if (cpp_mstore_queue.empty()) {
    return false;
  }

  uint64_t load_start = addr;
  uint64_t load_end = addr + len - 1;

  for (const auto& mstore : cpp_mstore_queue) {
    // Calculate memory access range for this matrix store
    uint32_t row_mem = mstore.transpose ? mstore.column : mstore.row;
    uint32_t column_mem = mstore.transpose ? mstore.row : mstore.column;

#ifdef CONFIG_RV_AME_FP4
    if (mstore.msew == MSEW_FP4) {
      if (row_mem == 0 || column_mem == 0) {
        continue;
      }
      uint64_t row_len = ((uint64_t)column_mem + 1) / 2;
      for (uint32_t row = 0; row < row_mem; row++) {
        if (row != 0 && mstore.stride > UINT64_MAX / row) {
          return true;
        }
        uint64_t offset = (uint64_t)row * mstore.stride;
        if (offset > UINT64_MAX - mstore.base_vaddr) {
          return true;
        }
        if (ranges_overlap(addr, (uint64_t)len,
                           mstore.base_vaddr + offset, row_len)) {
          return true;
        }
      }
      continue;
    }
#endif

    uint32_t width = 1 << mstore.msew;

    if (load_end < mstore.base_vaddr || load_start >= mstore.base_vaddr + row_mem * mstore.stride) {
      // Fast-path early exit: load is completely after or before this matrix store
      continue;
    } else if (row_mem == 0 || column_mem == 0) {
      // Skip if matrix is empty
      continue;
    }

    uint32_t start_row = (load_start - mstore.base_vaddr) / mstore.stride;
    uint32_t end_row = (load_end - mstore.base_vaddr) / mstore.stride;
    if (end_row != start_row && end_row < row_mem) {
      // There must be a conflict on end_row, 0
      return true;
    } else {
      // The same row, check the range

      // 每一行实际有效的数据范围是:
      //   row_base = base_vaddr + r * stride
      //   [row_base, row_base + column_mem * width - 1]
      // 行与行之间可能存在 padding，我们不把 padding 计入冲突范围。
      uint64_t col_bytes = (uint64_t)column_mem * width;

      uint64_t row_start = mstore.base_vaddr + (uint64_t)start_row * mstore.stride;
      uint64_t row_end = row_start + col_bytes - 1;

      // Check if load address range overlaps with this row's actual data range
      if (!(load_end < row_start || load_start > row_end)) {
        return true;  // Conflict found
      }
    }
  }

  return false;  // No conflict
}

#endif // CONFIG_RV_AME
