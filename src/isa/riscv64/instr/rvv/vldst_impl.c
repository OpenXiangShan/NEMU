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

#include "cpu/decode.h"
#include "debug.h"
#include "macro.h"
#include "memory/paddr.h"
#include "rtl/rtl.h"
#include <common.h>
#include <stdint.h>
#include <string.h>
#include <sys/cdefs.h>
#ifdef CONFIG_RVV

#include <cpu/cpu.h>
#include <cpu/difftest.h>
#include "vldst_impl.h"
#include "vcompute_impl.h"
#include "../local-include/intr.h"
#include "../local-include/trigger.h"

word_t fofvl = 0;
word_t vstvaltmp = 0;
word_t stvaltmp  = 0;
word_t mtvaltmp  = 0;

void isa_vec_misalign_data_addr_check(vaddr_t vaddr, int len, int type);
// reference: v_ext_macros.h in riscv-isa-sim

static void isa_emul_check(int emul, int nfields) {
  if (emul > 3) {
    Loge("vector EMUL > 8 happen: EMUL:%d\n", (1 << emul));
    longjmp_exception(EX_II);
  }
  if (emul < -3) {
    Loge("vector EMUL < 1/8 happen: EMUL:1/%d\n", 1 << (-emul));
    longjmp_exception(EX_II);
  }
  int real_emul = 1 << (emul < 0 ? 0 : emul);
  if (real_emul * nfields > 8) {
    Loge("vector EMUL * NFIELDS > 8 happen: EMUL:%s%d NFIELDS:%d\n",
      emul > 0 ? "" : "1/",
      emul > 0 ? real_emul : (1 << (-emul)),
      nfields
    );
    longjmp_exception(EX_II);
  }
}

static void vstore_check(int mode, Decode *s) {
  int eew = 0;
  switch(s->v_width) {
    case 1: eew = 0; break;
    case 2: eew = 1; break;
    case 4: eew = 2; break;
    case 8: eew = 3; break;
    default: Loge("illegal v_width: %d\n", s->v_width);
    longjmp_exception(EX_II); break;
  }
  uint64_t veew = mode == MODE_MASK ? 1 : 8 << eew;
  uint64_t vsew = 8 << vtype->vsew;
  double vflmul = compute_vflmul();
  float vemul = mode == MODE_MASK ? 1 : ((float)veew / vsew * vflmul);
  uint64_t emul = vemul < 1 ? 1 : vemul;
  if (!(vemul >= 0.125 && vemul <= 8)) {
    Loge("illegal EMUL: %f\n", vemul);
    longjmp_exception(EX_II);
  }
  require_aligned(id_dest->reg, vemul);
  uint64_t nf = s->v_nf + 1;
  if (!((nf * emul <= 8) && (id_dest->reg + nf * emul <= 32))) {
    Loge("illegal NFIELDS: %lu EMUL: %lu\n", nf, emul);
    longjmp_exception(EX_II);
  }
}

static void vload_check(int mode, Decode *s) {
  vstore_check(mode, s);
  require_vm(s);
}

static void index_vstore_check(Decode *s) {
  int eew = vtype->vsew;
  int elt_width = 0;
  switch(s->v_width) {
    case 1: elt_width = 0; break;
    case 2: elt_width = 1; break;
    case 4: elt_width = 2; break;
    case 8: elt_width = 3; break;
    default: break;
  }
  double vflmul = compute_vflmul();
  float vemul = (float)(8 << elt_width) / (8 << eew) * vflmul;
    if (!(vemul >= 0.125 && vemul <= 8)) {
    Loge("illegal EMUL: %f\n", vemul);
    longjmp_exception(EX_II);
  }

  uint64_t flmul = vflmul < 1 ? 1 : vflmul;

  require_aligned(id_dest->reg, vflmul);
  require_aligned(id_src2->reg, vemul);

  uint64_t nf = s->v_nf + 1;
  if (!((nf * flmul <= 8) && (id_dest->reg + nf * flmul <= 32))) {
    Loge("illegal NFIELDS: %lu LMUL: %lu\n", nf, flmul);
    longjmp_exception(EX_II);
  }
}

static void index_vload_check(Decode *s) {
  index_vstore_check(s);
  int eew = vtype->vsew;
  int elt_width = 0;
  switch(s->v_width) {
    case 1: elt_width = 0; break;
    case 2: elt_width = 1; break;
    case 4: elt_width = 2; break;
    case 8: elt_width = 3; break;
    default: break;
  }
  uint64_t nf = s->v_nf + 1;
  double vflmul = compute_vflmul();
  float vemul = (float)(8 << elt_width) / (8 << eew) * vflmul;
  uint64_t flmul = vflmul < 1 ? 1 : vflmul;
  for (uint64_t idx = 0; idx < nf; idx++) {
    uint64_t seg_vd = id_dest->reg + idx * flmul;
    if (elt_width > eew) {
      if (seg_vd != id_src2->reg) {
        require_noover(seg_vd, vflmul, id_src2->reg, vemul);
      }
    } else if (elt_width < eew) {
      if (vemul < 1) {
        require_noover(seg_vd, vflmul, id_src2->reg, vemul);
      } else {
        require_noover_widen(seg_vd, vflmul, id_src2->reg, vemul);
      }
    }
    if (nf >= 2) {
      require_noover(seg_vd, vflmul, id_src2->reg, vemul);
    }
  }
  require_vm(s);
}

