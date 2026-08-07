/***************************************************************************************
* Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
***************************************************************************************/

#include <common.h>

#if defined(CONFIG_RVMATRIX) && defined(CONFIG_SHARE_GOLDEN)

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <ext/cute_golden.h>
#include <isa.h>

#include "../isa/riscv64/instr/rvmatrix/mcompute_impl.h"

#define CUTE_GOLDEN_MAX_ADDRESS ((UINT64_C(1) << 48) - 1)
#define CUTE_GOLDEN_ERROR_SIZE 256

static cute_nemu_golden_memory_callbacks_t memory_callbacks;
static bool golden_initialized;
static char golden_error[CUTE_GOLDEN_ERROR_SIZE];

CPU_state cpu = {0};

static int32_t fail(const char *format, ...) {
  va_list arguments;
  va_start(arguments, format);
  vsnprintf(golden_error, sizeof(golden_error), format, arguments);
  va_end(arguments);
  return -1;
}

static void clear_error(void) {
  golden_error[0] = '\0';
}

static bool bytes_are_zero(const uint8_t *bytes, size_t size) {
  for (size_t index = 0; index < size; ++index) {
    if (bytes[index] != 0) return false;
  }
  return true;
}

static void reset_matrix_state(void) {
  memset(cpu.mtr, 0, sizeof(cpu.mtr));
  memset(cpu.macc, 0, sizeof(cpu.macc));
  memset(cpu.mcfg, 0, sizeof(cpu.mcfg));
  memset(cpu.mtokr, 0, sizeof(cpu.mtokr));
  cpu.mcsr = 0;
  cpu.mxrm = 0;
  cpu.msat = 0;
  cpu.mfflags = 0;
  cpu.mfrm = 0;
  cpu.msaten = 0;
  cpu.mtilem = 0;
  cpu.mtilen = 0;
  cpu.mtilek = 0;
}

static uint64_t read_little_endian(const uint8_t *source, size_t size) {
  uint64_t value = 0;
  for (size_t index = 0; index < size; ++index) {
    value |= (uint64_t)source[index] << (index * 8);
  }
  return value;
}

static void write_little_endian(uint8_t *destination, size_t size,
                                uint64_t value) {
  for (size_t index = 0; index < size; ++index) {
    destination[index] = (uint8_t)(value >> (index * 8));
  }
}

static bool valid_row_address(uint64_t base, uint64_t stride, uint16_t row,
                              uint64_t row_size, uint64_t *address) {
  if (row != 0 && stride > (UINT64_MAX - base) / row) return false;
  *address = base + stride * row;
  if (*address > CUTE_GOLDEN_MAX_ADDRESS) return false;
  return row_size == 0 || row_size - 1 <= CUTE_GOLDEN_MAX_ADDRESS - *address;
}

static int32_t validate_lsu(
    const cute_nemu_golden_lsu_operation_t *operation, uint8_t *register_id,
    uint16_t *matrix_rows, uint16_t *matrix_columns, size_t *element_bytes) {
  if (operation == NULL || operation->struct_size != sizeof(*operation)) {
    return fail("LSU operation has an incompatible struct size");
  }
  if (operation->reserved0 != 0 ||
      !bytes_are_zero(operation->reserved1, sizeof(operation->reserved1))) {
    return fail("LSU operation has non-zero reserved fields");
  }
  if (operation->target > CUTE_NEMU_GOLDEN_TARGET_ACC) {
    return fail("LSU operation has invalid target %u", operation->target);
  }
  if (operation->matrix > 15) {
    return fail("LSU operation has invalid matrix selector %u",
                operation->matrix);
  }
  if (operation->element_width > 3) {
    return fail("LSU element width %u is not supported by the golden model",
                operation->element_width);
  }
  *element_bytes = (size_t)1 << operation->element_width;
  const bool is_b = operation->target == CUTE_NEMU_GOLDEN_TARGET_B;
  *matrix_rows = is_b ? operation->columns : operation->rows;
  *matrix_columns = is_b ? operation->rows : operation->columns;
  *register_id = operation->target == CUTE_NEMU_GOLDEN_TARGET_ACC
                     ? (uint8_t)(4 + (operation->matrix & 3))
                     : (uint8_t)(operation->matrix & 3);

  const uint16_t maximum_rows = ROWNUM;
  const uint16_t row_bytes = operation->target == CUTE_NEMU_GOLDEN_TARGET_ACC
                                 ? ARLEN / 8
                                 : TRLEN / 8;
  const uint16_t maximum_columns = row_bytes / *element_bytes;
  if (*matrix_rows > maximum_rows || *matrix_columns > maximum_columns) {
    return fail("LSU matrix shape %ux%u exceeds register capacity %ux%u",
                *matrix_rows, *matrix_columns, maximum_rows, maximum_columns);
  }
  return 0;
}

