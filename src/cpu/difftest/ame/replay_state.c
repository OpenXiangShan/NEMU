#include <ame/mstore_queue_wrapper.h>
#include <ame/svstore_queue_wrapper.h>
#include <cpu/difftest/ame/amu_ctrl_queue_wrapper.h>
#include <cpu/difftest/ame/msync_queue_wrapper.h>
#include <isa.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#if defined(CONFIG_RV_AME) && defined(CONFIG_SHARE_REF)

// Difftest owns this ephemeral buffer and only returns it to the same loaded
// REF instance. The native entry encoding is not a persistent ABI.
typedef struct {
  size_t mstore;
  size_t svstore;
  size_t amu_ctrl;
  size_t msync;
} AmeReplayStateCounts;

static size_t calculate_state_size(const AmeReplayStateCounts *counts) {
  size_t total =
      sizeof(*counts) + sizeof(cpu.mtr) + sizeof(cpu.macc) + sizeof(cpu.mtokr);
#ifdef CONFIG_AME_TILEREG_UB_CHECK
  total += ame_matrix_region_state_size();
#endif // CONFIG_AME_TILEREG_UB_CHECK
#ifdef CONFIG_AME_MEM_ACCESS_CHECK
  total += counts->mstore * sizeof(mstore_info_t);
  total += counts->svstore * sizeof(svstore_info_t);
#endif // CONFIG_AME_MEM_ACCESS_CHECK
  total += counts->amu_ctrl * sizeof(amu_ctrl_event_t);
  total += counts->msync * sizeof(msync_event_t);
  return total;
}

static AmeReplayStateCounts get_state_counts(void) {
  AmeReplayStateCounts counts = {0};
#ifdef CONFIG_AME_MEM_ACCESS_CHECK
  counts.mstore = mstore_queue_size();
  counts.svstore = svstore_queue_size();
#endif // CONFIG_AME_MEM_ACCESS_CHECK
  counts.amu_ctrl = amu_ctrl_queue_size();
  counts.msync = msync_queue_size();
  return counts;
}

static void write_value(uint8_t **cursor, const void *value, size_t size) {
  memcpy(*cursor, value, size);
  *cursor += size;
}

#ifdef CONFIG_AME_MEM_ACCESS_CHECK
static void write_mstore_queue(uint8_t **cursor, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    mstore_info_t entry = mstore_queue_front();
    mstore_queue_pop();
    write_value(cursor, &entry, sizeof(entry));
    mstore_queue_push(entry);
  }
}

static void write_svstore_queue(uint8_t **cursor, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    svstore_info_t entry = svstore_queue_front();
    svstore_queue_pop();
    write_value(cursor, &entry, sizeof(entry));
    svstore_queue_push(entry);
  }
}
#endif // CONFIG_AME_MEM_ACCESS_CHECK

static void write_amu_ctrl_queue(uint8_t **cursor, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    amu_ctrl_event_t entry = amu_ctrl_queue_front();
    amu_ctrl_queue_pop();
    write_value(cursor, &entry, sizeof(entry));
    amu_ctrl_queue_push(entry);
  }
}

static void write_msync_queue(uint8_t **cursor, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    msync_event_t entry = msync_queue_front();
    msync_queue_pop();
    write_value(cursor, &entry, sizeof(entry));
    msync_queue_push(entry);
  }
}

static void read_value(const uint8_t **cursor, void *value, size_t size) {
  memcpy(value, *cursor, size);
  *cursor += size;
}

#ifdef CONFIG_AME_MEM_ACCESS_CHECK
static void restore_mstore_queue(const uint8_t **cursor, size_t count) {
  mstore_queue_reset();
  for (size_t i = 0; i < count; ++i) {
    mstore_info_t entry;
    read_value(cursor, &entry, sizeof(entry));
    mstore_queue_push(entry);
  }
}

static void restore_svstore_queue(const uint8_t **cursor, size_t count) {
  svstore_queue_reset();
  for (size_t i = 0; i < count; ++i) {
    svstore_info_t entry;
    read_value(cursor, &entry, sizeof(entry));
    svstore_queue_push(entry);
  }
}
#endif // CONFIG_AME_MEM_ACCESS_CHECK

static void restore_amu_ctrl_queue(const uint8_t **cursor, size_t count) {
  amu_ctrl_queue_reset();
  for (size_t i = 0; i < count; ++i) {
    amu_ctrl_event_t entry;
    read_value(cursor, &entry, sizeof(entry));
    amu_ctrl_queue_push(entry);
  }
}

static void restore_msync_queue(const uint8_t **cursor, size_t count) {
  msync_queue_reset();
  for (size_t i = 0; i < count; ++i) {
    msync_event_t entry;
    read_value(cursor, &entry, sizeof(entry));
    msync_queue_push(entry);
  }
}

void save_ame_replay_state(void *state) {
  const AmeReplayStateCounts counts = get_state_counts();
  uint8_t *cursor = state;
  write_value(&cursor, &counts, sizeof(counts));
  write_value(&cursor, cpu.mtr, sizeof(cpu.mtr));
  write_value(&cursor, cpu.macc, sizeof(cpu.macc));
  write_value(&cursor, cpu.mtokr, sizeof(cpu.mtokr));
#ifdef CONFIG_AME_TILEREG_UB_CHECK
  ame_matrix_region_state_save(cursor);
  cursor += ame_matrix_region_state_size();
#endif // CONFIG_AME_TILEREG_UB_CHECK
#ifdef CONFIG_AME_MEM_ACCESS_CHECK
  write_mstore_queue(&cursor, counts.mstore);
  write_svstore_queue(&cursor, counts.svstore);
#endif // CONFIG_AME_MEM_ACCESS_CHECK
  write_amu_ctrl_queue(&cursor, counts.amu_ctrl);
  write_msync_queue(&cursor, counts.msync);
}

void restore_ame_replay_state(const void *state) {
  const uint8_t *cursor = state;
  AmeReplayStateCounts counts;
  read_value(&cursor, &counts, sizeof(counts));

  read_value(&cursor, cpu.mtr, sizeof(cpu.mtr));
  read_value(&cursor, cpu.macc, sizeof(cpu.macc));
  read_value(&cursor, cpu.mtokr, sizeof(cpu.mtokr));
#ifdef CONFIG_AME_TILEREG_UB_CHECK
  ame_matrix_region_state_restore(cursor);
  cursor += ame_matrix_region_state_size();
#endif // CONFIG_AME_TILEREG_UB_CHECK
#ifdef CONFIG_AME_MEM_ACCESS_CHECK
  restore_mstore_queue(&cursor, counts.mstore);
  restore_svstore_queue(&cursor, counts.svstore);
#endif // CONFIG_AME_MEM_ACCESS_CHECK
  restore_amu_ctrl_queue(&cursor, counts.amu_ctrl);
  restore_msync_queue(&cursor, counts.msync);
}

size_t difftest_ame_replay_state_size(void) {
  const AmeReplayStateCounts counts = get_state_counts();
  return calculate_state_size(&counts);
}

#endif // CONFIG_RV_AME && CONFIG_SHARE_REF
