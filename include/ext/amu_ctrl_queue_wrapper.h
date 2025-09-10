#ifndef NEMU_AMU_CTRL_QUEUE_WRAPPER_H
#define NEMU_AMU_CTRL_QUEUE_WRAPPER_H

#include <common.h>
#include <ext/amuctrl.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_RVMATRIX
void amu_ctrl_queue_reset();
void amu_ctrl_queue_push(amu_ctrl_event_t amu_ctrl_event);
void amu_ctrl_queue_pop();
amu_ctrl_event_t amu_ctrl_queue_front();
amu_ctrl_event_t amu_ctrl_queue_back();
size_t amu_ctrl_queue_size();
bool amu_ctrl_queue_empty();

void amu_ctrl_queue_mma_emplace(uint8_t md, bool sat, bool isfp, uint8_t ms1, uint8_t ms2,
                                uint16_t mtilem, uint16_t mtilen, uint16_t mtilek,
                                uint8_t types, uint8_t typed);
void amu_ctrl_queue_mls_emplace(uint8_t ms, bool ls, bool transpose, bool isacc,
                                uint64_t base, uint64_t stride,
                                uint16_t row, uint16_t column, uint8_t msew);
void amu_ctrl_queue_mrelease_emplace(uint8_t tokenRd);
#endif // CONFIG_RVMATRIX

#ifdef __cplusplus
}
#endif

#endif // NEMU_AMU_CTRL_QUEUE_WRAPPER_H