/***************************************************************************************
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
* Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <common.h>

#if defined(CONFIG_RV_AME)

#include <limits.h>
#include <stdint.h>

#include "cpu/cpu.h"
#include "cpu/exec.h"
#include "mldst_impl.h"
#include "mreg.h"
#include "../local-include/csr.h"
#include "../local-include/intr.h"
#include "../local-include/rtl.h"
#include "../local-include/reg.h"
#ifdef CONFIG_AME_MEM_ACCESS_CHECK
#include "ame/mstore_queue_wrapper.h"
#endif // CONFIG_AME_MEM_ACCESS_CHECK
#include "mcommon.h"
#ifdef CONFIG_RV_AME_FP4
#include "ame/raw-fp4.h"
#endif
#if defined(CONFIG_RV_AME_FP4) && defined(CONFIG_FPU_SOFT) && \
    defined(CONFIG_SHARE_REF) && defined(CONFIG_DIFFTEST_AMU_CTRL) && \
    !defined(CONFIG_SHARE_CTRL)
#include <isa.h>
#include <memory/paddr.h>
#include <memory/vaddr.h>
#include <cpu/difftest/ame/amu_ctrl_queue_wrapper.h>
#include "mcompute_impl.h"
#include "mxfp.h"
#include <string.h>
#endif

#ifdef PRINT_AMUCTRLIO
#include <stdio.h>
#endif

uint8_t get_size(mcfg_t cfg) {
  uint8_t dsize = 0;
  switch (cfg.type_code) {
    case MTYPECODE_INT8:
    case MTYPECODE_UINT8:
    case MTYPECODE_FP8E5M2:
    case MTYPECODE_FP8E4M3:
    case MTYPECODE_INT4:
    case MTYPECODE_UINT4:
    case MTYPECODE_NVFP4:
    case MTYPECODE_MXFP4:
    case MTYPECODE_FP2PACK4:
    case MTYPECODE_FP2PACK5:
      dsize = 0;
      break;
    case MTYPECODE_FP16:
    case MTYPECODE_BF16:
      dsize = 1;
      break;
    case MTYPECODE_INT32:
    case MTYPECODE_TF32:
    case MTYPECODE_FP32:
      dsize = 2;
      break;
    default:
      longjmp_exception(EX_II);
      break;
  }
  return dsize;
}

void check_size(Decode *s, uint64_t rmax_mreg, uint64_t cmax_mreg,
                char m_name, uint8_t dsize, bool raw_fp4) {
  // Validate CSR values before narrowing them for the int-based memory API.
  bool valid = rmax_mreg <= ROWNUM && rmax_mreg <= INT_MAX &&
               cmax_mreg <= INT_MAX;
  switch (m_name) {
    case 'a': case 'b':
      if (cmax_mreg > (raw_fp4 ? TRENUM8 * 2 : TRENUM8 / (1 << dsize))) {
        valid = false;
      }
      break;
    case 'c':
      if (raw_fp4 || cmax_mreg > ARENUM8 / (1 << dsize)) {
        valid = false;
      }
      break;
    case 'm':
      break;
    default:
      valid = false;
      break;
  }
  if (!valid) {
    longjmp_exception(EX_II);
  }
}

static bool validate_raw_fp4_access(mcfg_t cfg, uint64_t mreg_id,
                                    char m_name) {
#ifdef CONFIG_RV_AME_FP4
  bool raw_fp4 = is_raw_fp4(cfg);
  bool shared_mxfp4 = false;
#if defined(CONFIG_FPU_SOFT) && defined(CONFIG_SHARE_REF) && \
    defined(CONFIG_DIFFTEST_AMU_CTRL) && !defined(CONFIG_SHARE_CTRL)
  shared_mxfp4 = cfg.table_set == 1 && cfg.type_code == MTYPECODE_MXFP4;
#endif
  if (is_raw_fp4_type_code(cfg) && !raw_fp4 && !shared_mxfp4) {
    longjmp_exception(EX_II);
  }
  if ((raw_fp4 || shared_mxfp4) &&
      (mreg_id >= 4 || (m_name != 'a' && m_name != 'b'))) {
    longjmp_exception(EX_II);
  }
  return raw_fp4 || shared_mxfp4;
#else
  (void)cfg;
  (void)mreg_id;
  (void)m_name;
  return false;
#endif
}

void exec_mld(Decode *s, uint64_t base_addr, uint64_t row_byte_stride,
              uint64_t td, int row, int column, uint8_t dsize,
              bool is_trans, char m_name, bool raw_fp4) {

#ifdef PRINT_AMUCTRLIO
  fprintf(stderr,
    "[AmuCtrlIO] op=1 \n"
    "            ms=%ld, ls=0, transpose=%d, baseVAddr=%#lx, stride=%#lx\n"
    "            row=%d, col=%d, width=%#x, m_name=%c\n",
    td, is_trans, base_addr, row_byte_stride, row, column, dsize, m_name);
#endif

#ifdef CONFIG_RV_AME_FP4
  if (raw_fp4) {
    if (raw_fp4_matrix_access(s, base_addr, row_byte_stride, row, column,
                              is_trans, false, td, m_name)) {
      mp_set_dirty();
#ifdef CONFIG_AME_TILEREG_UB_CHECK
      // An odd FP4 row also zeroes its last byte's high nibble.
      ame_matrix_region_mark_write(td, row, ((uint64_t)column + 1) / 2,
                                  s->pc);
#endif // CONFIG_AME_TILEREG_UB_CHECK
    }
    return;
  }
#else
  (void)raw_fp4;
#endif

  mp_set_dirty();

  rtl_lmm(s, &base_addr, &row_byte_stride,
    row, column, dsize, is_trans,
    MMU_TRANSLATE, m_name, td);
#ifdef CONFIG_AME_TILEREG_UB_CHECK
  // Track the register rectangle in instruction order, including deferred REF loads.
  ame_matrix_region_mark_write(td, row, (uint64_t)column << dsize,
                              s->pc);
#endif // CONFIG_AME_TILEREG_UB_CHECK
}

void exec_mst(Decode *s, uint64_t base_addr, uint64_t row_byte_stride,
              uint64_t ts3, int row, int column, uint8_t dsize,
              bool is_trans, char m_name, bool raw_fp4) {
#ifdef PRINT_AMUCTRLIO
  fprintf(stderr,
    "[AmuCtrlIO] op=1 \n"
    "            ms=%ld, ls=1, transpose=%d, baseVAddr=%#lx, stride=%#lx\n"
    "            row=%d, col=%d, width=%#x, m_name=%c\n",
    ts3, is_trans, base_addr, row_byte_stride, row, column, dsize, m_name);
#endif

#ifdef CONFIG_RV_AME_FP4
  if (raw_fp4) {
#ifdef CONFIG_AME_TILEREG_UB_CHECK
    ame_matrix_region_check_read(ts3, row, ((uint64_t)column + 1) / 2, s->pc);
#endif // CONFIG_AME_TILEREG_UB_CHECK
    if (raw_fp4_matrix_access(s, base_addr, row_byte_stride, row, column,
                              is_trans, true, ts3, m_name)) {
#ifdef CONFIG_AME_MEM_ACCESS_CHECK
      mstore_queue_emplace(base_addr, row_byte_stride, row, column,
                           MSEW_FP4, is_trans);
#endif // CONFIG_AME_MEM_ACCESS_CHECK
    }
    return;
  }
#else
  (void)raw_fp4;
#endif

#ifdef CONFIG_AME_TILEREG_UB_CHECK
  ame_matrix_region_check_read(ts3, row, (uint64_t)column << dsize, s->pc);
#endif // CONFIG_AME_TILEREG_UB_CHECK
  rtl_smm(s, &base_addr, &row_byte_stride,
    row, column, dsize, is_trans,
    MMU_TRANSLATE, m_name, ts3);

#ifdef CONFIG_AME_MEM_ACCESS_CHECK
  mstore_queue_emplace(base_addr, row_byte_stride, row, column, dsize, is_trans);
#endif // CONFIG_AME_MEM_ACCESS_CHECK
}

void mld(Decode *s, bool is_trans, char m_name) {
  uint64_t td = s->dest.reg;
  uint8_t dsize = get_size(cpu.mcfg[td]);
  bool raw_fp4 = validate_raw_fp4_access(cpu.mcfg[td], td, m_name);
  uint64_t rmax_mreg = 0, cmax_mreg = 0;

  switch (m_name) {
    case 'a':
      rmax_mreg = mtilem->val;
      cmax_mreg = mtilek->val;
      break;
    case 'b':
      rmax_mreg = mtilen->val;
      cmax_mreg = mtilek->val;
      break;
    case 'c':
      rmax_mreg = mtilem->val;
      cmax_mreg = mtilen->val;
      break;
    default:
      Assert(false, "mld %c: invalid matrix selection!\n", m_name);
      break;
  }

  check_size(s, rmax_mreg, cmax_mreg, m_name, dsize, raw_fp4);
  exec_mld(s, reg_l(s->src1.reg), reg_l(s->src2.reg), td,
    (int)rmax_mreg, (int)cmax_mreg, dsize, is_trans, m_name, raw_fp4);
}

void mst(Decode *s, bool is_trans, char m_name) {
  uint64_t ts3 = s->dest.reg;
  uint8_t dsize = get_size(cpu.mcfg[ts3]);
  bool raw_fp4 = validate_raw_fp4_access(cpu.mcfg[ts3], ts3, m_name);
  uint64_t rmax_mreg = 0, cmax_mreg = 0;

  switch (m_name) {
    case 'a':
      rmax_mreg = mtilem->val;
      cmax_mreg = mtilek->val;
      break;
    case 'b':
      rmax_mreg = mtilen->val;
      cmax_mreg = mtilek->val;
      break;
    case 'c':
      rmax_mreg = mtilem->val;
      cmax_mreg = mtilen->val;
      break;
    default:
      Assert(false, "mst %c: invalid matrix selection!\n", m_name);
      break;
  }

  check_size(s, rmax_mreg, cmax_mreg, m_name, dsize, raw_fp4);
  exec_mst(s, reg_l(s->src1.reg), reg_l(s->src2.reg), ts3,
    (int)rmax_mreg, (int)cmax_mreg, dsize, is_trans, m_name, raw_fp4);
}

void mld_whole(Decode *s, char m_name) {
  uint64_t td = s->dest.reg;
  bool raw_fp4 = validate_raw_fp4_access(cpu.mcfg[td], td, m_name);
  uint8_t dsize = m_name == 'c' ? 2 : 0;
  uint64_t row_byte_stride = m_name == 'c' ? ARENUM8 : TRENUM8;

  exec_mld(s, reg_l(s->src1.reg), row_byte_stride, td,
    ROWNUM, m_name == 'c' ? ARENUM32 : (raw_fp4 ? TRENUM8 * 2 : TRENUM8),
    dsize, false, m_name, raw_fp4);
}

void mst_whole(Decode *s, char m_name) {
  uint64_t ts3 = s->dest.reg;
  bool raw_fp4 = validate_raw_fp4_access(cpu.mcfg[ts3], ts3, m_name);
  uint8_t dsize = m_name == 'c' ? 2 : 0;
  uint64_t row_byte_stride = m_name == 'c' ? ARENUM8 : TRENUM8;

  exec_mst(s, reg_l(s->src1.reg), row_byte_stride, ts3,
    ROWNUM, m_name == 'c' ? ARENUM32 : (raw_fp4 ? TRENUM8 * 2 : TRENUM8),
    dsize, false, m_name, raw_fp4);
}

#if defined(CONFIG_RV_AME_FP4) && defined(CONFIG_FPU_SOFT) && \
    defined(CONFIG_SHARE_REF) && defined(CONFIG_DIFFTEST_AMU_CTRL) && \
    !defined(CONFIG_SHARE_CTRL)
static bool mld_mx_source_paddr(vaddr_t address, int access_bytes,
                                paddr_t *paddr) {
  if (access_bytes <= 0) return false;
  address = get_effective_address(address, MEM_TYPE_READ);
  const int mmu_mode = isa_mmu_check(address, access_bytes, MEM_TYPE_READ);
  if (mmu_mode == MMU_DIRECT) {
    *paddr = address;
  } else {
    const paddr_t page_base = isa_mmu_translate(address, access_bytes, MEM_TYPE_READ);
    if ((page_base & PAGE_MASK) != MEM_RET_OK) return false;
    *paddr = page_base | (address & PAGE_MASK);
  }
  const uint64_t last_byte = (uint64_t)access_bytes - 1;
  if (*paddr > UINT64_MAX - last_byte || !in_pmem(*paddr) ||
      !in_pmem(*paddr + last_byte)) {
    return false;
  }
  return check_paddr(*paddr, access_bytes, MEM_TYPE_READ, MEM_TYPE_READ,
                     cpu.mode, address);
}
#endif

void mld_convert_f32_mx(Decode *s, bool is_b_side) {
#if defined(CONFIG_RV_AME_FP4) && defined(CONFIG_FPU_SOFT) && \
    defined(CONFIG_SHARE_REF) && defined(CONFIG_DIFFTEST_AMU_CTRL) && \
    !defined(CONFIG_SHARE_CTRL)
  require_matrix();
  const uint64_t td = s->dest.reg;
  if (td >= 4 || !amu_snapshot_ready()) longjmp_exception(EX_II);

  const mcfg_t cfg = cpu.mcfg[td];
  const word_t rows = is_b_side ? mtilen->val : mtilem->val;
  const uint64_t base = reg_l(s->src1.reg);
  const uint64_t stride = reg_l(s->src2.reg);
  if (cfg.table_set != 1 || cfg.type_code != MTYPECODE_MXFP4 ||
      mtilek->val != 64 || rows != 8 || stride < 256 || (stride & 3) ||
      (base & 3) || stride > (UINT64_MAX - 255) / 7 ||
      base > UINT64_MAX - (7 * stride + 255) ||
      cpu.mscale_generation[(is_b_side ? 4 : 0) + td] == UINT32_MAX) {
    longjmp_exception(EX_II);
  }

  paddr_t physical_base = 0;
  for (unsigned row = 0; row < 8; ++row) {
    for (unsigned lane = 0; lane < 64; ++lane) {
      const uint64_t offset = (uint64_t)row * stride + lane * 4;
      paddr_t physical;
      if (!mld_mx_source_paddr(base + offset, 4, &physical)) {
        longjmp_exception(EX_II);
      }
      if (row == 0 && lane == 0) {
        physical_base = physical;
      } else if (physical_base > UINT64_MAX - offset ||
                 physical != physical_base + offset) {
        longjmp_exception(EX_II);
      }
    }
  }

  uint32_t source[8][64];
  uint8_t scales[8][2];
  bool scale_supported[8][2];
  uint8_t encoded_payload[8][32] = {{0}};
  for (unsigned row = 0; row < 8; ++row) {
    for (unsigned lane = 0; lane < 64; ++lane) {
      source[row][lane] = (uint32_t)vaddr_read(s,
        base + (uint64_t)row * stride + lane * 4, 4, MMU_DYNAMIC);
    }
  }

  for (unsigned row = 0; row < 8; ++row) {
    for (unsigned block = 0; block < 2; ++block) {
      scale_supported[row][block] =
        mxfp_dynamic_scale_e(source[row] + block * 32, &scales[row][block]);
    }
  }

  uint8_t flags = 0;
  bool dynamic_success = true;
  for (unsigned row = 0; row < 8; ++row) {
    for (unsigned block = 0; block < 2; ++block) {
      uint8_t encoded[16] = {0};
      const bool accepted = scale_supported[row][block] &&
        mxfp_encode_supplied_block(source[row] + block * 32,
          scales[row][block], 1, 0, encoded);
      if (!accepted) {
        scales[row][block] = 0;
        flags |= 0x10;
        dynamic_success = false;
      }
      memcpy(encoded_payload[row] + block * sizeof(encoded), encoded,
        sizeof(encoded));
    }
  }

  const unsigned scale_reg = (is_b_side ? 4 : 0) + td;
  memset(cpu.mscale[scale_reg], 127, sizeof(cpu.mscale[scale_reg]));
  for (unsigned row = 0; row < 8; ++row) {
    memcpy(cpu.mscale[scale_reg][row], scales[row], 2);
  }
  cpu.mscale_valid[scale_reg] = dynamic_success;
  cpu.mscale_rows[scale_reg] = 8;
  cpu.mscale_blocks[scale_reg] = 2;
  cpu.mscale_generation[scale_reg]++;
  cpu.mscale_source_stride[scale_reg] = stride;
  cpu.mscale_mode[scale_reg] = 10;
  mfflags->val |= flags;
  mcsr->mfflags = mfflags->val;
  mp_set_dirty();

  amu_ctrl_event_t event = {0};
  event.valid = 1;
  event.op = 1;
  event.md = td;
  event.types2 = !is_b_side;
  event.base = physical_base;
  event.stride = stride;
  event.mtilem = is_b_side ? 64 : 8;
  event.mtilen = is_b_side ? 8 : 64;
  event.typed = 2;
  event.computeType = 10;
  event.isConvertF32 = 1;
  event.isDynamicMxfpQuant = 1;
  event.pc = s->pc;
  amu_matrix_view_t expected_tr = {
    .bytes = &encoded_payload[0][0],
    .capacity = sizeof(encoded_payload),
    .row_pitch = 32,
    .reserved = 0,
  };
  amu_ctrl_queue_push_snapshot(event, flags, &expected_tr);
#ifdef CONFIG_AME_TILEREG_UB_CHECK
  ame_matrix_region_mark_write(td, 8, 32, s->pc);
#endif
#else
  (void)s;
  (void)is_b_side;
  longjmp_exception(EX_II);
#endif
}

void mld_scale(Decode *s, bool is_b_side) {
#if defined(CONFIG_RV_AME_FP4) && defined(CONFIG_FPU_SOFT) && \
    defined(CONFIG_SHARE_REF) && defined(CONFIG_DIFFTEST_AMU_CTRL) && \
    !defined(CONFIG_SHARE_CTRL)
  require_matrix();
  const uint64_t scale_reg = s->dest.reg;
  const word_t rows = is_b_side ? mtilen->val : mtilem->val;
  const uint64_t base = reg_l(s->src1.reg);
  const uint64_t stride = reg_l(s->src2.reg);
  if (!amu_snapshot_ready() || scale_reg >= 4 || mtilek->val != 64 ||
      rows != 8 || stride < 2 || stride > (UINT64_MAX - 1) / 7 ||
      base > UINT64_MAX - (7 * stride + 1)) {
    longjmp_exception(EX_II);
  }

  mcfg_t fp32_cfg = {0};
  fp32_cfg.type_code = MTYPECODE_FP32;
  const uint8_t mode = get_mxfp_mmacc_mode(cpu.mcfg[scale_reg],
    cpu.mcfg[scale_reg], fp32_cfg);
  const unsigned blocks = 2;
  const unsigned scale_index = (is_b_side ? 4 : 0) + scale_reg;
  if (mode != 10 || cpu.mscale_generation[scale_index] == UINT32_MAX) {
    longjmp_exception(EX_II);
  }

  paddr_t physical_base = 0;
  for (unsigned row = 0; row < rows; ++row) {
    for (unsigned block = 0; block < blocks; ++block) {
      const uint64_t offset = (uint64_t)row * stride + block;
      paddr_t physical;
      if (!mld_mx_source_paddr(base + offset, 1, &physical)) {
        longjmp_exception(EX_II);
      }
      if (row == 0 && block == 0) {
        physical_base = physical;
      } else if (physical_base > UINT64_MAX - offset ||
                 physical != physical_base + offset) {
        longjmp_exception(EX_II);
      }
    }
  }

  uint8_t loaded[8][2];
  for (unsigned row = 0; row < rows; ++row) {
    for (unsigned block = 0; block < blocks; ++block) {
      loaded[row][block] = (uint8_t)vaddr_read(s,
        base + (uint64_t)row * stride + block, 1, MMU_DYNAMIC);
    }
  }

  for (unsigned row = 0; row < rows; ++row) {
    memcpy(cpu.mscale[scale_index][row], loaded[row], blocks);
  }
  cpu.mscale_valid[scale_index] = 1;
  cpu.mscale_rows[scale_index] = rows;
  cpu.mscale_blocks[scale_index] = blocks;
  cpu.mscale_generation[scale_index]++;
  cpu.mscale_source_stride[scale_index] = stride;
  cpu.mscale_mode[scale_index] = mode;
  mp_set_dirty();

  amu_ctrl_event_t event = {0};
  event.valid = 1;
  event.op = 1;
  event.md = scale_reg;
  event.types2 = !is_b_side;
  event.base = physical_base;
  event.stride = stride;
  event.mtilem = rows;
  event.mtilen = blocks;
  event.computeType = mode;
  event.isScale = 1;
  event.pc = s->pc;
  amu_ctrl_queue_push(event);
#else
  (void)s;
  (void)is_b_side;
  longjmp_exception(EX_II);
#endif
}

#endif // CONFIG_RV_AME