uint32_t cute_nemu_golden_abi_version(void) {
  return CUTE_NEMU_GOLDEN_ABI_VERSION;
}

int32_t cute_nemu_golden_initialize(
    const cute_nemu_golden_memory_callbacks_t *callbacks) {
  if (golden_initialized) {
    return fail("NEMU golden model is already initialized");
  }
  if (callbacks == NULL || callbacks->struct_size != sizeof(*callbacks) ||
      callbacks->abi_version != CUTE_NEMU_GOLDEN_ABI_VERSION ||
      callbacks->read == NULL || callbacks->write == NULL) {
    return fail("NEMU golden model received invalid memory callbacks");
  }
  memory_callbacks = *callbacks;
  golden_initialized = true;
  reset_matrix_state();
  clear_error();
  return 0;
}

int32_t cute_nemu_golden_reset(void) {
  if (!golden_initialized) {
    return fail("NEMU golden model is not initialized");
  }
  reset_matrix_state();
  clear_error();
  return 0;
}

int32_t cute_nemu_golden_execute_lsu(
    const cute_nemu_golden_lsu_operation_t *operation) {
  if (!golden_initialized) {
    return fail("NEMU golden model is not initialized");
  }

  uint8_t register_id = 0;
  uint16_t matrix_rows = 0;
  uint16_t matrix_columns = 0;
  size_t element_bytes = 0;
  if (validate_lsu(operation, &register_id, &matrix_rows, &matrix_columns,
                   &element_bytes) != 0) {
    return -1;
  }

  uint8_t row_data[ALEN / 8];
  const uint16_t memory_rows = operation->transpose
                                   ? matrix_columns
                                   : matrix_rows;
  const uint16_t memory_columns = operation->transpose
                                      ? matrix_rows
                                      : matrix_columns;
  const uint64_t row_size = (uint64_t)memory_columns * element_bytes;
  for (uint16_t row = 0; row < memory_rows; ++row) {
    uint64_t row_address = 0;
    if (!valid_row_address(operation->address, operation->stride, row,
                           row_size, &row_address)) {
      return fail("LSU row %u exceeds the 48-bit address space", row);
    }

    if (!operation->store) {
      if (row_size != 0 &&
          memory_callbacks.read(memory_callbacks.opaque, row_address,
                                row_data, row_size) != 0) {
        return fail("LSU memory read callback failed at 0x%" PRIx64,
                    row_address);
      }
      for (uint16_t column = 0; column < memory_columns; ++column) {
        const uint64_t value = read_little_endian(
            row_data + (size_t)column * element_bytes, element_bytes);
        const uint16_t register_row = operation->transpose ? column : row;
        const uint16_t register_column = operation->transpose ? row : column;
        set_mreg(register_id, register_row, register_column, value,
                 operation->element_width);
      }
    } else {
      for (uint16_t column = 0; column < memory_columns; ++column) {
        rtlreg_t value = 0;
        const uint16_t register_row = operation->transpose ? column : row;
        const uint16_t register_column = operation->transpose ? row : column;
        get_mreg(register_id, register_row, register_column, &value,
                 operation->element_width, false);
        write_little_endian(row_data + (size_t)column * element_bytes,
                            element_bytes, value);
      }
      if (row_size != 0 &&
          memory_callbacks.write(memory_callbacks.opaque, row_address,
                                 row_data, row_size) != 0) {
        return fail("LSU memory write callback failed at 0x%" PRIx64,
                    row_address);
      }
    }
  }

  clear_error();
  return 0;
}

