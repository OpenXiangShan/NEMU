/***************************************************************************************
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
***************************************************************************************/

#ifndef NEMU_STORE_LOG_HASH_H
#define NEMU_STORE_LOG_HASH_H

#include <utils.h>

// Canonical records are shared by committed-store queues and boundary hashes.
typedef struct {
  uint64_t addr;
  uint64_t data;
  uint8_t mask;
} store_record_t;

// Normalization only: records live on the caller stack, not in a retained log.
// A scalar write touches at most two aligned 8-byte records. Masked-off data
// bytes are zero, matching the committed-store queue and store-hash protocol.
static inline unsigned store_record_split(uint64_t addr, uint64_t data, int len,
                                          store_record_t records[2]) {
  assert(len > 0 && len <= 8);
  unsigned offset = addr & 7;
  unsigned low_len = MIN_OF((unsigned)len, 8 - offset);
  records[0] = (store_record_t) {
    .addr = addr & ~UINT64_C(7),
    .data = (data & (UINT64_MAX >> ((8 - low_len) * 8))) << (offset * 8),
    .mask = ((1u << low_len) - 1) << offset,
  };
  if (low_len == (unsigned)len) return 1;

  unsigned high_len = len - low_len;
  records[1] = (store_record_t) {
    .addr = records[0].addr + 8,
    .data = (data >> (low_len * 8)) & (UINT64_MAX >> ((8 - high_len) * 8)),
    .mask = (1u << high_len) - 1,
  };
  return 2;
}

// Runtime collection and constant-size boundary digest.
// STORE_LOG separately enables rollback.
#ifdef CONFIG_STORE_LOG_HASH
void store_log_hash_reset(void);
void store_log_hash_set_enabled(bool enabled);
bool store_log_hash_enabled(void);
void store_log_hash_update(uint64_t addr, uint64_t data, uint8_t mask);
void store_log_hash(uint64_t *lo, uint64_t *hi, uint64_t *count);
#ifdef CONFIG_STORE_LOG
void store_log_hash_checkpoint(void);
void store_log_hash_restore(void);
#endif
#endif

#endif // NEMU_STORE_LOG_HASH_H