#ifndef CONFIG_SHARE
static inline unsigned gen_mask_for_unit_stride(Decode *s, int eew, vstart_t *vstart, uint64_t vl_val,
    uint8_t * restrict masks) {
  unsigned count = 0;
  switch (eew) {
  case 0: {
    for (word_t i = vstart->val; i < vl_val; i++) {
      masks[i] = get_mask(0, i) ? 0xff : 0;
      Logm("masks[%ld] = %x", i, masks[i]);
      masks[i] |= s->vm != 0 ? 0xff : 0;
      count += masks[i] != 0;
    };
    break;
  }
  case 1: {
    uint16_t *x_masks = (uint16_t *)masks;
    for (uint64_t i = vstart->val; i < vl_val; i++) {
      Assert(vl_val <= 64, "vl_val > 64");
      x_masks[i] = get_mask(0, i) ? 0xffff : 0;
      Logm("xmasks[%ld] = %x", i, x_masks[i]);
      x_masks[i] |= s->vm != 0 ? 0xffff : 0;
      count += x_masks[i] != 0;
    };
    break;
  }
  case 2: {
    uint32_t *x_masks = (uint32_t *)masks;
    for (uint64_t i = vstart->val; i < vl_val; i++) {
      Assert(vl_val <= 32, "vl_val > 32");
      x_masks[i] = get_mask(0, i) ? ~0U : 0;
      Logm("xmasks[%ld] = %x", i, x_masks[i]);
      x_masks[i] |= s->vm != 0 ? ~0U : 0;
      count += x_masks[i] != 0;
    };
    break;
  }
  case 3: {
    uint64_t *x_masks = (uint64_t *)masks;
    for (uint64_t i = vstart->val; i < vl_val; i++) {
      Assert(vl_val <= 16, "vl_val > 16");
      x_masks[i] = get_mask(0, i) ? ~(0UL) : 0;
      Logm("xmasks[%ld] = %lx", i, x_masks[i]);
      x_masks[i] |= s->vm != 0 ? ~(0UL) : 0;
      count += x_masks[i] != 0;
    }
    break;
  }
  default:
    Assert(0, "eew >= 4 is reserved");
  }
  return count;
}

#endif // CONFIG_SHARE

#ifdef CONFIG_MULTICORE_DIFF
extern bool need_read_golden_mem;

uint32_t vec_laod_mul = 0;
uint64_t vec_read_golden_mem_addr = 0;
uint64_t vec_read_golden_mem_data = 0;

uint64_t vec_load_diffteset_buf[8] = {0};

uint64_t vec_load_difftest_addr_queue[128] = {0};
uint64_t vec_load_difftest_data_queue[128] = {0};
uint8_t  vec_load_difftest_len_queue[128]  = {0};
uint32_t vec_load_difftest_info_queue_cnt = 0;

void init_vec_load_difftest_info(int mul, int vd) {
  vec_laod_mul = mul;
  for (int i = 0; i < mul; i++) {
    set_vec_dual_difftest_reg(i, 0, cpu.vr[vd+i]._64[0], 3);
    set_vec_dual_difftest_reg(i, 1, cpu.vr[vd+i]._64[1], 3);
  }
  vec_load_difftest_info_queue_cnt = 0;
}

void set_vec_load_difftest_info(int fn, int len) {
  vec_load_diffteset_buf[fn] = vec_read_golden_mem_data;
  vec_load_difftest_addr_queue[vec_load_difftest_info_queue_cnt] = vec_read_golden_mem_addr;
  vec_load_difftest_data_queue[vec_load_difftest_info_queue_cnt] = vec_read_golden_mem_data;
  vec_load_difftest_len_queue[vec_load_difftest_info_queue_cnt]  = len;
  vec_load_difftest_info_queue_cnt ++;
}
#endif // CONFIG_MULTICORE_DIFF

