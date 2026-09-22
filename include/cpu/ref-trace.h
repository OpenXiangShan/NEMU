#ifndef __CPU_REF_TRACE_H__
#define __CPU_REF_TRACE_H__

#include <common.h>

#define REF_TRACE_MAX_MEM_EVENTS 16
#define REF_TRACE_MAX_CSR_EVENTS 16

typedef struct {
  bool is_write;
  uint8_t len;
  vaddr_t addr;
  word_t data;
} RefTraceMemEvent;

typedef struct {
  uint16_t addr;
  word_t data;
} RefTraceCsrEvent;

extern bool ref_trace_enabled;
extern RefTraceMemEvent ref_trace_mem_events[REF_TRACE_MAX_MEM_EVENTS];
extern unsigned ref_trace_mem_event_count;
extern RefTraceCsrEvent ref_trace_csr_events[REF_TRACE_MAX_CSR_EVENTS];
extern unsigned ref_trace_csr_event_count;

static inline void ref_trace_record_mem(bool is_write, vaddr_t addr, int len,
                                        word_t data) {
  if (unlikely(ref_trace_enabled)) {
    unsigned index = ref_trace_mem_event_count++;
    if (index < REF_TRACE_MAX_MEM_EVENTS) {
      ref_trace_mem_events[index] = (RefTraceMemEvent) {
        .is_write = is_write,
        .len = len,
        .addr = addr,
        .data = data,
      };
    }
  }
}

static inline void ref_trace_record_csr(uint16_t addr, word_t data) {
  if (unlikely(ref_trace_enabled)) {
    unsigned index = ref_trace_csr_event_count++;
    if (index < REF_TRACE_MAX_CSR_EVENTS) {
      ref_trace_csr_events[index] = (RefTraceCsrEvent) {
        .addr = addr,
        .data = data,
      };
    }
  }
}

#endif
