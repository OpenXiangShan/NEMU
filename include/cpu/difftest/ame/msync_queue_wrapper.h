#ifndef __AME_MSYNC_QUEUE_WRAPPER_H__
#define __AME_MSYNC_QUEUE_WRAPPER_H__

#include <common.h>
#include <cpu/difftest/ame/msync.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_RV_AME) && defined(CONFIG_SHARE_REF)
void msync_queue_reset();
void msync_queue_push(msync_event_t msync_event);
void msync_queue_pop();
msync_event_t msync_queue_front();
msync_event_t msync_queue_back();
size_t msync_queue_size();
bool msync_queue_empty();

void msync_queue_emplace(uint8_t op, uint8_t msyncRd);
#endif // defined(CONFIG_RV_AME) && defined(CONFIG_SHARE_REF)

#ifdef __cplusplus
}
#endif

#endif // __AME_MSYNC_QUEUE_WRAPPER_H__