void vld(Decode *s, int mode, int mmu_mode) {
  vload_check(mode, s);
  if(check_vstart_ignore(s)) return;
  uint64_t nf, fn, vl_val, base_addr, vd, addr, is_unit_stride;
  int64_t stride;
  int eew, emul, vemul;

  // s->v_width is the bytes of a unit
  // eew is the coding like vsew
  eew = 0;
  switch(s->v_width) {
    case 1: eew = 0; break;
    case 2: eew = 1; break;
    case 4: eew = 2; break;
    case 8: eew = 3; break;
    default: break;
  }
  emul = vtype->vlmul > 4 ? vtype->vlmul - 8 + eew - vtype->vsew : vtype->vlmul + eew - vtype->vsew;
  isa_emul_check(mode == MODE_MASK ? 1 : emul, 1);
  emul = emul < 0 ? 0 : emul;
  vemul = emul;
  emul = 1 << emul;

  if (mode == MODE_STRIDED) {
    stride = id_src2->val;
    is_unit_stride = 0;
  } else {
    stride = 0;
    is_unit_stride = 1;
  }
  // previous decode does not load vals for us
  rtl_lr(s, &(s->src1.val), s->src1.reg, 4);
  rtl_mv(s, &(tmp_reg[0]), &(s->src1.val));

  nf = s->v_nf + 1;
  vl_val = mode == MODE_MASK ? (vl->val + 7) / 8 : vl->val;
  base_addr = tmp_reg[0];
  vd = id_dest->reg;

  IFDEF(CONFIG_MULTICORE_DIFF, init_vec_load_difftest_info(emul, vd));

  bool fast_vle = false;

#if !defined(CONFIG_SHARE) && !defined(CONFIG_TDATA1_MCONTROL6)
  uint64_t start_addr = base_addr + (vstart->val * nf) * s->v_width;
  uint64_t last_addr = base_addr + (vl_val * nf - 1) * s->v_width;
  uint64_t vle_size = last_addr - start_addr + s->v_width;
  __attribute__((unused)) bool cross_page = last_addr / PAGE_SIZE != start_addr / PAGE_SIZE;
  uint8_t masks[VLMAX_8] = {0};

  Logm("vld start_addr: %#lx, v_width: %u, vl_val: %lu, vle size=%lu, vstart->val: %lu, nf=%lu",
      base_addr, s->v_width, vl_val, vle_size, vstart->val, nf);

  if (is_unit_stride && nf == 1 && vl_val > vstart->val && vtype->vlmul < 4 && !cross_page) {
    s->last_access_host_addr = NULL;
    extern void dummy_vaddr_data_read(struct Decode *s, vaddr_t addr, int len, int mmu_mode);
    dummy_vaddr_data_read(s, start_addr, s->v_width, mmu_mode);
    // Now we have the host address of first element in Decode *s->last_access_host_addr
    if (s->last_access_host_addr != NULL) {

      // get address of first element in register file
      void *reg_file_addr = NULL;
      get_vreg_with_addr(vd, vstart->val, &tmp_reg[1], eew, 0, 0, 0, &reg_file_addr);
      Assert(reg_file_addr != NULL, "reg_file_addr is NULL");
      uint8_t * restrict reg_file_addr_8 = reg_file_addr;

      __attribute__((unused)) unsigned count = gen_mask_for_unit_stride(s, eew, vstart, vl_val, masks);

      uint8_t invert_masks[VLMAX_8] = {0};
      uint8_t * restrict last_access_host_addr_u8 = s->last_access_host_addr;
      
#ifdef DEBUG_FAST_VLE
      switch (s->v_width) {
        case 1: for (int i = 0; i < vle_size; i++) {
            Logm("Element %i, mask = %x, inv mask = %x, reg = %x, mem = %x", i,
                 masks[i], invert_masks[i], reg_file_addr_8[i],
                 last_access_host_addr[i]);
          }
          break;
        case 2:
          for (int i = 0; i < vle_size; i += 2) {
            Logm("Element %i, mask = %x, inv mask = %x, reg = %x, mem = %x", i,
                 *(uint16_t *)&masks[i], *(uint16_t *)&invert_masks[i],
                 *(uint16_t *)&reg_file_addr_8[i],
                 *(uint16_t *)&last_access_host_addr[i]);
          }
          break;
        case 4:
          for (int i = 0; i < vle_size; i += 4) {
            Logm("Element %i, mask = %x, inv mask = %x, reg = %x, mem = %x", i,
                 *(uint32_t *)&masks[i], *(uint32_t *)&invert_masks[i],
                 *(uint32_t *)&reg_file_addr_8[i],
                 *(uint32_t *)&last_access_host_addr[i]);
          }
          break;
        case 8:
          for (int i = 0; i < vle_size; i += 8) {
            Logm("Element %i, mask = %lx, inv mask = %lx, reg = %lx, mem = %lx",
                 i, *(uint64_t *)&masks[i], *(uint64_t *)&invert_masks[i],
                 *(uint64_t *)&reg_file_addr_8[i],
                 *(uint64_t *)&last_access_host_addr[i]);
          }
          break;
        default:
                panic("Unexpected vwidth = %d", s->v_width);
      }
# endif // DEBUG_FAST_VLE

      for (int i = 0; i < VLMAX_8; i++) {
        invert_masks[i] = ~masks[i];
        masks[i] &= last_access_host_addr_u8[i];
        if (RVV_AGNOSTIC && vtype->vma) {
          invert_masks[i] = 0xff;
        } else {
          invert_masks[i] &= reg_file_addr_8[i];
        }
        masks[i] |= invert_masks[i];
      }
      memcpy(reg_file_addr, masks, vle_size);
      fast_vle = true;
    }
  }
#endif // !CONFIG_SHARE && !CONFIG_TDATA1_MCONTROL6

  // Store all seg8 intermediate data
  uint64_t vloadBuf[8];

  if (!fast_vle) {  // this block is the original slow path
    for (uint64_t idx = vstart->val; idx < vl_val; idx++, vstart->val++) {
      rtlreg_t mask = get_mask(0, idx);
      if (s->vm == 0 && mask == 0) {
        if (RVV_AGNOSTIC && vtype->vma) {
          tmp_reg[1] = (uint64_t) -1;
          for (fn = 0; fn < nf; fn++) {
            set_vreg(vd + fn * emul, idx, tmp_reg[1], eew, 0, 0);
            IFDEF(CONFIG_MULTICORE_DIFF, set_vec_dual_difftest_reg_idx(fn * emul, idx, tmp_reg[1], eew));
          }
        }
        continue;
      }
      for (fn = 0; fn < nf; fn++) {
        addr = base_addr + idx * stride + (idx * nf * is_unit_stride + fn) * s->v_width;

        IFDEF(CONFIG_TDATA1_MCONTROL6, trig_action_t action = check_triggers_mcontrol6(cpu.TM, TRIG_OP_LOAD, addr, TRIGGER_NO_VALUE); \
                                trigger_handler(TRIG_TYPE_MCONTROL6, action, addr));

        isa_vec_misalign_data_addr_check(addr, s->v_width, MEM_TYPE_READ);

        IFDEF(CONFIG_MULTICORE_DIFF, need_read_golden_mem = true);
        rtl_lm(s, &vloadBuf[fn], &addr, 0, s->v_width, mmu_mode);
        IFDEF(CONFIG_MULTICORE_DIFF, set_vec_load_difftest_info(fn, s->v_width));
      }
      // set vreg after all segment done with no exception
      for (fn = 0; fn < nf; fn++) {
        set_vreg(vd + fn * emul, idx, vloadBuf[fn], eew, 0, 0);
        IFDEF(CONFIG_MULTICORE_DIFF, set_vec_dual_difftest_reg_idx(fn * emul, idx, vec_load_diffteset_buf[fn], eew));
      }
    }
  }

  // Tail agnostic is not handled in fast path
  if (RVV_AGNOSTIC && (mode == MODE_MASK || vtype->vta)) {   // set tail of vector register to 1
    int vlmax =  mode == MODE_MASK ? VLEN / 8 : get_vlen_max(eew, vemul, 0);
    for(int idx = vl_val; idx < vlmax; idx++) {
      tmp_reg[1] = (uint64_t) -1;
      for (fn = 0; fn < nf; fn++) {
        set_vreg(vd + fn * emul, idx, tmp_reg[1], eew, 0, 0);
        IFDEF(CONFIG_MULTICORE_DIFF, set_vec_dual_difftest_reg_idx(fn * emul, idx, tmp_reg[1], eew));
      }
    }
  }

  vstart->val = 0;
  cpu.isVldst = false;
  vp_set_dirty();
}

