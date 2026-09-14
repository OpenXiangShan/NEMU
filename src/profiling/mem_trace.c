#include <common.h>
#include <profiling/mem_trace.h>
#include <inttypes.h>
#include <stdio.h>

typedef struct {
  bool active;
  MemTraceVectorKind kind;
  uint64_t pc;
  uint64_t bytes;
  uint64_t first_addr;
} MemTraceVectorEvent;

typedef struct {
  bool enabled;
  uint64_t scalar_load_bytes;
  uint64_t scalar_store_bytes;
  uint64_t vector_load_bytes;
  uint64_t vector_store_bytes;
  uint64_t matrix_load_bytes;
  uint64_t matrix_store_bytes;
  MemTraceVectorEvent vector_event;
} MemTraceState;

static MemTraceState mem_trace_state;

static void mem_trace_reset_vector_event(void) {
  mem_trace_state.vector_event.active = false;
  mem_trace_state.vector_event.kind = MEM_TRACE_VECTOR_LOAD;
  mem_trace_state.vector_event.pc = 0;
  mem_trace_state.vector_event.bytes = 0;
  mem_trace_state.vector_event.first_addr = 0;
}

static void mem_trace_reset_counters(void) {
  mem_trace_state.scalar_load_bytes = 0;
  mem_trace_state.scalar_store_bytes = 0;
  mem_trace_state.vector_load_bytes = 0;
  mem_trace_state.vector_store_bytes = 0;
  mem_trace_state.matrix_load_bytes = 0;
  mem_trace_state.matrix_store_bytes = 0;
  mem_trace_reset_vector_event();
}

static void mem_trace_print_event(const char *kind, uint64_t bytes,
                                  uint64_t pc, uint64_t addr) {
  if (mem_trace_state.enabled) {
    printf("[T] %s %" PRIu64 "B pc=0x%" PRIx64 " addr=0x%" PRIx64 "\n",
           kind, bytes, pc, addr);
  }
}

static void mem_trace_print_total(const char *kind, uint64_t bytes) {
  printf("[T] %s %" PRIu64 "B", kind, bytes);
  if (bytes < 1024) {
    printf("\n");
  } else if (bytes < 1024 * 1024) {
    printf(" (%.2fKiB)\n", (double) bytes / 1024.0);
  } else {
    printf(" (%.2fMiB)\n", (double) bytes / (1024.0 * 1024.0));
  }
}

static void mem_trace_finish_vector_event(bool partial) {
  MemTraceVectorEvent *event = &mem_trace_state.vector_event;
  if (!event->active) {
    return;
  }

  const char *kind = event->kind == MEM_TRACE_VECTOR_LOAD ? "vl" : "vs";
  if (partial && mem_trace_state.enabled) {
    printf("[T] %s %" PRIu64 "B pc=0x%" PRIx64
           " addr=0x%" PRIx64 " partial=1\n",
           kind, event->bytes, event->pc, event->first_addr);
  } else {
    mem_trace_print_event(kind, event->bytes, event->pc, event->first_addr);
  }
  mem_trace_reset_vector_event();
}

void mem_trace_begin(void) {
  mem_trace_reset_counters();
  mem_trace_state.enabled = true;
  printf("[T] begin\n");
  printf("[T] address_space physical\n");
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

void mem_trace_vector_begin(MemTraceVectorKind kind, uint64_t pc) {
  if (!mem_trace_state.enabled) {
    return;
  }

  mem_trace_finish_vector_event(true);
  mem_trace_state.vector_event.active = true;
  mem_trace_state.vector_event.kind = kind;
  mem_trace_state.vector_event.pc = pc;
}

void mem_trace_vector_access(uint64_t bytes, uint64_t addr) {
  MemTraceVectorEvent *event = &mem_trace_state.vector_event;
  if (!mem_trace_state.enabled || !event->active) {
    return;
  }

  if (event->bytes == 0 && bytes > 0) {
    event->first_addr = addr;
  }
  event->bytes += bytes;
  if (event->kind == MEM_TRACE_VECTOR_LOAD) {
    mem_trace_state.vector_load_bytes += bytes;
  } else {
    mem_trace_state.vector_store_bytes += bytes;
  }
}

void mem_trace_vector_end(void) {
  mem_trace_finish_vector_event(false);
}

void mem_trace_vector_fault(void) {
  mem_trace_finish_vector_event(true);
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

void mem_trace_matrix_sync_reset(uint8_t sync) {
  if (mem_trace_state.enabled) {
    printf("[T] msyncregreset sync%u\n", sync);
  }
}

void mem_trace_matrix_release(uint8_t sync) {
  if (mem_trace_state.enabled) {
    printf("[T] mrelease sync%u\n", sync);
  }
}

void mem_trace_matrix_acquire(uint8_t sync, uint64_t threshold) {
  if (mem_trace_state.enabled) {
    printf("[T] macquire sync%u, %" PRIu64 "\n", sync, threshold);
  }
}

void mem_trace_matrix_fence(void) {
  if (mem_trace_state.enabled) {
    printf("[T] mfence\n");
  }
}
