#ifndef __PROFILING_MEM_TRACE_H__
#define __PROFILING_MEM_TRACE_H__

#include <stdint.h>
#include <stdbool.h>

#define NEMU_MEM_TRACE_BEGIN 0x103
#define NEMU_MEM_TRACE_END   0x104

typedef enum {
  MEM_TRACE_VECTOR_LOAD,
  MEM_TRACE_VECTOR_STORE,
} MemTraceVectorKind;

void mem_trace_begin(void);
void mem_trace_end(void);
void mem_trace_abort(void);
bool mem_trace_enabled(void);
void mem_trace_scalar_load(uint64_t bytes);
void mem_trace_scalar_store(uint64_t bytes);
void mem_trace_vector_begin(MemTraceVectorKind kind, uint64_t pc);
void mem_trace_vector_access(uint64_t bytes, uint64_t addr);
void mem_trace_vector_end(void);
void mem_trace_vector_fault(void);
void mem_trace_matrix_load(uint64_t bytes, uint64_t pc, uint64_t addr);
void mem_trace_matrix_store(uint64_t bytes, uint64_t pc, uint64_t addr);
void mem_trace_matrix_sync_reset(uint8_t sync);
void mem_trace_matrix_release(uint8_t sync);
void mem_trace_matrix_acquire(uint8_t sync, uint64_t threshold);
void mem_trace_matrix_fence(void);

#endif