void vldx(Decode *s, int mmu_mode) {
  //v_width 0  ->  8    SEW   0  ->  8
  //        5  ->  16         1  ->  16
  //        6  ->  32         2  ->  32
  //        7  ->  64         3  ->  64
  index_vload_check(s);
  if(check_vstart_ignore(s)) return;
  uint64_t nf = s->v_nf + 1, fn, vl_val, base_addr, vd, index, addr;
  int eew, lmul, index_width, data_width;

  index_width = 0;
  eew = vtype->vsew;
  switch(s->v_width) {
    case 1: index_width = 0; break;
    case 2: index_width = 1; break;
    case 4: index_width = 2; break;
    case 8: index_width = 3; break;
    default: break;
  }
  data_width = 1 << eew;
  lmul = vtype->vlmul > 4 ? vtype->vlmul - 8 : vtype->vlmul;
  isa_emul_check(lmul, nf);
  lmul = lmul < 0 ? 0 : lmul;
  lmul = 1 << lmul;

  // previous decode does not load vals for us
  rtl_lr(s, &(s->src1.val), s->src1.reg, 4);
  rtl_mv(s, &(tmp_reg[0]), &(s->src1.val));

  vl_val = vl->val;
  base_addr = tmp_reg[0];
  vd = id_dest->reg;

  IFDEF(CONFIG_MULTICORE_DIFF, init_vec_load_difftest_info(lmul, vd));

  // Store all seg8 intermediate data
  uint64_t vloadBuf[8];

  for (uint64_t idx = vstart->val; idx < vl_val; idx++, vstart->val++) {
    rtlreg_t mask = get_mask(0, idx);
    if (s->vm == 0 && mask == 0) {
      if (RVV_AGNOSTIC && vtype->vma) {
        tmp_reg[1] = (uint64_t) -1;
        for (fn = 0; fn < nf; fn++) {
          set_vreg(vd + fn * lmul, idx, tmp_reg[1], eew, 0, 0);
          IFDEF(CONFIG_MULTICORE_DIFF, set_vec_dual_difftest_reg_idx(fn * lmul, idx, tmp_reg[1], eew));
        }
      }
      continue;
    }
    for (fn = 0; fn < nf; fn++) {
      // read index
      get_vreg(id_src2->reg, idx, &tmp_reg[2], index_width, 0, 0, 0);
      index = tmp_reg[2];

      // read data in memory
      addr = base_addr + index + fn * data_width;

      IFDEF(CONFIG_TDATA1_MCONTROL6, trig_action_t action = check_triggers_mcontrol6(cpu.TM, TRIG_OP_LOAD, addr, TRIGGER_NO_VALUE); \
                              trigger_handler(TRIG_TYPE_MCONTROL6, action, addr));

      isa_vec_misalign_data_addr_check(addr, data_width, MEM_TYPE_READ);

      IFDEF(CONFIG_MULTICORE_DIFF, need_read_golden_mem = true);
      rtl_lm(s, &vloadBuf[fn], &addr, 0, data_width, mmu_mode);
      IFDEF(CONFIG_MULTICORE_DIFF, set_vec_load_difftest_info(fn, data_width));
    }

    // set vreg after all segment done with no exception
    for (fn = 0; fn < nf; fn++) {
      set_vreg(vd + fn * lmul, idx, vloadBuf[fn], eew, 0, 0);
      IFDEF(CONFIG_MULTICORE_DIFF, set_vec_dual_difftest_reg_idx(fn * lmul, idx, vec_load_diffteset_buf[fn], eew));
    }
  }

  if (RVV_AGNOSTIC && vtype->vta) {   // set tail of vector register to 1
    int vlmax = get_vlen_max(vtype->vsew, vtype->vlmul, 0);
    for(int idx = vl->val; idx < vlmax; idx++) {
      tmp_reg[1] = (uint64_t) -1;
      for (fn = 0; fn < nf; fn++) {
        set_vreg(vd + fn * lmul, idx, tmp_reg[1], eew, 0, 0);
        IFDEF(CONFIG_MULTICORE_DIFF, set_vec_dual_difftest_reg_idx(fn * lmul, idx, tmp_reg[1], eew));
      }
    }
  }

  // TODO: the idx larger than vl need reset to zero.
  vstart->val = 0;
  cpu.isVldst = false;
  vp_set_dirty();
}

extern uint64_t g_nr_vst, g_nr_vst_unit, g_nr_vst_unit_optimized;

