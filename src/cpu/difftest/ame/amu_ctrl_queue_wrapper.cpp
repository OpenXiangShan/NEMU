#include <cpu/difftest/ame/amu_ctrl_queue_wrapper.h>
#include <cpu/decode.h>
#include <isa.h>

#if defined(CONFIG_RV_AME) && defined(CONFIG_SHARE_REF)
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <utility>
#include <vector>

struct OwnedAmuCtrl {
  amu_ctrl_event_t event;
  uint64_t epoch;
  amu_scale_metadata_t scales[2];
  std::vector<uint8_t> bytes[2];
  std::vector<uint8_t> expected_tr;
  uint32_t raised_flags;
  bool delivered;
};

std::deque<OwnedAmuCtrl> cpp_amu_ctrl_queue;
extern Decode *prev_s;

static void queue_invariant_failure(const char *message) {
  std::fprintf(stderr, "NEMU AMU queue invariant failed: %s\n", message);
  std::abort();
}

static auto pending_head() {
  return std::find_if(cpp_amu_ctrl_queue.begin(), cpp_amu_ctrl_queue.end(),
    [](const OwnedAmuCtrl &owned) { return !owned.delivered; });
}

static bool is_mx_mma(const amu_ctrl_event_t &event) {
  return event.op == 0 && (event.computeType == 7 || event.computeType == 8 ||
                           event.computeType == 10);
}

static bool needs_lazy_snapshot(const OwnedAmuCtrl &owned) {
  return owned.event.isDynamicMxfpQuant || is_mx_mma(owned.event);
}

static auto oldest_delivered_snapshot() {
  return std::find_if(cpp_amu_ctrl_queue.begin(), cpp_amu_ctrl_queue.end(),
    [](const OwnedAmuCtrl &owned) { return owned.delivered && needs_lazy_snapshot(owned); });
}

static void capture_scale(OwnedAmuCtrl *owned, unsigned side, unsigned reg,
                          bool generated) {
  if (reg >= 4) return;
  const unsigned index = side * 4 + reg;
  amu_scale_metadata_t *metadata = &owned->scales[side];
  metadata->reg_id = reg;
  metadata->side = side;
  metadata->valid = cpu.mscale_valid[index];
  metadata->rows = cpu.mscale_rows[index];
  metadata->blocks = cpu.mscale_blocks[index];
  metadata->row_pitch = metadata->blocks;
  metadata->generation = cpu.mscale_generation[index];
  metadata->source_stride = cpu.mscale_source_stride[index];
  if (metadata->rows > ROWNUM || metadata->blocks > AME_MAX_SCALE_BLOCKS ||
      !metadata->blocks || (!metadata->valid && !generated)) {
    metadata->valid = 0;
    return;
  }
  const size_t byte_count = size_t(metadata->rows) * metadata->blocks;
  owned->bytes[side].resize(byte_count);
  for (size_t row = 0; row < metadata->rows; ++row) {
    memcpy(owned->bytes[side].data() + row * metadata->blocks,
      cpu.mscale[index][row], metadata->blocks);
  }
}

void amu_ctrl_queue_reset() {
  cpp_amu_ctrl_queue.clear();
}

void amu_ctrl_queue_push(amu_ctrl_event_t store_commit) {
  amu_ctrl_queue_push_snapshot(store_commit, 0, nullptr);
}

