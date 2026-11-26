/***************************************************************************************
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
* Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of the license at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <cpu/cpu.h>
#include <cpu/decode.h>
#include <ame/mstore.h>
#include <memory/paddr.h>
#include <memory/vaddr.h>
#include "ame/raw-fp4.h"
#ifdef CONFIG_AME_MEM_ACCESS_CHECK
#include <ame/svstore_queue_wrapper.h>
#endif // CONFIG_AME_MEM_ACCESS_CHECK
#include "../instr/ame/mreg.h"
#include "../local-include/intr.h"

#if defined(CONFIG_RV_AME) && defined(CONFIG_RV_AME_FP4)

static paddr_t raw_fp4_paddrs[RAW_FP4_TILE_BYTES];

static bool raw_fp4_add_offset(vaddr_t base, uint64_t offset, vaddr_t *address) {
  uint64_t max_vaddr = (uint64_t)~(vaddr_t)0;
  if (offset > max_vaddr - (uint64_t)base) {
    return false;
  }
  *address = base + (vaddr_t)offset;
  return true;
}

static bool raw_fp4_byte_vaddr(vaddr_t base, vaddr_t stride, int memory_row,
                               size_t byte_index, vaddr_t *address) {
  uint64_t row = (uint64_t)memory_row;
  uint64_t row_offset;

  if (row != 0 && (uint64_t)stride > UINT64_MAX / row) {
    return false;
  }
  row_offset = row * (uint64_t)stride;
  if ((uint64_t)byte_index > UINT64_MAX - row_offset) {
    return false;
  }
  return raw_fp4_add_offset(base, row_offset + (uint64_t)byte_index, address);
}

// Both matrix issue and standalone byte preflight translate a byte-sized access.
static bool translate_byte(vaddr_t address, int type, paddr_t *paddr) {
  if (isa_mmu_check(address, 1, type) == MMU_DIRECT) {
    *paddr = address;
    return true;
  }
  paddr_t page_base = isa_mmu_translate(address, 1, type);
  if ((page_base & PAGE_MASK) != MEM_RET_OK) return false;
  *paddr = page_base | (address & PAGE_MASK);
  return true;
}

static bool raw_fp4_translate_byte(vaddr_t address, int type, paddr_t *paddr) {
  address = get_effective_address(address, type);
  if (!translate_byte(address, type, paddr)) return false;
  if (!check_paddr(*paddr, 1, type, type, cpu.mode, address)) {
    return false;
  }
  if (!in_pmem(*paddr)) {
    longjmp_exception(EX_II);
  }
  return true;
}

static bool raw_fp4_preflight(vaddr_t base, vaddr_t stride, int memory_rows,
                              size_t row_bytes, int type) {
  size_t byte_count = 0;

  for (int memory_row = 0; memory_row < memory_rows; memory_row++) {
    for (size_t byte_index = 0; byte_index < row_bytes; byte_index++) {
      vaddr_t address;
      if (!raw_fp4_byte_vaddr(base, stride, memory_row, byte_index, &address)) {
        longjmp_exception(EX_II);
      }
      if (!raw_fp4_translate_byte(address, type, &raw_fp4_paddrs[byte_count])) {
        return false;
      }
      byte_count++;
    }
  }
  return true;
}

static void raw_fp4_transfer_byte(uint8_t *memory, int row, int first_column, int columns,
                           bool transpose, bool store, int mreg_id) {
  uint8_t packed = *memory;
  for (int column = first_column; column < first_column + 2 && column < columns; ++column) {
    int r = raw_fp4_register_row(row, column, transpose);
    int k = raw_fp4_register_column(row, column, transpose);
    if (store) {
      packed = raw_fp4_set_nibble(packed, column, get_mreg_nibble(mreg_id, r, k));
    } else {
      set_mreg_nibble(mreg_id, r, k, raw_fp4_get_nibble(packed, column));
    }
  }
  if (store) *memory = packed;
}

void raw_fp4_transfer_matrix(uint8_t *base, uint64_t stride, int rows, int columns,
                             bool transpose, bool store, int reg) {
  int memory_rows = raw_fp4_memory_rows(rows, columns, transpose);
  int memory_columns = raw_fp4_memory_columns(rows, columns, transpose);
  for (int r = 0; r < memory_rows; ++r) {
    for (int c = 0; c < memory_columns; c += 2) {
      raw_fp4_transfer_byte(base + r * stride + c / 2, r, c, memory_columns, transpose, store, reg);
    }
  }
}

static bool issue_packed_matrix(struct Decode *s, vaddr_t base, vaddr_t stride, int row,
                                int column, bool transpose, bool store, char m_name, int mreg_id) {
  // Issue one event with its original element dimensions. SHARE_REF transfers
  // the data when the corresponding AMU completion arrives.
  paddr_t pbase;
  save_globals(s);
  int type = store ? MEM_TYPE_MATRIX_WRITE : MEM_TYPE_MATRIX_READ;
  if (!translate_byte(base, type, &pbase)) return false;
  if (store) {
    paddr_write_matrix(pbase, stride, row, column, MSEW_FP4, transpose, cpu.mode, base, m_name, mreg_id);
  } else {
    paddr_read_matrix(pbase, stride, row, column, MSEW_FP4, transpose, cpu.mode, base, m_name, mreg_id);
  }
#ifdef CONFIG_QUERY_REF
  cpu.query_mem_event.pc = cpu.debug.current_pc;
  cpu.query_mem_event.mem_access = true;
  cpu.query_mem_event.mem_access_is_load = !store;
  cpu.query_mem_event.mem_access_vaddr = base;
#endif
  return true;
}

bool raw_fp4_matrix_access(struct Decode *s, vaddr_t base, vaddr_t stride, int row,
                           int column, bool transpose, bool store, char m_name, int mreg_id) {
  if (ISDEF(CONFIG_SHARE) || ISDEF(CONFIG_DIFFTEST_AMU_CTRL) ||
      ISDEF(CONFIG_DIFFTEST_STORE_COMMIT) || ISDEF(CONFIG_STORE_LOG)) {
    return issue_packed_matrix(s, base, stride, row, column, transpose, store, m_name, mreg_id);
  }

  if (mreg_id < 0 || mreg_id >= 4 || row < 0 || column < 0) {
    longjmp_exception(EX_II);
  }
  if (row > ROWNUM || column > TRENUM8 * 2) {
    longjmp_exception(EX_II);
  }

  int memory_rows = raw_fp4_memory_rows(row, column, transpose);
  int memory_columns = raw_fp4_memory_columns(row, column, transpose);
  size_t row_bytes = raw_fp4_packed_bytes((size_t)memory_columns);
  size_t tile_bytes = (size_t)row * raw_fp4_packed_bytes((size_t)column);
  size_t transfer_bytes = (size_t)memory_rows * row_bytes;

  if (tile_bytes > RAW_FP4_TILE_BYTES || transfer_bytes > RAW_FP4_TILE_BYTES) {
    longjmp_exception(EX_II);
  }

  int type = store ? MEM_TYPE_WRITE : MEM_TYPE_READ;
  if (!raw_fp4_preflight(base, stride, memory_rows, row_bytes, type)) {
    return false;
  }

#ifdef CONFIG_AME_MEM_ACCESS_CHECK
  if (!store) {
    for (size_t i = 0; i < transfer_bytes; ++i) {
      vaddr_t vaddr = base + (i / row_bytes) * stride + i % row_bytes;
      if (svstore_queue_check_matrix_addr_conflict(
              raw_fp4_paddrs[i], 0, 1, 1, 0, false, s->pc, vaddr)) {
        break;
      }
    }
  }
#endif

  size_t byte_count = 0;
  for (int memory_row = 0; memory_row < memory_rows; memory_row++) {
    for (size_t byte_index = 0; byte_index < row_bytes; byte_index++, byte_count++) {
      int memory_column = (int)(byte_index << 1);
      uint8_t *host_byte = guest_to_host(raw_fp4_paddrs[byte_count]);

#ifdef CONFIG_MEMORY_REGION_ANALYSIS
      analysis_memory_commit(raw_fp4_paddrs[byte_count]);
#endif

      raw_fp4_transfer_byte(host_byte, memory_row, memory_column, memory_columns,
                            transpose, store, mreg_id);
    }
  }
  return true;
}

#endif // CONFIG_RV_AME && CONFIG_RV_AME_FP4