void vst(Decode *s, int mode, int mmu_mode) {
  vstore_check(mode, s);
  if(check_vstart_ignore(s)) return;
  g_nr_vst += 1;
  uint64_t idx;
  uint64_t nf, vl_val, base_addr, vd, addr, is_unit_stride;
  int64_t stride;
  int eew, emul;

  eew = 0;
  switch(s->v_width) {
    case 1: eew = 0; break;
    case 2: eew = 1; break;
    case 4: eew = 2; break;
    case 8: eew = 3; break;
    default: break;
  }
  emul = vtype->vlmul > 4 ? vtype->vlmul - 8 + eew - vtype->vsew : vtype->vlmul + eew - vtype->vsew;
  isa_emul_check(mode == MODE_MASK ? 1 : emul, 1);
  emul = emul < 0 ? 0 : emul;
  emul = 1 << emul;

  if (mode == MODE_STRIDED) {
    stride = id_src2->val;
    is_unit_stride = 0;
  } else {
    stride = 0;
    is_unit_stride = 1;
    g_nr_vst_unit += 1;
  }
  // previous decode does not load vals for us
  rtl_lr(s, &(s->src1.val), s->src1.reg, 4);
  rtl_mv(s, &(tmp_reg[0]), &(s->src1.val));

  nf = s->v_nf + 1;
  vl_val = mode == MODE_MASK ? (vl->val + 7) / 8 : vl->val;
  base_addr = tmp_reg[0];
  vd = id_dest->reg;

  bool fast_vse = false;

#if !defined(CONFIG_SHARE) && !defined(CONFIG_TDATA1_MCONTROL6)
  uint64_t start_addr = base_addr + (vstart->val * nf) * s->v_width;
  uint64_t last_addr = base_addr + (vl_val * nf - 1) * s->v_width;
  uint64_t vse_size = last_addr - start_addr + s->v_width;
  __attribute__((unused)) bool cross_page = last_addr / PAGE_SIZE != start_addr / PAGE_SIZE;
  uint8_t masks[VLMAX_8] = {0};

  if (is_unit_stride && nf == 1 && vl_val > vstart->val && vtype->vlmul < 4 && !cross_page) {
    assert(vl_val <= VLEN);
    uint8_t invert_masks[VLMAX_8] = {0};

    // get address of first element in memory; manually trigger isa_mmu_check.
    // if exception happens, it will goto the exception handler.
    s->last_access_host_addr = NULL;
    extern void dummy_vaddr_write(struct Decode *s, vaddr_t addr, int len, int mmu_mode);
    dummy_vaddr_write(s, start_addr, s->v_width, mmu_mode);
    // Now we have the host address of first element in Decode *s->last_access_host_addr
    if (s->last_access_host_addr != NULL) {

      // get address of first element in register file
      void *reg_file_addr = NULL;
      get_vreg_with_addr(vd, vstart->val, &tmp_reg[1], eew, 0, 0, 0, &reg_file_addr);
      Assert(reg_file_addr != NULL, "reg_file_addr is NULL");
      uint8_t * restrict reg_file_addr_8 = reg_file_addr;

      unsigned count = gen_mask_for_unit_stride(s, eew, vstart, vl_val, masks);

      Logm("vst start_addr: %#lx, last_addr: %#lx, v_width: %u, vl_val: %lu, "
          "vstart->val: %lu, v0: %016lx_%016lx, nf=%lu",
          start_addr, last_addr, s->v_width, vl_val, vstart->val, cpu.vr[0]._64[1], cpu.vr[0]._64[0],
          nf);
      Logm("vse size = %lu, valid mask count = %u", vse_size, count);
      Logm("mem base host addr = %p, reg file base host addr = %p", s->last_access_host_addr, reg_file_addr);

      uint8_t * restrict last_access_addr_8 = s->last_access_host_addr;
      for (int i = 0; i < VLMAX_8; i++) {
        invert_masks[i] = ~masks[i];
        masks[i] &= reg_file_addr_8[i];
        invert_masks[i] &= last_access_addr_8[i];
        masks[i] |= invert_masks[i];
      }
#ifdef CONFIG_DIFFTEST_STORE_COMMIT
      extern void store_commit_queue_push(uint64_t addr, uint64_t data,
                                          int len);
      for (int i = 0; i < vse_size; i += s->v_width) {
        // store_commit_queue_push((uint8_t *)s->last_access_host_addr), data, len);
        switch (s->v_width) {
          case 1: store_commit_queue_push(
              host_to_guest((uint8_t *)s->last_access_host_addr + i),
              masks[i], s->v_width, 0); break;
          case 2: store_commit_queue_push(
              host_to_guest((uint8_t *)s->last_access_host_addr + i),
              *(uint16_t *)&masks[i], s->v_width, 0); break;
          case 4: store_commit_queue_push(
              host_to_guest((uint8_t *)s->last_access_host_addr + i),
              *(uint32_t *)&masks[i], s->v_width, 0); break;
          case 8: store_commit_queue_push(
              host_to_guest((uint8_t *)s->last_access_host_addr + i),
              *(uint64_t *)&masks[i], s->v_width, 0); break;
          default: panic("Unexpected vwidth = %d", s->v_width);
        }
      }
#endif
      memcpy(s->last_access_host_addr, masks, vse_size);
      fast_vse = true; // skip all operations
    }
  }

  g_nr_vst_unit_optimized += fast_vse;
#endif // !CONFIG_SHARE && !CONFIG_TDATA1_MCONTROL6

  // We enter this block if we are not able to optimize the store or we are debugging fast VSE
  if (!fast_vse || ISDEF(DEBUG_FAST_VSE)) {  // this block is the original slow path
    for (idx = vstart->val; idx < vl_val; idx++, vstart->val++) {
      rtlreg_t mask = get_mask(0, idx);
      if (s->vm == 0 && mask == 0) {
#ifdef DEBUG_FAST_VSE
        if (ISNDEF(CONFIG_SHARE) && ISDEF(CONFIG_DIFFTEST_STORE_COMMIT) && simple_vse) {
          uint64_t offset = idx * stride + (idx * nf * is_unit_stride + 0) * s->v_width;
          switch (s->v_width) {
            case 1: assert(masks[offset] == 0); break;
            case 2: assert(*(uint16_t *)&masks[offset] == 0); break;
            case 4: assert(*(uint32_t *)&masks[offset] == 0); break;
            case 8: assert(*(uint64_t *)&masks[offset] == 0); break;
            default: panic("Unexpected vwidth = %d", s->v_width);
          }
        }
#endif
        continue;
      }
      for (unsigned fn = 0; fn < nf; fn++) {
        get_vreg(vd + fn * emul, idx, &tmp_reg[1], eew, 0, 0, 0);
        uint64_t offset = idx * stride + (idx * nf * is_unit_stride + fn) * s->v_width;
        addr = base_addr + offset;
        if (!fast_vse) {
          IFDEF(CONFIG_TDATA1_MCONTROL6, trig_action_t action = check_triggers_mcontrol6(cpu.TM, TRIG_OP_STORE, addr, TRIGGER_NO_VALUE); \
                                  trigger_handler(TRIG_TYPE_MCONTROL6, action, addr));

          isa_vec_misalign_data_addr_check(addr, s->v_width, MEM_TYPE_WRITE);

          rtl_sm(s, &tmp_reg[1], &addr, 0, s->v_width, mmu_mode);
        }
#ifdef DEBUG_FAST_VSE
        if (simple_vse) {
          bool match = memcmp(&tmp_reg[1], &masks[offset], s->v_width) == 0;
          if (!match) {
            Logm("Mismatch at idx = %lu, fn = %u, offset = %lu, addr = %lx, "
                "slow version = %lx, fast version = %lx", idx, fn, offset, addr,
                tmp_reg[1], *(uint64_t *)&masks[offset]);
            Assert(false, "Fast and slow version mismatch");
          }
        }
#endif
      }
    }
  }

  vstart->val = 0;
  cpu.isVldst = false;
  vp_set_dirty();
}

