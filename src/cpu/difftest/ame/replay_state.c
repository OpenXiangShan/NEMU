#include <ame/mstore_queue_wrapper.h>
#include <cpu/difftest/ame/amu_ctrl_queue_wrapper.h>
#include <cpu/difftest/ame/msync_queue_wrapper.h>
#include <isa.h>
#include <memory/store_queue_wrapper.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#if defined(CONFIG_RV_AME) && defined(CONFIG_SHARE_REF)

// Difftest owns this ephemeral buffer and only returns it to the same loaded
// REF instance. The native entry encoding is not a persistent ABI.
typedef struct {
  size_t matrix_store;
  size_t mstore;
  size_t amu_ctrl;
  size_t msync;
} AmeReplayStateCounts;

static size_t calculate_state_size(const AmeReplayStateCounts *counts) {
  size_t total =
      sizeof(*counts) + sizeof(cpu.mtr) + sizeof(cpu.macc) + sizeof(cpu.mtokr);
#ifdef CONFIG_DIFFTEST_STORE_COMMIT
  total += counts->matrix_store * sizeof(matrix_store_commit_t);
#endif // CONFIG_DIFFTEST_STORE_COMMIT
#ifdef CONFIG_AME_MSTORE_ACCESS_CHECK
  total += counts->mstore * sizeof(mstore_info_t);
#endif // CONFIG_AME_MSTORE_ACCESS_CHECK
  total += counts->amu_ctrl * sizeof(amu_ctrl_event_t);
  total += counts->msync * sizeof(msync_event_t);
  return total;
}

static AmeReplayStateCounts get_state_counts(void) {
  AmeReplayStateCounts counts = {0};
#ifdef CONFIG_DIFFTEST_STORE_COMMIT
  counts.matrix_store = matrix_store_queue_size();
#endif // CONFIG_DIFFTEST_STORE_COMMIT
#ifdef CONFIG_AME_MSTORE_ACCESS_CHECK
  counts.mstore = mstore_queue_size();
#endif // CONFIG_AME_MSTORE_ACCESS_CHECK
  counts.amu_ctrl = amu_ctrl_queue_size();
  counts.msync = msync_queue_size();
  return counts;
}

static void write_value(uint8_t **cursor, const void *value, size_t size) {
  memcpy(*cursor, value, size);
  *cursor += size;
}

#ifdef CONFIG_DIFFTEST_STORE_COMMIT
static void write_matrix_store_queue(uint8_t **cursor, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    matrix_store_commit_t entry = matrix_store_queue_front();
    matrix_store_queue_pop();
    write_value(cursor, &entry, sizeof(entry));
    matrix_store_queue_push(entry);
  }
}
#endif // CONFIG_DIFFTEST_STORE_COMMIT

#ifdef CONFIG_AME_MSTORE_ACCESS_CHECK
static void write_mstore_queue(uint8_t **cursor, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    mstore_info_t entry = mstore_queue_front();
    mstore_queue_pop();
    write_value(cursor, &entry, sizeof(entry));
    mstore_queue_push(entry);
  }
}
#endif // CONFIG_AME_MSTORE_ACCESS_CHECK

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

#ifdef CONFIG_DIFFTEST_STORE_COMMIT
static void restore_matrix_store_queue(const uint8_t **cursor, size_t count) {
  matrix_store_queue_reset();
  for (size_t i = 0; i < count; ++i) {
    matrix_store_commit_t entry;
    read_value(cursor, &entry, sizeof(entry));
    matrix_store_queue_push(entry);
  }
}
#endif // CONFIG_DIFFTEST_STORE_COMMIT

#ifdef CONFIG_AME_MSTORE_ACCESS_CHECK
static void restore_mstore_queue(const uint8_t **cursor, size_t count) {
  mstore_queue_reset();
  for (size_t i = 0; i < count; ++i) {
    mstore_info_t entry;
    read_value(cursor, &entry, sizeof(entry));
    mstore_queue_push(entry);
  }
}
#endif // CONFIG_AME_MSTORE_ACCESS_CHECK

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
#ifdef CONFIG_DIFFTEST_STORE_COMMIT
  write_matrix_store_queue(&cursor, counts.matrix_store);
#endif // CONFIG_DIFFTEST_STORE_COMMIT
#ifdef CONFIG_AME_MSTORE_ACCESS_CHECK
  write_mstore_queue(&cursor, counts.mstore);
#endif // CONFIG_AME_MSTORE_ACCESS_CHECK
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
#ifdef CONFIG_DIFFTEST_STORE_COMMIT
  restore_matrix_store_queue(&cursor, counts.matrix_store);
#endif // CONFIG_DIFFTEST_STORE_COMMIT
#ifdef CONFIG_AME_MSTORE_ACCESS_CHECK
  restore_mstore_queue(&cursor, counts.mstore);
#endif // CONFIG_AME_MSTORE_ACCESS_CHECK
  restore_amu_ctrl_queue(&cursor, counts.amu_ctrl);
  restore_msync_queue(&cursor, counts.msync);
}

size_t difftest_ame_replay_state_size(void) {
  const AmeReplayStateCounts counts = get_state_counts();
  return calculate_state_size(&counts);
}

#endif // CONFIG_RV_AME && CONFIG_SHARE_REF