void amu_ctrl_queue_push_snapshot(amu_ctrl_event_t store_commit, uint32_t raised_flags,
                                  const amu_matrix_view_t *expected_tr) {
  OwnedAmuCtrl owned = {};
  owned.event = store_commit;
  owned.epoch = cpu.mscale_epoch;
  owned.raised_flags = raised_flags;
  const bool dynamic = store_commit.isDynamicMxfpQuant;
  if (dynamic) {
    const unsigned rows = store_commit.types2 ? store_commit.mtilem : store_commit.mtilen;
    const unsigned k = store_commit.types2 ? store_commit.mtilen : store_commit.mtilem;
    if (store_commit.op != 1 || store_commit.isConvertF32 != 1 || store_commit.isScale ||
        store_commit.sat || store_commit.isfp || store_commit.types1 ||
        store_commit.types2 > 1 || store_commit.md >= 4 || store_commit.computeType != 10 ||
        store_commit.typed != 2 || rows != 8 || k != 64 ||
        (store_commit.base & 3) || store_commit.stride < 256 ||
        (store_commit.stride & 3) || (raised_flags != 0 && raised_flags != 0x10) ||
        !expected_tr || !expected_tr->bytes || expected_tr->capacity < 256 ||
        expected_tr->row_pitch != 32 || expected_tr->reserved) {
      queue_invariant_failure("invalid dynamic result owner");
    }
    owned.expected_tr.resize(256);
    for (unsigned row = 0; row < 8; ++row) {
      memcpy(owned.expected_tr.data() + row * 32,
        expected_tr->bytes + row * expected_tr->row_pitch, 32);
    }
  } else if (raised_flags || expected_tr) {
    queue_invariant_failure("producer-local status requires a dynamic result owner");
  }

  for (unsigned side = 0; side < 2; ++side) {
    const bool scale_load = store_commit.op == 1 && store_commit.isScale &&
      bool(store_commit.types2) == (side == 0);
    const bool generated = dynamic && bool(store_commit.types2) == (side == 0);
    if (!is_mx_mma(store_commit) && !scale_load && !generated) continue;
    const unsigned reg = scale_load || generated ? store_commit.md :
      (side == 0 ? store_commit.ms1 : store_commit.ms2);
    capture_scale(&owned, side, reg, generated);
    amu_scale_metadata_t *metadata = &owned.scales[side];
    if (is_mx_mma(store_commit)) {
      const unsigned rows = side ? store_commit.mtilen : store_commit.mtilem;
      const unsigned blocks = (store_commit.mtilek + AME_MX_BLOCK_SIZE - 1) /
        AME_MX_BLOCK_SIZE;
      const unsigned index = side * 4 + reg;
      if (!metadata->valid || !metadata->generation || metadata->rows != rows ||
          metadata->blocks != blocks || metadata->row_pitch != blocks ||
          metadata->reg_id != reg || metadata->side != side || metadata->reserved ||
          metadata->source_stride < blocks || cpu.mscale_mode[index] != store_commit.computeType ||
          owned.bytes[side].size() != size_t(rows) * blocks) {
        queue_invariant_failure("scaled MMA scale metadata does not match its active tile");
      }
    }
    if (generated && (metadata->generation == 0 || metadata->rows != 8 ||
        metadata->blocks != 2 || metadata->source_stride != store_commit.stride ||
        metadata->valid != (raised_flags == 0))) {
      queue_invariant_failure("dynamic scale metadata disagrees with local completion status");
    }
  }
  cpp_amu_ctrl_queue.push_back(std::move(owned));
}

void amu_ctrl_queue_pop() {
  auto pending = pending_head();
  if (pending != cpp_amu_ctrl_queue.end()) cpp_amu_ctrl_queue.erase(pending);
}

amu_ctrl_event_t amu_ctrl_queue_front() {
  auto pending = pending_head();
  return pending == cpp_amu_ctrl_queue.end() ? amu_ctrl_event_t{} : pending->event;
}

amu_ctrl_event_t amu_ctrl_queue_back() {
  for (auto iter = cpp_amu_ctrl_queue.rbegin(); iter != cpp_amu_ctrl_queue.rend(); ++iter) {
    if (!iter->delivered) return iter->event;
  }
  return amu_ctrl_event_t{};
}

amu_ctrl_event_t amu_ctrl_queue_at(size_t index) {
  for (const auto &owned : cpp_amu_ctrl_queue) {
    if (owned.delivered) continue;
    if (index == 0) return owned.event;
    --index;
  }
  return amu_ctrl_event_t{};
}

size_t amu_ctrl_queue_size() {
  return std::count_if(cpp_amu_ctrl_queue.begin(), cpp_amu_ctrl_queue.end(),
    [](const OwnedAmuCtrl &owned) { return !owned.delivered; });
}

bool amu_ctrl_queue_empty() {
  return pending_head() == cpp_amu_ctrl_queue.end();
}