void vstx(Decode *s, int mmu_mode) {
  index_vstore_check(s);
  if(check_vstart_ignore(s)) return;
  uint64_t idx;
  uint64_t nf = s->v_nf + 1, fn, vl_val, base_addr, vd, index, addr;
  int eew, lmul, index_width, data_width;

  index_width = 0;
  eew = vtype->vsew;
  switch(s->v_width) {
    case 1: index_width = 0; break;
    case 2: index_width = 1; break;
    case 4: index_width = 2; break;
    case 8: index_width = 3; break;
    default: break;
  }
  data_width = 1 << eew;
  lmul = vtype->vlmul > 4 ? vtype->vlmul - 8 : vtype->vlmul;
  isa_emul_check(lmul, nf);
  lmul = lmul < 0 ? 0 : lmul;
  lmul = 1 << lmul;

  // previous decode does not load vals for us
  rtl_lr(s, &(s->src1.val), s->src1.reg, 4);
  rtl_mv(s, &(tmp_reg[0]), &(s->src1.val));

  vl_val = vl->val;
  base_addr = tmp_reg[0];
  vd = id_dest->reg;
  for (idx = vstart->val; idx < vl_val; idx++, vstart->val++) {
    rtlreg_t mask = get_mask(0, idx);
    if (s->vm == 0 && mask == 0) {
      continue;
    }
    for (fn = 0; fn < nf; fn++) {
      // read index
      get_vreg(id_src2->reg, idx, &tmp_reg[2], index_width, 0, 0, 0);
      index = tmp_reg[2];

      // read data in vector register
      get_vreg(vd + fn * lmul, idx, &tmp_reg[1], eew, 0, 0, 0);
      addr = base_addr + index + fn * data_width;

      IFDEF(CONFIG_TDATA1_MCONTROL6, trig_action_t action = check_triggers_mcontrol6(cpu.TM, TRIG_OP_STORE, addr, TRIGGER_NO_VALUE); \
                              trigger_handler(TRIG_TYPE_MCONTROL6, action, addr));

      isa_vec_misalign_data_addr_check(addr, data_width, MEM_TYPE_WRITE);

      rtl_sm(s, &tmp_reg[1], &addr, 0, data_width, mmu_mode);
    }
  }

  // TODO: the idx larger than vl need reset to zero.
  vstart->val = 0;
  cpu.isVldst = false;
  vp_set_dirty();
}

static void isa_whole_reg_check(uint64_t vd, uint64_t nfields) {
  if (nfields != 1 && nfields != 2 && nfields != 4 && nfields != 8) {
    Loge("illegal NFIELDS for whole register instrs: NFIELDS:%lu", nfields);
    longjmp_exception(EX_II);
  }
  if (vd % nfields) {
    Loge("vector register group misaligned for whole register instrs: NFIELDS:%lu vd:%lu",
      nfields, vd);
    longjmp_exception(EX_II);
  }
}

void vlr(Decode *s, int mmu_mode) {
  uint64_t idx, vreg_idx, offset, pos;
  uint64_t len, base_addr, vd, addr, elt_per_reg, size;
  int eew;

  eew = 0;
  switch(s->v_width) {
    case 1: eew = 0; break;
    case 2: eew = 1; break;
    case 4: eew = 2; break;
    case 8: eew = 3; break;
    default: break;
  }

  // previous decode does not load vals for us
  rtl_lr(s, &(s->src1.val), s->src1.reg, 4);
  rtl_mv(s, &(tmp_reg[0]), &(s->src1.val));

  len = s->v_nf + 1;
  elt_per_reg = VLEN / (8*s->v_width);
  size = len * elt_per_reg;
  base_addr = tmp_reg[0];
  vd = id_dest->reg;
  idx = vstart->val;

  IFDEF(CONFIG_MULTICORE_DIFF, init_vec_load_difftest_info(len, vd));
  isa_whole_reg_check(vd, len);

  if (vstart->val < size) {
    vreg_idx = vstart->val / elt_per_reg;
    offset = vstart->val % elt_per_reg;
    if (offset) {
      // first vreg
      for (pos = offset; pos < elt_per_reg; pos++, vstart->val++) {
        addr = base_addr + idx * s->v_width;

        IFDEF(CONFIG_TDATA1_MCONTROL6, trig_action_t action = check_triggers_mcontrol6(cpu.TM, TRIG_OP_LOAD, addr, TRIGGER_NO_VALUE); \
                                trigger_handler(TRIG_TYPE_MCONTROL6, action, addr));

        isa_vec_misalign_data_addr_check(addr, s->v_width, MEM_TYPE_READ);

        IFDEF(CONFIG_MULTICORE_DIFF, need_read_golden_mem = true);

        rtl_lm(s, &tmp_reg[1], &addr, 0, s->v_width, mmu_mode);
        IFDEF(CONFIG_MULTICORE_DIFF, set_vec_load_difftest_info(0, s->v_width));
        set_vreg(vd + vreg_idx, pos, tmp_reg[1], eew, 0, 1);
        IFDEF(CONFIG_MULTICORE_DIFF, set_vec_dual_difftest_reg_idx(vreg_idx, pos, vec_load_diffteset_buf[0], eew));
        idx++;
      }
      vreg_idx++;
    }
    for (; vreg_idx < len; vreg_idx++) {
      for (pos = 0; pos < elt_per_reg; pos++, vstart->val++) {
        addr = base_addr + idx * s->v_width;

        IFDEF(CONFIG_TDATA1_MCONTROL6, trig_action_t action = check_triggers_mcontrol6(cpu.TM, TRIG_OP_LOAD, addr, TRIGGER_NO_VALUE); \
                                trigger_handler(TRIG_TYPE_MCONTROL6, action, addr));

        isa_vec_misalign_data_addr_check(addr, s->v_width, MEM_TYPE_READ);

        IFDEF(CONFIG_MULTICORE_DIFF, need_read_golden_mem = true);

        rtl_lm(s, &tmp_reg[1], &addr, 0, s->v_width, mmu_mode);
        IFDEF(CONFIG_MULTICORE_DIFF, set_vec_load_difftest_info(0, s->v_width));
        set_vreg(vd + vreg_idx, pos, tmp_reg[1], eew, 0, 1);
        IFDEF(CONFIG_MULTICORE_DIFF, set_vec_dual_difftest_reg_idx(vreg_idx, pos, vec_load_diffteset_buf[0], eew));
        idx++;
      }
    }
  }

  vstart->val = 0;
  cpu.isVldst = false;
  vp_set_dirty();
}

