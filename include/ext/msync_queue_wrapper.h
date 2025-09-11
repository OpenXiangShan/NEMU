#ifndef NEMU_MSYNC_QUEUE_WRAPPER_H
#define NEMU_MSYNC_QUEUE_WRAPPER_H

#include <common.h>
#include <ext/msync.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_RVMATRIX
void msync_queue_reset();
void msync_queue_push(msync_event_t msync_event);
void msync_queue_pop();
msync_event_t msync_queue_front();
msync_event_t msync_queue_back();
size_t msync_queue_size();
bool msync_queue_empty();

void msync_queue_emplace(uint8_t op, uint8_t token);
#endif // CONFIG_RVMATRIX

#ifdef __cplusplus
}
#endif

#endif // NEMU_MSYNC_QUEUE_WRAPPER_H