int amu_ctrl_queue_copy_snapshot(amu_ctrl_snapshot_t *snapshot) {
  auto pending = pending_head();
  if (!snapshot || snapshot->version != AMU_CTRL_SNAPSHOT_VERSION ||
      snapshot->struct_size != sizeof(*snapshot) || snapshot->reserved ||
      snapshot->expected_tr.reserved || pending == cpp_amu_ctrl_queue.end()) return -2;
  const OwnedAmuCtrl &owned = *pending;
  for (unsigned side = 0; side < 2; ++side) {
    if (snapshot->scales[side].capacity < owned.bytes[side].size() ||
        (!owned.bytes[side].empty() && !snapshot->scales[side].bytes)) return -2;
  }
  if (!owned.expected_tr.empty() &&
      (!snapshot->expected_tr.bytes || snapshot->expected_tr.capacity < owned.expected_tr.size() ||
       snapshot->expected_tr.row_pitch != 32)) return -2;
  snapshot->epoch = owned.epoch;
  snapshot->raised_flags = owned.raised_flags;
  for (unsigned side = 0; side < 2; ++side) {
    snapshot->scales[side].metadata = owned.scales[side];
    if (!owned.bytes[side].empty()) {
      memcpy(snapshot->scales[side].bytes, owned.bytes[side].data(), owned.bytes[side].size());
    }
  }
  if (!owned.expected_tr.empty()) {
    memcpy(snapshot->expected_tr.bytes, owned.expected_tr.data(), owned.expected_tr.size());
  }
  snapshot->expected_tr.row_pitch = owned.expected_tr.empty() ? 0 : 32;
  return 0;
}

void amu_ctrl_queue_complete_pending() {
  auto pending = pending_head();
  if (pending == cpp_amu_ctrl_queue.end()) return;
  if (needs_lazy_snapshot(*pending)) pending->delivered = true;
  else cpp_amu_ctrl_queue.erase(pending);
}

static bool same_dynamic_event(const amu_ctrl_event_t &left,
                               const amu_ctrl_event_t &right) {
  const bool same_mls = left.op == 1 && right.op == 1 &&
    left.md == right.md && left.sat == right.sat && left.isfp == right.isfp &&
    left.types1 == right.types1 && left.types2 == right.types2 &&
    left.base == right.base && left.stride == right.stride &&
    left.mtilem == right.mtilem && left.mtilen == right.mtilen &&
    left.typed == right.typed;
  const bool same_metadata = left.computeType == right.computeType &&
    left.isScale == right.isScale && left.isConvertF32 == right.isConvertF32 &&
    left.isDynamicMxfpQuant == right.isDynamicMxfpQuant;
  return left.valid == right.valid && same_mls && left.pc == right.pc && same_metadata;
}

