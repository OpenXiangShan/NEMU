#ifndef __PROFILING_MEM_TRACE_H__
#define __PROFILING_MEM_TRACE_H__

#include <stdint.h>
#include <stdbool.h>

#define NEMU_MEM_TRACE_BEGIN 0x103
#define NEMU_MEM_TRACE_END   0x104

void mem_trace_begin(void);
void mem_trace_end(void);
void mem_trace_abort(void);
bool mem_trace_enabled(void);
void mem_trace_scalar_load(uint64_t bytes);
void mem_trace_scalar_store(uint64_t bytes);
void mem_trace_vector_load(uint64_t bytes);
void mem_trace_vector_store(uint64_t bytes);
void mem_trace_matrix_load(uint64_t bytes);
void mem_trace_matrix_store(uint64_t bytes);

#endif