void vsr(Decode *s, int mmu_mode) {
  uint64_t idx, vreg_idx, offset, pos;
  uint64_t len, base_addr, vd, addr, elt_per_reg, size;

  // previous decode does not load vals for us
  rtl_lr(s, &(s->src1.val), s->src1.reg, 4);
  rtl_mv(s, &(tmp_reg[0]), &(s->src1.val));

  len = s->v_nf + 1;
  elt_per_reg = vlenb->val;
  size = len * elt_per_reg;
  base_addr = tmp_reg[0];
  vd = id_dest->reg;
  idx = vstart->val;

  isa_whole_reg_check(vd, len);

  if (vstart->val < size) {
    vreg_idx = vstart->val / elt_per_reg;
    offset = vstart->val % elt_per_reg;
    if (offset) {
      // first vreg
      for (pos = offset; pos < elt_per_reg; pos++, vstart->val++) {
        // read 1 byte and store 1 byte to memory
        get_vreg(vd + vreg_idx, pos, &tmp_reg[1], 0, 0, 0, 1);
        addr = base_addr + idx;

        IFDEF(CONFIG_TDATA1_MCONTROL6, trig_action_t action = check_triggers_mcontrol6(cpu.TM, TRIG_OP_STORE, addr, TRIGGER_NO_VALUE); \
                                trigger_handler(TRIG_TYPE_MCONTROL6, action, addr));

        isa_vec_misalign_data_addr_check(addr, 1, MEM_TYPE_WRITE);

        rtl_sm(s, &tmp_reg[1], &addr, 0, 1, mmu_mode);
        idx++;
      }
      vreg_idx++;
    }
    for (; vreg_idx < len; vreg_idx++) {
      for (pos = 0; pos < elt_per_reg; pos++, vstart->val++) {
        // read 1 byte and store 1 byte to memory
        get_vreg(vd + vreg_idx, pos, &tmp_reg[1], 0, 0, 0, 1);
        addr = base_addr + idx;

        IFDEF(CONFIG_TDATA1_MCONTROL6, trig_action_t action = check_triggers_mcontrol6(cpu.TM, TRIG_OP_STORE, addr, TRIGGER_NO_VALUE); \
                                trigger_handler(TRIG_TYPE_MCONTROL6, action, addr));

        isa_vec_misalign_data_addr_check(addr, 1, MEM_TYPE_WRITE);

        rtl_sm(s, &tmp_reg[1], &addr, 0, 1, mmu_mode);
        idx++;
      }
    }
  }

  vstart->val = 0;
  cpu.isVldst = false;
  vp_set_dirty();
}


