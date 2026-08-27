#include <common.h>
#include <profiling/mem_trace.h>
#include <stdio.h>

typedef struct {
  bool enabled;
  uint64_t scalar_load_bytes;
  uint64_t scalar_store_bytes;
  uint64_t vector_load_bytes;
  uint64_t vector_store_bytes;
  uint64_t matrix_load_bytes;
  uint64_t matrix_store_bytes;
} MemTraceState;

static MemTraceState mem_trace_state;

static void mem_trace_reset_counters(void) {
  mem_trace_state.scalar_load_bytes = 0;
  mem_trace_state.scalar_store_bytes = 0;
  mem_trace_state.vector_load_bytes = 0;
  mem_trace_state.vector_store_bytes = 0;
  mem_trace_state.matrix_load_bytes = 0;
  mem_trace_state.matrix_store_bytes = 0;
}

static void mem_trace_print_event(const char *kind, uint64_t bytes,
                                  uint64_t pc, uint64_t addr) {
  if (mem_trace_state.enabled) {
    printf("[T] %s %luB pc=0x%lx addr=0x%lx\n", kind, bytes, pc, addr);
  }
}

static void mem_trace_print_total(const char *kind, uint64_t bytes) {
  if (bytes < 1024) {
    printf("[T] %s %luB\n", kind, bytes);
  } else if (bytes < 1024 * 1024) {
    printf("[T] %s %.2fKB\n", kind, (double) bytes / 1024.0);
  } else {
    printf("[T] %s %.2fMB\n", kind, (double) bytes / (1024.0 * 1024.0));
  }
}

void mem_trace_begin(void) {
  mem_trace_reset_counters();
  mem_trace_state.enabled = true;
  printf("[T] begin\n");
}

void mem_trace_end(void) {
  if (!mem_trace_state.enabled) {
    return;
  }

  mem_trace_print_total("vl_total", mem_trace_state.vector_load_bytes);
  mem_trace_print_total("vs_total", mem_trace_state.vector_store_bytes);
  mem_trace_print_total("ml_total", mem_trace_state.matrix_load_bytes);
  mem_trace_print_total("ms_total", mem_trace_state.matrix_store_bytes);
  mem_trace_print_total("scalar_load", mem_trace_state.scalar_load_bytes);
  mem_trace_print_total("scalar_store", mem_trace_state.scalar_store_bytes);
  printf("[T] end\n");
  mem_trace_state.enabled = false;
  mem_trace_reset_counters();
}

void mem_trace_abort(void) {
  mem_trace_state.enabled = false;
  mem_trace_reset_counters();
}

bool mem_trace_enabled(void) {
  return mem_trace_state.enabled;
}

void mem_trace_scalar_load(uint64_t bytes) {
  if (mem_trace_state.enabled) {
    mem_trace_state.scalar_load_bytes += bytes;
  }
}

void mem_trace_scalar_store(uint64_t bytes) {
  if (mem_trace_state.enabled) {
    mem_trace_state.scalar_store_bytes += bytes;
  }
}

void mem_trace_vector_load(uint64_t bytes, uint64_t pc, uint64_t addr) {
  if (mem_trace_state.enabled) {
    mem_trace_state.vector_load_bytes += bytes;
  }
  mem_trace_print_event("vl", bytes, pc, addr);
}

void mem_trace_vector_store(uint64_t bytes, uint64_t pc, uint64_t addr) {
  if (mem_trace_state.enabled) {
    mem_trace_state.vector_store_bytes += bytes;
  }
  mem_trace_print_event("vs", bytes, pc, addr);
}

void mem_trace_matrix_load(uint64_t bytes, uint64_t pc, uint64_t addr) {
  if (mem_trace_state.enabled) {
    mem_trace_state.matrix_load_bytes += bytes;
  }
  mem_trace_print_event("ml", bytes, pc, addr);
}

void mem_trace_matrix_store(uint64_t bytes, uint64_t pc, uint64_t addr) {
  if (mem_trace_state.enabled) {
    mem_trace_state.matrix_store_bytes += bytes;
  }
  mem_trace_print_event("ms", bytes, pc, addr);
}
