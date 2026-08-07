#ifndef __EXT_CUTE_GOLDEN_H__
#define __EXT_CUTE_GOLDEN_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CUTE_NEMU_GOLDEN_ABI_VERSION 1u

typedef int32_t (*cute_nemu_golden_memory_read_fn)(
    void *opaque, uint64_t address, uint8_t *destination, uint64_t size);
typedef int32_t (*cute_nemu_golden_memory_write_fn)(
    void *opaque, uint64_t address, const uint8_t *source, uint64_t size);

typedef struct {
  uint32_t struct_size;
  uint32_t abi_version;
  void *opaque;
  cute_nemu_golden_memory_read_fn read;
  cute_nemu_golden_memory_write_fn write;
} cute_nemu_golden_memory_callbacks_t;

typedef enum {
  CUTE_NEMU_GOLDEN_TARGET_A = 0,
  CUTE_NEMU_GOLDEN_TARGET_B = 1,
  CUTE_NEMU_GOLDEN_TARGET_ACC = 2,
} cute_nemu_golden_matrix_target_t;

typedef struct {
  uint32_t struct_size;
  uint32_t reserved0;
  uint64_t address;
  uint64_t stride;
  uint16_t rows;
  uint16_t columns;
  uint8_t store;
  uint8_t transpose;
  uint8_t target;
  uint8_t matrix;
  uint8_t element_width;
  uint8_t reserved1[3];
} cute_nemu_golden_lsu_operation_t;

typedef struct {
  uint32_t struct_size;
  uint16_t m;
  uint16_t n;
  uint16_t k;
  uint8_t source_a;
  uint8_t source_b;
  uint8_t destination;
  uint8_t source_a_type;
  uint8_t source_b_type;
  uint8_t destination_type;
  uint8_t is_fp;
  uint8_t saturate;
  uint8_t rounding_mode;
  uint8_t reserved[3];
} cute_nemu_golden_mma_operation_t;

typedef struct {
  uint32_t struct_size;
  uint8_t matrix;
  uint8_t reserved[3];
} cute_nemu_golden_mzero_operation_t;

typedef struct {
  uint32_t struct_size;
  uint8_t sync_read;
  uint8_t reserved[3];
} cute_nemu_golden_release_operation_t;

uint32_t cute_nemu_golden_abi_version(void);
int32_t cute_nemu_golden_initialize(
    const cute_nemu_golden_memory_callbacks_t *callbacks);
int32_t cute_nemu_golden_reset(void);
int32_t cute_nemu_golden_execute_lsu(
    const cute_nemu_golden_lsu_operation_t *operation);
int32_t cute_nemu_golden_execute_mma(
    const cute_nemu_golden_mma_operation_t *operation);
int32_t cute_nemu_golden_execute_mzero(
    const cute_nemu_golden_mzero_operation_t *operation);
int32_t cute_nemu_golden_execute_release(
    const cute_nemu_golden_release_operation_t *operation);
const char *cute_nemu_golden_last_error(void);
void cute_nemu_golden_destroy(void);

#ifdef __cplusplus
}
#endif

#endif