void vldff(Decode *s, int mode, int mmu_mode) {
  fofvl = 0;
  vload_check(mode, s);
  if(check_vstart_ignore(s)) return;
  uint64_t nf, fn, vl_val, base_addr, vd, addr, is_unit_stride;
  int64_t stride;
  int eew, emul, vemul;

  // s->v_width is the bytes of a unit
  // eew is the coding like vsew
  eew = 0;
  switch(s->v_width) {
    case 1: eew = 0; break;
    case 2: eew = 1; break;
    case 4: eew = 2; break;
    case 8: eew = 3; break;
    default: break;
  }
  emul = vtype->vlmul > 4 ? vtype->vlmul - 8 + eew - vtype->vsew : vtype->vlmul + eew - vtype->vsew;
  isa_emul_check(mode == MODE_MASK ? 1 : emul, 1);
  emul = emul < 0 ? 0 : emul;
  vemul = emul;
  emul = 1 << emul;

  if (mode == MODE_STRIDED) {
    stride = id_src2->val;
    is_unit_stride = 0;
  } else {
    stride = 0;
    is_unit_stride = 1;
  }
  // previous decode does not load vals for us
  rtl_lr(s, &(s->src1.val), s->src1.reg, 4);
  rtl_mv(s, &(tmp_reg[0]), &(s->src1.val));

  nf = s->v_nf + 1;
  vl_val = mode == MODE_MASK ? (vl->val + 7) / 8 : vl->val;
  base_addr = tmp_reg[0];
  vd = id_dest->reg;

  IFDEF(CONFIG_MULTICORE_DIFF, init_vec_load_difftest_info(emul, vd));

  bool fast_vle = false;

  int cause;
  PUSH_CONTEXT(&cause);
  if (cause) {
    if (fofvl) {
      vl->val = fofvl;
      #ifdef CONFIG_RVH
      vstval->val = vstvaltmp;
      #endif // CONFIG_RVH
      stval->val = stvaltmp;
      mtval->val = mtvaltmp;
      cpu.isVldst = false;
    } else {
      pop_context();
      longjmp_exception(cause);
    }
  } else {
  #if !defined(CONFIG_SHARE) && !defined(CONFIG_TDATA1_MCONTROL6)
    uint64_t start_addr = base_addr + (vstart->val * nf) * s->v_width;
    uint64_t last_addr = base_addr + (vl_val * nf - 1) * s->v_width;
    uint64_t vle_size = last_addr - start_addr + s->v_width;
    __attribute__((unused)) bool cross_page = last_addr / PAGE_SIZE != start_addr / PAGE_SIZE;
    uint8_t masks[VLMAX_8] = {0};

    Logm("vld start_addr: %#lx, v_width: %u, vl_val: %lu, vle size=%lu, vstart->val: %lu, nf=%lu",
        base_addr, s->v_width, vl_val, vle_size, vstart->val, nf);

    if (is_unit_stride && nf == 1 && vl_val > vstart->val && vtype->vlmul < 4 && !cross_page) {
      s->last_access_host_addr = NULL;
      extern void dummy_vaddr_data_read(struct Decode *s, vaddr_t addr, int len, int mmu_mode);
      dummy_vaddr_data_read(s, start_addr, s->v_width, mmu_mode);
      // Now we have the host address of first element in Decode *s->last_access_host_addr
      if (s->last_access_host_addr != NULL) {

        // get address of first element in register file
        void *reg_file_addr = NULL;
        get_vreg_with_addr(vd, vstart->val, &tmp_reg[1], eew, 0, 0, 0, &reg_file_addr);
        Assert(reg_file_addr != NULL, "reg_file_addr is NULL");
        uint8_t * restrict reg_file_addr_8 = reg_file_addr;

        __attribute__((unused)) unsigned count = gen_mask_for_unit_stride(s, eew, vstart, vl_val, masks);

        uint8_t invert_masks[VLMAX_8] = {0};
        uint8_t * restrict last_access_host_addr_u8 = s->last_access_host_addr;

  #ifdef DEBUG_FAST_VLE
        switch (s->v_width) {
          case 1: for (int i = 0; i < vle_size; i++) {
              Logm("Element %i, mask = %x, inv mask = %x, reg = %x, mem = %x", i,
                   masks[i], invert_masks[i], reg_file_addr_8[i],
                   last_access_host_addr[i]);
            }
            break;
          case 2:
            for (int i = 0; i < vle_size; i += 2) {
              Logm("Element %i, mask = %x, inv mask = %x, reg = %x, mem = %x", i,
                   *(uint16_t *)&masks[i], *(uint16_t *)&invert_masks[i],
                   *(uint16_t *)&reg_file_addr_8[i],
                   *(uint16_t *)&last_access_host_addr[i]);
            }
            break;
          case 4:
            for (int i = 0; i < vle_size; i += 4) {
              Logm("Element %i, mask = %x, inv mask = %x, reg = %x, mem = %x", i,
                   *(uint32_t *)&masks[i], *(uint32_t *)&invert_masks[i],
                   *(uint32_t *)&reg_file_addr_8[i],
                   *(uint32_t *)&last_access_host_addr[i]);
            }
            break;
          case 8:
            for (int i = 0; i < vle_size; i += 8) {
              Logm("Element %i, mask = %lx, inv mask = %lx, reg = %lx, mem = %lx",
                   i, *(uint64_t *)&masks[i], *(uint64_t *)&invert_masks[i],
                   *(uint64_t *)&reg_file_addr_8[i],
                   *(uint64_t *)&last_access_host_addr[i]);
            }
            break;
          default:
                  panic("Unexpected vwidth = %d", s->v_width);
        }
  # endif // DEBUG_FAST_VLE

        for (int i = 0; i < VLMAX_8; i++) {
          invert_masks[i] = ~masks[i];
          masks[i] &= last_access_host_addr_u8[i];
          if (RVV_AGNOSTIC && vtype->vma) {
            invert_masks[i] = 0xff;
          } else {
            invert_masks[i] &= reg_file_addr_8[i];
          }
          masks[i] |= invert_masks[i];
        }
        memcpy(reg_file_addr, masks, vle_size);
        fast_vle = true;
      }
    }
  #endif // !CONFIG_SHARE && !CONFIG_TDATA1_MCONTROL6

    // Store all seg8 intermediate data
    uint64_t vloadBuf[8];

    if (!fast_vle) {  // this block is the original slow path
      for (uint64_t idx = vstart->val; idx < vl_val; idx++) {
        fofvl = idx;
        #ifdef CONFIG_RVH
        vstvaltmp = vstval->val;
        #endif // CONFIG_RVH
        stvaltmp  = stval->val;
        mtvaltmp  = mtval->val;

        rtlreg_t mask = get_mask(0, idx);
        if (s->vm == 0 && mask == 0) {
          if (RVV_AGNOSTIC && vtype->vma) {
            tmp_reg[1] = (uint64_t) -1;
            for (fn = 0; fn < nf; fn++) {
              set_vreg(vd + fn * emul, idx, tmp_reg[1], eew, 0, 0);
              IFDEF(CONFIG_MULTICORE_DIFF, set_vec_dual_difftest_reg_idx(fn * emul, idx, tmp_reg[1], eew));
            }
          }
          continue;
        }
        for (fn = 0; fn < nf; fn++) {
          addr = base_addr + idx * stride + (idx * nf * is_unit_stride + fn) * s->v_width;

          IFDEF(CONFIG_TDATA1_MCONTROL6, trig_action_t action = check_triggers_mcontrol6(cpu.TM, TRIG_OP_LOAD, addr, TRIGGER_NO_VALUE); \
                                  trigger_handler(TRIG_TYPE_MCONTROL6, action, addr));
          isa_vec_misalign_data_addr_check(addr, s->v_width, MEM_TYPE_READ);

          IFDEF(CONFIG_MULTICORE_DIFF, need_read_golden_mem = true);
          if (fofvl == 0) {
            rtl_lm(s, &vloadBuf[fn], &addr, 0, s->v_width, mmu_mode);
            IFDEF(CONFIG_MULTICORE_DIFF, set_vec_load_difftest_info(fn, s->v_width));
          } else {
            rtl_lm(s, &tmp_reg[1], &addr, 0, s->v_width, mmu_mode);
            IFDEF(CONFIG_MULTICORE_DIFF, set_vec_load_difftest_info(fn, s->v_width));
            set_vreg(vd + fn * emul, idx, tmp_reg[1], eew, 0, 0);
            IFDEF(CONFIG_MULTICORE_DIFF, set_vec_dual_difftest_reg_idx(fn * emul, idx, vec_load_diffteset_buf[fn], eew));
          }

        }
        if (fofvl == 0) {
          for (fn = 0; fn < nf; fn++) {
            set_vreg(vd + fn * emul, idx, vloadBuf[fn], eew, 0, 0);
            IFDEF(CONFIG_MULTICORE_DIFF, set_vec_dual_difftest_reg_idx(fn * emul, idx, vec_load_diffteset_buf[fn], eew));
          }
        }

      }
    }

    // Tail agnostic is not handled in fast path
    if (RVV_AGNOSTIC && (mode == MODE_MASK || vtype->vta)) {   // set tail of vector register to 1
      int vlmax =  mode == MODE_MASK ? VLEN / 8 : get_vlen_max(eew, vemul, 0);
      for(int idx = vl_val; idx < vlmax; idx++) {
        tmp_reg[1] = (uint64_t) -1;
        for (fn = 0; fn < nf; fn++) {
          set_vreg(vd + fn * emul, idx, tmp_reg[1], eew, 0, 0);
          IFDEF(CONFIG_MULTICORE_DIFF, set_vec_dual_difftest_reg_idx(fn * emul, idx, tmp_reg[1], eew));
        }
      }
    }

  }
  pop_context();

  vstart->val = 0;
  fofvl = 0;
  cpu.isVldst = false;
  vp_set_dirty();
}

#endif // CONFIG_RVV