int amu_ctrl_queue_apply_dynamic_snapshot(const amu_ctrl_event_t *event,
                                          amu_mma_snapshot_t *snapshot) {
  const unsigned rows = event && event->types2 ? event->mtilem : event ? event->mtilen : 0;
  const unsigned k = event && event->types2 ? event->mtilen : event ? event->mtilem : 0;
  if (!amu_snapshot_ready() || !event || !snapshot ||
      snapshot->version != AMU_MMA_SNAPSHOT_VERSION ||
      snapshot->struct_size != sizeof(*snapshot) || snapshot->reserved || !snapshot->scales ||
      snapshot->scales->version != AMU_CTRL_SNAPSHOT_VERSION ||
      snapshot->scales->struct_size != sizeof(*snapshot->scales) || snapshot->scales->reserved ||
      event->op != 1 || event->isConvertF32 != 1 || event->isDynamicMxfpQuant != 1 ||
      event->isScale || event->sat || event->isfp || event->types1 || event->types2 > 1 ||
      event->md >= 4 || event->computeType != 10 || event->typed != 2 || rows != 8 || k != 64 ||
      (event->base & 3) || event->stride < 256 || (event->stride & 3)) return -2;
  for (unsigned index = 0; index < 4; ++index) {
    const amu_matrix_view_t &view = snapshot->matrices[index];
    if (view.bytes || view.capacity || view.row_pitch || view.reserved) return -2;
  }

  const unsigned side = event->types2 ? 0 : 1;
  auto delivered = oldest_delivered_snapshot();
  if (delivered == cpp_amu_ctrl_queue.end()) return -1;
  for (auto iter = cpp_amu_ctrl_queue.begin(); iter != delivered; ++iter) {
    if (!iter->delivered) return -1;
  }
  if (!same_dynamic_event(delivered->event, *event) || delivered->expected_tr.size() != 256) return 1;

  const amu_ctrl_snapshot_t *owned_scales = snapshot->scales;
  const amu_scale_metadata_t &expected = delivered->scales[side];
  const amu_scale_metadata_t &actual = owned_scales->scales[side].metadata;
  const amu_scale_snapshot_t &owned_scale = owned_scales->scales[side];
  const amu_matrix_view_t &expected_tr = owned_scales->expected_tr;
  if (owned_scales->epoch != delivered->epoch ||
      owned_scales->raised_flags != delivered->raised_flags ||
      actual.generation != expected.generation || actual.rows != expected.rows ||
      actual.blocks != expected.blocks || actual.row_pitch != expected.row_pitch ||
      actual.reg_id != expected.reg_id || actual.side != expected.side ||
      actual.valid != expected.valid || actual.reserved ||
      actual.source_stride != expected.source_stride || !owned_scale.bytes ||
      owned_scale.capacity < delivered->bytes[side].size() ||
      (delivered->bytes[side].size() &&
       memcmp(owned_scale.bytes, delivered->bytes[side].data(), delivered->bytes[side].size())) ||
      !expected_tr.bytes || expected_tr.capacity < delivered->expected_tr.size() ||
      expected_tr.row_pitch != 32 || expected_tr.reserved ||
      memcmp(expected_tr.bytes, delivered->expected_tr.data(), delivered->expected_tr.size())) return 1;
  for (unsigned other = 0; other < 2; ++other) {
    if (other == side) continue;
    const amu_scale_metadata_t &unused = owned_scales->scales[other].metadata;
    if (unused.valid || unused.generation || unused.rows || unused.blocks || unused.row_pitch ||
        unused.reg_id || unused.side || unused.reserved || unused.source_stride) return 1;
  }
  for (unsigned row = 0; row < 8; ++row) {
    memcpy(cpu.mtr[delivered->event.md][row]._8,
      delivered->expected_tr.data() + row * 32, 32);
  }
  snapshot->raised_flags = delivered->raised_flags;
  cpp_amu_ctrl_queue.erase(delivered);
  return 0;
}

static bool same_scaled_mma_event(const amu_ctrl_event_t &left,
                                  const amu_ctrl_event_t &right) {
  return left.valid == right.valid && left.op == 0 && right.op == 0 &&
    left.rm == right.rm && left.md == right.md && left.sat == right.sat &&
    left.isfp == right.isfp && left.ms1 == right.ms1 && left.ms2 == right.ms2 &&
    left.mtilem == right.mtilem && left.mtilen == right.mtilen &&
    left.mtilek == right.mtilek && left.types1 == right.types1 &&
    left.types2 == right.types2 && left.typed == right.typed &&
    left.base == right.base && left.stride == right.stride && left.pc == right.pc &&
    left.computeType == right.computeType && left.isScale == right.isScale &&
    left.isConvertF32 == right.isConvertF32 &&
    left.isDynamicMxfpQuant == right.isDynamicMxfpQuant;
}

