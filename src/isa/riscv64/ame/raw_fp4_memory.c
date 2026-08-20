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
#include <memory/host.h>
#include <memory/paddr.h>
#include <memory/vaddr.h>
#include "ame/raw-fp4.h"
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

static bool raw_fp4_translate_byte(vaddr_t address, int type, paddr_t *paddr) {
  address = get_effective_address(address, type);
  int mmu_mode = isa_mmu_check(address, 1, type);

  if (mmu_mode == MMU_DIRECT) {
    *paddr = address;
  } else {
    paddr_t page_base = isa_mmu_translate(address, 1, type);
    if ((page_base & PAGE_MASK) != MEM_RET_OK) {
      return false;
    }
    *paddr = page_base | (address & PAGE_MASK);
  }

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

bool raw_fp4_matrix_access(struct Decode *s, vaddr_t base, vaddr_t stride,
                           int row, int column, bool transpose, bool store, int mreg_id) {
#if defined(CONFIG_SHARE) || defined(CONFIG_SHARE_REF) || \
    defined(CONFIG_SHARE_CTRL) || defined(CONFIG_DIFFTEST_AMU_CTRL) || \
    defined(CONFIG_USE_SPARSEMM) || defined(CONFIG_DIFFTEST_STORE_COMMIT) || \
    defined(CONFIG_STORE_LOG)
  longjmp_exception(EX_II);
#endif

  (void)s;
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

#ifdef CONFIG_RVH
  if (!store) {
    extern int rvh_hlvx_check(struct Decode *s, int type);
    rvh_hlvx_check(s, MEM_TYPE_READ);
  }
#endif

  int type = store ? MEM_TYPE_WRITE : MEM_TYPE_READ;
  if (!raw_fp4_preflight(base, stride, memory_rows, row_bytes, type)) {
    return false;
  }

  size_t byte_count = 0;
  for (int memory_row = 0; memory_row < memory_rows; memory_row++) {
    for (size_t byte_index = 0; byte_index < row_bytes; byte_index++, byte_count++) {
      int memory_column = (int)(byte_index << 1);
      uint8_t *host_byte = guest_to_host(raw_fp4_paddrs[byte_count]);

#ifdef CONFIG_MEMORY_REGION_ANALYSIS
      analysis_memory_commit(raw_fp4_paddrs[byte_count]);
#endif

      if (!store) {
        uint8_t packed_byte = *host_byte;
        for (int nibble = 0; nibble < 2 && memory_column + nibble < memory_columns; nibble++) {
          int register_row = raw_fp4_register_row(memory_row, memory_column + nibble, transpose);
          int register_column = raw_fp4_register_column(memory_row, memory_column + nibble, transpose);
          set_mreg_nibble(mreg_id, register_row, register_column,
                          raw_fp4_get_nibble(packed_byte, memory_column + nibble));
        }
      } else {
        int register_row = raw_fp4_register_row(memory_row, memory_column, transpose);
        int register_column = raw_fp4_register_column(memory_row, memory_column, transpose);
        uint8_t packed_byte = get_mreg_nibble(mreg_id, register_row, register_column);

        if (memory_column + 1 < memory_columns) {
          register_row = raw_fp4_register_row(memory_row, memory_column + 1, transpose);
          register_column = raw_fp4_register_column(memory_row, memory_column + 1, transpose);
          packed_byte |= get_mreg_nibble(mreg_id, register_row, register_column) << 4;
        } else {
          packed_byte = raw_fp4_set_nibble(*host_byte, memory_column, packed_byte);
        }
        *host_byte = packed_byte;
      }
    }
  }

  if (!store && (column & 1)) {
    for (int register_row = 0; register_row < row; register_row++) {
      set_mreg_nibble(mreg_id, register_row, column, 0);
    }
  }
  return true;
}

#endif // CONFIG_RV_AME && CONFIG_RV_AME_FP4