int32_t cute_nemu_golden_execute_mma(
    const cute_nemu_golden_mma_operation_t *operation) {
  if (!golden_initialized) {
    return fail("NEMU golden model is not initialized");
  }
  if (operation == NULL || operation->struct_size != sizeof(*operation)) {
    return fail("MMA operation has an incompatible struct size");
  }
  if (!bytes_are_zero(operation->reserved, sizeof(operation->reserved))) {
    return fail("MMA operation has non-zero reserved fields");
  }
  if (operation->source_a > 15 || operation->source_b > 15 ||
      operation->destination > 15) {
    return fail("MMA operation has an invalid matrix selector");
  }
  if (operation->is_fp) {
    return fail("floating-point MMA is not supported by the golden model yet");
  }
  if ((operation->source_a_type != 0 && operation->source_a_type != 4) ||
      (operation->source_b_type != 0 && operation->source_b_type != 4) ||
      operation->destination_type != 2) {
    return fail("integer MMA currently requires u8/i8 sources and i32 destination");
  }
  if (operation->m > ROWNUM || operation->n > ROWNUM ||
      operation->k > TRLEN / 8) {
    return fail("MMA shape %ux%ux%u exceeds NEMU matrix capacity",
                operation->m, operation->n, operation->k);
  }

  const uint8_t source_a = operation->source_a & 3;
  const uint8_t source_b = operation->source_b & 3;
  const uint8_t destination = 4 + (operation->destination & 3);
  const bool source_a_signed = operation->source_a_type == 4;
  const bool source_b_signed = operation->source_b_type == 4;

  cpu.mcfg[source_a].type_code = source_a_signed ? MTYPECODE_INT8
                                                  : MTYPECODE_UINT8;
  cpu.mcfg[source_b].type_code = source_b_signed ? MTYPECODE_INT8
                                                  : MTYPECODE_UINT8;
  cpu.mcfg[destination].type_code = MTYPECODE_INT32;
  cpu.mtilem = operation->m;
  cpu.mtilen = operation->n;
  cpu.mtilek = operation->k;
  cpu.msaten = operation->saturate != 0;
  cpu.mxrm = operation->rounding_mode;

  matrix_compute_integer(destination, source_a, source_b, operation->m,
                         operation->n, operation->k, 2, 0, 0,
                         source_a_signed, source_b_signed,
                         operation->saturate != 0);
  clear_error();
  return 0;
}

int32_t cute_nemu_golden_execute_mzero(
    const cute_nemu_golden_mzero_operation_t *operation) {
  if (!golden_initialized) {
    return fail("NEMU golden model is not initialized");
  }
  if (operation == NULL || operation->struct_size != sizeof(*operation)) {
    return fail("mzero operation has an incompatible struct size");
  }
  if (!bytes_are_zero(operation->reserved, sizeof(operation->reserved)) ||
      operation->matrix > 15) {
    return fail("mzero operation has invalid fields");
  }

  const uint8_t index = operation->matrix & 3;
  if ((operation->matrix & 4) != 0) {
    memset(cpu.macc[index], 0, sizeof(cpu.macc[index]));
  } else {
    memset(cpu.mtr[index], 0, sizeof(cpu.mtr[index]));
  }
  clear_error();
  return 0;
}

int32_t cute_nemu_golden_execute_release(
    const cute_nemu_golden_release_operation_t *operation) {
  if (!golden_initialized) {
    return fail("NEMU golden model is not initialized");
  }
  if (operation == NULL || operation->struct_size != sizeof(*operation)) {
    return fail("release operation has an incompatible struct size");
  }
  if (!bytes_are_zero(operation->reserved, sizeof(operation->reserved)) ||
      operation->sync_read >= MSYNC) {
    return fail("release operation has invalid sync selector");
  }
  ++cpu.mtokr[operation->sync_read];
  clear_error();
  return 0;
}

const char *cute_nemu_golden_last_error(void) {
  return golden_error;
}

void cute_nemu_golden_destroy(void) {
  reset_matrix_state();
  memset(&memory_callbacks, 0, sizeof(memory_callbacks));
  golden_initialized = false;
  clear_error();
}

#endif // CONFIG_RVMATRIX && CONFIG_SHARE_GOLDEN
