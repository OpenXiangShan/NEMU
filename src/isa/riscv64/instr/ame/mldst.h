/***************************************************************************************
* Copyright (c) 2020-2022 Institute of Computing Technology, Chinese Academy of Sciences
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

#include "cpu/cpu.h"
#include <common.h>

#ifdef CONFIG_RV_AME

#include "cpu/exec.h"
#include "mreg.h"
#include "../local-include/csr.h"
#include "../local-include/intr.h"
#include "../local-include/rtl.h"
#include "../local-include/reg.h"
#include "ext/mstore_queue_wrapper.h"
#include "mcommon.h"
#include "mcompute_impl.h"
#include "mxfp.h"
#include <stdlib.h>
#include <ext/amu_ctrl_queue_wrapper.h>
#include <ext/cutest.h>

// #define PRINT_AMUCTRLIO

#ifdef PRINT_AMUCTRLIO
#include <stdio.h>
#endif // PRINT_AMUCTRLIO

uint8_t get_size(mcfg_t cfg) {
  uint8_t dsize = 0;
  switch (cfg.type_code) {
    // set m_d_sz
    // 0 for 8bit, 1 for 16bit, 2 for 32bit, 3 for 4bit
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

void check_size(Decode *s, int rmax_mreg, int cmax_mreg, char m_name, uint8_t dsize) {
  bool valid = true;
  if (rmax_mreg > ROWNUM) {
    valid = false;
  }
  switch (m_name) {
    case 'a': case 'b':
      if (cmax_mreg > TRENUM8 / (1 << dsize)) {
        valid = false;
      }
      break;
    case 'c':
      if (cmax_mreg > ARENUM8 / (1 << dsize)) {
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

void exec_mld(Decode *s, uint64_t base_addr, uint64_t row_byte_stride,
              uint64_t td, int row, int column, uint8_t dsize,
              bool is_trans, char m_name) {
  mp_set_dirty();

#ifdef PRINT_AMUCTRLIO
  fprintf(stderr,
    "[AmuCtrlIO] op=1 \n"
    "            ms=%ld, ls=0, transpose=%d, baseVAddr=%#lx, stride=%#lx\n"
    "            row=%d, col=%d, width=%#x, m_name=%c\n",
    td, is_trans, base_addr, row_byte_stride, row, column, dsize, m_name);
#endif

  rtl_lmm(s, &base_addr, &row_byte_stride,
    row, column, dsize, is_trans,
    MMU_TRANSLATE, m_name, td);
}

void exec_mst(Decode *s, uint64_t base_addr, uint64_t row_byte_stride,
              uint64_t ts3, int row, int column, uint8_t dsize,
              bool is_trans, char m_name) {
#ifdef PRINT_AMUCTRLIO
  fprintf(stderr,
    "[AmuCtrlIO] op=1 \n"
    "            ms=%ld, ls=1, transpose=%d, baseVAddr=%#lx, stride=%#lx\n"
    "            row=%d, col=%d, width=%#x, m_name=%c\n",
    ts3, is_trans, base_addr, row_byte_stride, row, column, dsize, m_name);
#endif

  rtl_smm(s, &base_addr, &row_byte_stride,
    row, column, dsize, is_trans,
    MMU_TRANSLATE, m_name, ts3);

  mstore_queue_emplace(base_addr, row_byte_stride, row, column, dsize, is_trans);
}

void exec_mld_mxfp4(Decode *s, uint64_t base_addr, uint64_t row_byte_stride,
                    uint64_t td, int row, int column) {
  mp_set_dirty();
  const int packed_bytes = (column + 1) / 2;
  for (int r = 0; r < row; r++) {
    for (int c = 0; c < packed_bytes; c++) {
      const uint8_t packed =
        (uint8_t) vaddr_read(s, base_addr + r * row_byte_stride + c, 1, MMU_DYNAMIC);
#ifndef CONFIG_SHARE_REF
      const int k0 = c * 2;
      set_mtreg_nibble(td & 0x3, r, k0, packed & 0x0f);
      if (k0 + 1 < column) {
        set_mtreg_nibble(td & 0x3, r, k0 + 1, packed >> 4);
      }
#else
      // Like ordinary matrix loads, the REF instruction validates the access
      // now, but SWROB-head execution owns the physical matrix update.
      (void)packed;
#endif
    }
  }
}

// m_name: 'a'/'b'/'c'
void mld(Decode *s, bool is_trans, char m_name) {
  uint64_t td = s->dest.reg;
  mcfg_t cfg = cpu.mcfg[td];
  if (is_mx_mcfg(cfg)) {
    require_mx_transport();
  }
  uint8_t dsize = get_size(cfg);
  const bool mxfp4 = cfg.table_set == 1 && cfg.type_code == MTYPECODE_MXFP4;
  int rmax_mreg = 0, cmax_mreg = 0;

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

  if (mxfp4 && m_name != 'c' && !is_trans) {
    const word_t active_rows = m_name == 'a' ? mtilem->val : mtilen->val;
    if (active_rows > ROWNUM || mtilek->val > 2 * TRENUM8) {
      longjmp_exception(EX_II);
    }
    exec_mld_mxfp4(s, reg_l(s->src1.reg), reg_l(s->src2.reg), td,
      rmax_mreg, cmax_mreg);
#if defined(CONFIG_DIFFTEST_AMU_CTRL) || defined(CONFIG_SHARE_CTRL)
    const vaddr_t base = reg_l(s->src1.reg);
    paddr_t physical_base = base;
    if (isa_mmu_check(base, 1, MEM_TYPE_READ) == MMU_TRANSLATE) {
      paddr_t page = isa_mmu_translate(base, 1, MEM_TYPE_READ);
      Assert((page & PAGE_MASK) == MEM_RET_OK, "packed matrix base translation failed");
      physical_base = page | (base & PAGE_MASK);
    }
    // Match MlsUnit's canonical MLS units: dimensions are logical elements,
    // width=e4(7), and data-load computeType=0 (only scale MLS has a mode).
#ifdef CONFIG_DIFFTEST_AMU_CTRL
    amu_ctrl_queue_mls_emplace(td, false, false, false, m_name == 'a',
      physical_base, reg_l(s->src2.reg),
      m_name == 'b' ? cmax_mreg : rmax_mreg,
      m_name == 'b' ? rmax_mreg : cmax_mreg, 7);
#endif
#ifdef CONFIG_SHARE_CTRL
    cutest_mls_emplace(td, false, false, false, m_name == 'a',
      physical_base, reg_l(s->src2.reg),
      m_name == 'b' ? cmax_mreg : rmax_mreg,
      m_name == 'b' ? rmax_mreg : cmax_mreg, 7);
#endif
#endif
    return;
  }

  check_size(s, rmax_mreg, cmax_mreg, m_name, dsize);
  exec_mld(s, reg_l(s->src1.reg), reg_l(s->src2.reg), td,
    rmax_mreg, cmax_mreg, dsize, is_trans, m_name);
}

void mst(Decode *s, bool is_trans, char m_name) {
  uint64_t ts3 = s->dest.reg;
  uint8_t dsize = get_size(cpu.mcfg[ts3]);
  int rmax_mreg = 0, cmax_mreg = 0;

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

  check_size(s, rmax_mreg, cmax_mreg, m_name, dsize);
  exec_mst(s, reg_l(s->src1.reg), reg_l(s->src2.reg), ts3,
    rmax_mreg, cmax_mreg, dsize, is_trans, m_name);
}

void mld_whole(Decode *s, char m_name) {
  uint64_t td = s->dest.reg;
  uint8_t dsize = m_name == 'c' ? 2 : 0;

  exec_mld(s, reg_l(s->src1.reg), reg_l(s->src2.reg), td,
    ROWNUM, m_name == 'c' ? ARENUM32 : TRENUM8, dsize, false, m_name);
}

void mst_whole(Decode *s, char m_name) {
  uint64_t ts3 = s->dest.reg;
  uint8_t dsize = m_name == 'c' ? 2 : 0;

  exec_mst(s, reg_l(s->src1.reg), reg_l(s->src2.reg), ts3,
    ROWNUM, m_name == 'c' ? ARENUM32 : TRENUM8, dsize, false, m_name);
}

void mld_scale(Decode *s, bool is_b_side) {
  require_matrix();
  require_mx_transport();

  const int scale_reg = s->dest.reg & 0x3;
  if (s->dest.reg > 3 || mtilek->val == 0 ||
      mtilek->val > 2 * TRENUM8) {
    longjmp_exception(EX_II);
  }

  const word_t rows = is_b_side ? mtilen->val : mtilem->val;
  const int blocks = (mtilek->val + AME_MX_BLOCK_SIZE - 1) / AME_MX_BLOCK_SIZE;
  const uint64_t base = reg_l(s->src1.reg);
  const uint64_t stride = reg_l(s->src2.reg);
  if (rows > ROWNUM || blocks <= 0 ||
      blocks > AME_MAX_SCALE_BLOCKS || stride < (uint64_t) blocks) {
    longjmp_exception(EX_II);
  }

  const int side = is_b_side ? 4 : 0;
  const uint8_t mode = mx_compute_type(cpu.mcfg[scale_reg]);
  if (cpu.mscale_generation[side + scale_reg] == UINT32_MAX) {
    longjmp_exception(EX_II);
  }
  uint8_t loaded[ROWNUM][AME_MAX_SCALE_BLOCKS];
  for (word_t row = 0; row < rows; row++) {
    for (int block = 0; block < blocks; block++) {
      loaded[row][block] =
        (uint8_t) vaddr_read(s, base + row * stride + block, 1, MMU_DYNAMIC);
    }
  }
  // Publish atomically after every read succeeds. A faulting reload must not
  // leave new bytes reachable through the previous successful load's shape.
  for (word_t row = 0; row < rows; row++) {
    memcpy(cpu.mscale[side + scale_reg][row], loaded[row], blocks);
  }
  cpu.mscale_rows[side + scale_reg] = rows;
  cpu.mscale_blocks[side + scale_reg] = blocks;
  cpu.mscale_valid[side + scale_reg] = 1;
  cpu.mscale_generation[side + scale_reg]++;
  cpu.mscale_source_stride[side + scale_reg] = stride;
  cpu.mscale_mode[side + scale_reg] = mode;
  mp_set_dirty();
#if defined(CONFIG_DIFFTEST_AMU_CTRL) || defined(CONFIG_SHARE_CTRL)
  // Canonical CPU payload units: row=M/N, column=ceil(K/32), width=e8.
  paddr_t physical_base = base;
  if (isa_mmu_check(base, 1, MEM_TYPE_READ) == MMU_TRANSLATE) {
    paddr_t page = isa_mmu_translate(base, 1, MEM_TYPE_READ);
    Assert((page & PAGE_MASK) == MEM_RET_OK, "scale base translation failed");
    physical_base = page | (base & PAGE_MASK);
  }
  amu_ctrl_event_t event = {0};
  event.valid = 1;
  event.op = 1;
  event.md = scale_reg;
  event.types2 = !is_b_side;
  event.base = physical_base;
  event.stride = stride;
  event.mtilem = rows;
  event.mtilen = blocks;
  event.computeType = mx_compute_type(cpu.mcfg[scale_reg]);
  event.isScale = 1;
  event.pc = s->pc;
#ifdef CONFIG_DIFFTEST_AMU_CTRL
  amu_ctrl_queue_push(event, 0, NULL);
#endif
#ifdef CONFIG_SHARE_CTRL
  cutest_scale_emplace(event);
#endif
#endif
}

// Standalone supplied mode consumes SF published by mld_scale; dynamic mode is
// its own same-slot generated-SF producer. Shared-reference mode admits only
// dynamic and defers physical TR installation to the ordered exec seam. A
// missing/mismatched supplied SF is a fail-stop precondition violation, not a
// numeric NV or a newly promised precise architectural SF exception.
void mld_convert_f32_mx(Decode *s, bool is_b_side) {
#if (defined(CONFIG_RV_AME_MXFP_CONVERT_LOAD) && defined(CONFIG_FPU_SOFT) && \
     !defined(CONFIG_SHARE) && !defined(CONFIG_SHARE_REF) && \
     !defined(CONFIG_SHARE_CTRL) && !defined(CONFIG_DIFFTEST_AMU_CTRL)) || \
    (defined(CONFIG_RV_AME_MXFP_CONVERT_LOAD) && \
     defined(CONFIG_RV_AME_MXFP_DYNAMIC_QUANT) && defined(CONFIG_FPU_SOFT) && \
     defined(CONFIG_SHARE_REF) && defined(CONFIG_DIFFTEST_AMU_CTRL) && \
     !defined(CONFIG_SHARE_CTRL))
  require_matrix();
  require_mx_transport();
  const uint64_t td = s->dest.reg;
  // Validate before indexing MCFG/TR, reading memory or changing any flags.
  if (td > 3) longjmp_exception(EX_II);
  const mcfg_t cfg = cpu.mcfg[td];
  const word_t rows = is_b_side ? mtilen->val : mtilem->val;
#ifdef CONFIG_RV_AME_MXFP_DYNAMIC_QUANT
  const uint32_t high7 = BITS(s->isa.instr.val, 31, 25);
  const bool dynamic = high7 == 0x1c || high7 == 0x5c;
#else
  const bool dynamic = false;
#endif
#ifdef CONFIG_SHARE_REF
  const bool shared_reference = true;
#else
  const bool shared_reference = false;
#endif
  const bool fp4 = cfg.type_code == MTYPECODE_MXFP4;
  const unsigned row_bytes = fp4 ? 32 : 64;
  const uint64_t base = reg_l(s->src1.reg), stride = reg_l(s->src2.reg);
  if (!is_mx_mcfg(cfg) || (shared_reference && !dynamic) ||
      mtilek->val != 64 || rows == 0 ||
      rows > 64 || rows > ROWNUM || row_bytes > TRENUM8 ||
      stride < 256 || (stride & 3) ||
      (dynamic && (!fp4 || rows != 8))) {
    longjmp_exception(EX_II);
  }
  if (base & 3) {
    // Same load-address-misaligned cause/tval owner as the scalar MMU path;
    // the explicit F32 instruction requires alignment regardless of AC_SOFT.
    cpu.trapInfo.tval = base;
    longjmp_exception(EX_LAM);
  }
  const uint8_t mode = mx_compute_type(cfg);
  const int scale_reg = (is_b_side ? 4 : 0) + td;
  if (dynamic && cpu.mscale_generation[scale_reg] == UINT32_MAX) {
    longjmp_exception(EX_II);
  }
  if (!dynamic) {
    // Keep this single-use predicate in its existing owner: PERF_OPT expands
    // this header inside execute(), where a static inline definition is illegal.
    if (!(cpu.mscale_valid[scale_reg] && cpu.mscale_generation[scale_reg] > 0 &&
          cpu.mscale_mode[scale_reg] == mode && cpu.mscale_rows[scale_reg] == rows &&
          cpu.mscale_blocks[scale_reg] == 2)) {
      panic("experimental converting load SF precondition: side=%c rd=%lu "
        "mode=%u rows=%lu need blocks=2, published valid=%u generation=%u "
        "mode=%u rows=%u blocks=%u", is_b_side ? 'B' : 'A', td, mode, rows,
        cpu.mscale_valid[scale_reg], cpu.mscale_generation[scale_reg],
        cpu.mscale_mode[scale_reg], cpu.mscale_rows[scale_reg],
        cpu.mscale_blocks[scale_reg]);
      abort();
    }
  }

  // Single-hart M-mode, valid full containing-line physical DRAM is a caller
  // precondition. Do not model bank/WAR timing or promise late-fault recovery.
  // Capture supplied SF, then complete EVERY source read before the first
  // TR/SF/flags write.
  uint32_t source[64][64];
  uint8_t scales[64][2];
#ifdef CONFIG_SHARE_REF
  // Shared mode admits only 8-row FP4 dynamic conversion. Keep the compact
  // owner result separate from the per-block encoder scratch below.
  uint8_t encoded_payload[8][32] = {{0}};
#endif
  if (!dynamic) {
    for (word_t row = 0; row < rows; ++row) {
      memcpy(scales[row], cpu.mscale[scale_reg][row], 2);
    }
  }
  for (word_t row = 0; row < rows; ++row) {
    for (unsigned k = 0; k < 64; ++k) {
      source[row][k] = (uint32_t)vaddr_read(s,
        base + row * stride + k * 4, 4, MMU_DYNAMIC);
    }
  }
  bool dynamic_supported[64][2];
  if (dynamic) {
    for (word_t row = 0; row < rows; ++row) {
      for (unsigned block = 0; block < 2; ++block) {
        dynamic_supported[row][block] =
          mxfp_dynamic_scale_e(source[row] + block * 32, &scales[row][block]);
      }
    }
    // NEMU stores SF row-major. The compact generated 64B line leaves every
    // unused logical byte at the canonical E8M0 padding value 127.
    memset(cpu.mscale[scale_reg], 127, sizeof(cpu.mscale[scale_reg]));
  }
  uint8_t flags = 0;
  bool dynamic_success = true;
  const unsigned block_bytes = row_bytes / 2;
  for (word_t row = 0; row < rows; ++row) {
    for (unsigned block = 0; block < 2; ++block) {
      uint8_t encoded[32] = {0};
      bool accepted = mxfp_encode_supplied_block(source[row] + block * 32,
          scales[row][block], fp4, cfg.type_code == MTYPECODE_FP8E5M2, encoded);
      if (dynamic) {
        accepted &= dynamic_supported[row][block];
        dynamic_success &= accepted;
        cpu.mscale[scale_reg][row][block] = accepted ? scales[row][block] : 0;
      }
      if (!accepted) memset(encoded, 0, block_bytes);
#ifdef CONFIG_SHARE_REF
      if (dynamic) {
        assert(row < 8 && block < 2);
        memcpy(encoded_payload[row] + block * block_bytes, encoded, block_bytes);
      }
#endif
      if (!accepted) {
        flags |= 0x10;
      }
#ifndef CONFIG_SHARE_REF
      // Standalone conversion writes its physical TR immediately. The helper
      // writes zero on reject and preserves inactive rows/tail bytes.
      for (unsigned b = 0; b < block_bytes; ++b) {
        set_mreg(td, row, block * block_bytes + b, accepted ? encoded[b] : 0, 0);
      }
#endif
    }
  }
  if (dynamic) {
    cpu.mscale_rows[scale_reg] = rows;
    cpu.mscale_blocks[scale_reg] = 2;
    cpu.mscale_generation[scale_reg]++;
    cpu.mscale_source_stride[scale_reg] = stride;
    cpu.mscale_mode[scale_reg] = mode;
    cpu.mscale_valid[scale_reg] = dynamic_success;
  }
  mfflags->val |= flags;
  mcsr->mfflags = mfflags->val;
  mp_set_dirty();
#ifdef CONFIG_SHARE_REF
  // SHARE_REF owns only a local compact result here. Do not advance physical
  // cpu.mtr: an older ordinary/MMA operation may still be the SWROB head. The
  // existing control queue retains this same owner through immutable DUT
  // comparison, and exec_amu applies its REF-owned bytes in program order.
  paddr_t physical_base = base;
  if (isa_mmu_check(base, 1, MEM_TYPE_READ) == MMU_TRANSLATE) {
    paddr_t page = isa_mmu_translate(base, 1, MEM_TYPE_READ);
    Assert((page & PAGE_MASK) == MEM_RET_OK, "dynamic convert base translation failed");
    physical_base = page | (base & PAGE_MASK);
  }
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
  event.computeType = mode;
  event.isConvertF32 = 1;
  event.isDynamicMxfpQuant = 1;
  event.pc = s->pc;
  amu_matrix_view_t ref_result = {
    .bytes = &encoded_payload[0][0],
    .capacity = 8 * 32,
    .row_pitch = 32,
    .reserved = 0,
  };
  amu_ctrl_queue_push(event, flags, &ref_result);
#endif
#else
  // Defense in depth: even a direct helper call cannot bypass decode gating
  // or execute as an ordinary load in any shared configuration.
  (void)s; (void)is_b_side;
  longjmp_exception(EX_II);
#endif
}

def_EHelper(mlacvt_f32_mx) {
  mld_convert_f32_mx(s, false);
}

def_EHelper(mlbcvt_f32_mx) {
  mld_convert_f32_mx(s, true);
}

def_EHelper(mla) {
  mld(s, false, 'a');
}

def_EHelper(mlb) {
  mld(s, false, 'b');
}

def_EHelper(mlc) {
  mld(s, false, 'c');
}

def_EHelper(mlat) {
  mld(s, true, 'a');
}

def_EHelper(mlbt) {
  mld(s, true, 'b');
}

def_EHelper(mlct) {
  mld(s, true, 'c');
}

def_EHelper(msa) {
  mst(s, false, 'a');
}

def_EHelper(msb) {
  mst(s, false, 'b');
}

def_EHelper(msc) {
  mst(s, false, 'c');
}

def_EHelper(msat) {
  mst(s, true, 'a');
}

def_EHelper(msbt) {
  mst(s, true, 'b');
}

def_EHelper(msct) {
  mst(s, true, 'c');
}

def_EHelper(mlawhole) {
  mld_whole(s, 'a');
}

def_EHelper(mlbwhole) {
  mld_whole(s, 'b');
}

def_EHelper(mlcwhole) {
  mld_whole(s, 'c');
}

def_EHelper(msawhole) {
  mst_whole(s, 'a');
}

def_EHelper(msbwhole) {
  mst_whole(s, 'b');
}

def_EHelper(mscwhole) {
  mst_whole(s, 'c');
}

def_EHelper(mlascale) {
  mld_scale(s, false);
}

def_EHelper(mlbscale) {
  mld_scale(s, true);
}

#endif // CONFIG_RV_AME