int amu_ctrl_queue_apply_scaled_mma_snapshot(const amu_ctrl_event_t *event,
                                             amu_mma_snapshot_t *snapshot) {
  if (!amu_snapshot_ready() || !event || !snapshot ||
      snapshot->version != AMU_MMA_SNAPSHOT_VERSION ||
      snapshot->struct_size != sizeof(*snapshot) || snapshot->reserved || !snapshot->scales ||
      snapshot->scales->version != AMU_CTRL_SNAPSHOT_VERSION ||
      snapshot->scales->struct_size != sizeof(*snapshot->scales) || snapshot->scales->reserved ||
      !is_mx_mma(*event)) return -2;

  auto delivered = oldest_delivered_snapshot();
  if (delivered == cpp_amu_ctrl_queue.end()) return -1;
  for (auto iter = cpp_amu_ctrl_queue.begin(); iter != delivered; ++iter) {
    if (!iter->delivered) return -1;
  }
  if (!same_scaled_mma_event(delivered->event, *event) ||
      snapshot->scales->epoch != delivered->epoch || snapshot->scales->raised_flags ||
      snapshot->scales->expected_tr.bytes || snapshot->scales->expected_tr.capacity ||
      snapshot->scales->expected_tr.row_pitch || snapshot->scales->expected_tr.reserved) return 1;

  const unsigned blocks = (event->mtilek + AME_MX_BLOCK_SIZE - 1) / AME_MX_BLOCK_SIZE;
  for (unsigned side = 0; side < 2; ++side) {
    const unsigned rows = side ? event->mtilen : event->mtilem;
    const size_t byte_count = size_t(rows) * blocks;
    const amu_scale_snapshot_t &scale = snapshot->scales->scales[side];
    const amu_scale_metadata_t &actual = scale.metadata;
    const amu_scale_metadata_t &expected = delivered->scales[side];
    if (!scale.bytes || scale.capacity < byte_count ||
        delivered->bytes[side].size() != byte_count ||
        actual.generation != expected.generation || actual.rows != expected.rows ||
        actual.blocks != expected.blocks || actual.row_pitch != expected.row_pitch ||
        actual.reg_id != expected.reg_id || actual.side != expected.side ||
        actual.valid != expected.valid || actual.reserved ||
        actual.source_stride != expected.source_stride ||
        memcmp(scale.bytes, delivered->bytes[side].data(), byte_count) != 0) return 1;
  }

  const int result = exec_amu_scaled_mma_snapshot(event,
    delivered->bytes[0].data(), delivered->bytes[0].size(),
    delivered->bytes[1].data(), delivered->bytes[1].size(), snapshot);
  if (result != 0) return result;
  cpp_amu_ctrl_queue.erase(delivered);
  return 0;
}

void amu_ctrl_queue_mma_emplace(uint8_t md, uint8_t rm, bool sat, bool isfp,
                                uint8_t ms1, uint8_t ms2,
                                uint16_t mtilem, uint16_t mtilen, uint16_t mtilek,
                                uint8_t types1, uint8_t types2, uint8_t typed,
                                uint8_t compute_type) {
  amu_ctrl_event_t event = {};
  event.valid = true;
  event.op = 0;
  event.rm = rm;
  event.md = md;
  event.sat = sat;
  event.ms1 = ms1;
  event.ms2 = ms2;
  event.mtilem = mtilem;
  event.mtilen = mtilen;
  event.mtilek = mtilek;
  event.types1 = types1;
  event.types2 = types2;
  event.typed = typed;
  event.isfp = isfp;
  event.computeType = compute_type;
  event.pc = prev_s->pc;
  amu_ctrl_queue_push(event);
}

void amu_ctrl_queue_mls_emplace(uint8_t ms, bool ls, bool transpose, bool isacc,
                                bool isA, uint64_t base, uint64_t stride,
                                uint16_t row, uint16_t column, uint8_t msew) {
  amu_ctrl_event_t event = {};
  event.valid = true;
  event.op = 1;
  event.md = ms;
  event.sat = ls;
  event.isfp = transpose;
  event.types1 = isacc;
  event.types2 = isA;
  event.base = base;
  event.stride = stride;
  event.mtilem = row;
  event.mtilen = column;
  event.typed = msew;
  event.pc = prev_s->pc;
  amu_ctrl_queue_push(event);
}

void amu_ctrl_queue_mrelease_emplace(uint8_t msyncRd) {
  amu_ctrl_event_t event = {};
  event.op = 2;
  event.mtilem = msyncRd;
  event.pc = prev_s->pc;
  amu_ctrl_queue_push(event);
}

void amu_ctrl_queue_mzero_emplace(bool isacc, uint8_t md) {
  amu_ctrl_event_t event = {};
  event.op = 3;
  event.md = md;
  event.base = 0x1b8;
  event.pc = prev_s->pc;
  amu_ctrl_queue_push(event);
}

#endif // defined(CONFIG_RV_AME) && defined(CONFIG_SHARE_REF)
