#ifndef NEMU_MSTORE_QUEUE_WRAPPER_H
#define NEMU_MSTORE_QUEUE_WRAPPER_H

#include <common.h>
#include <ext/mstore.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_RVMATRIX
void mstore_queue_reset();
void mstore_queue_push(mstore_info_t mstore_info);
void mstore_queue_pop();
mstore_info_t mstore_queue_front();
mstore_info_t mstore_queue_back();
size_t mstore_queue_size();
bool mstore_queue_empty();

void mstore_queue_emplace(uint64_t base_addr, uint64_t stride,
                          uint32_t row, uint32_t column, uint32_t msew, bool transpose);

void mstore_queue_update_mrelease(uint8_t tok_i, uint64_t mtokr_value);

void mstore_queue_update_acquire(uint8_t tok_i, uint64_t threshold);

bool mstore_queue_check_addr_conflict(uint64_t addr, int len);
#endif // CONFIG_RVMATRIX

#ifdef __cplusplus
}
#endif

#endif // NEMU_MSTORE_QUEUE_WRAPPER_H

