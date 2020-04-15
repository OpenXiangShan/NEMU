#include "spill.h"
#include "tran.h"
#include "rv64-backend/rv_ins_def.h"
#include "rtl/rtl.h"

#include <string.h>

Tmp_reg tmp_regs[TMP_REG_NUM];
Tmp_reg spill_tmp_reg;
uint32_t suffix_inst = 0;

static inline void load_imm_no_opt(uint32_t r, const sword_t imm) {
  RV_IMM rv_imm = { .val = imm };
  uint32_t lui_imm = rv_imm.imm_31_12 + (rv_imm.imm_11_0 >> 11);
  rv64_lui(r, lui_imm);
  rv64_addiw(r, r, rv_imm.imm_11_0);
}

void tmp_regs_init() {
    suffix_inst = 0;
    if (TMP_REG_NUM == 2) {
        tmp_regs[0].idx = TMP_REG_1;
        tmp_regs[0].map_ptr = 0;
        tmp_regs[0].used = 0;
        tmp_regs[1].idx = TMP_REG_2;
        tmp_regs[1].map_ptr = 0;
        tmp_regs[1].used = 0;
    } else {
        panic("Other TMP_REG_NUM!\n");
    }
    spill_tmp_reg.idx = TMP_REG_ADDR;
    spill_tmp_reg.map_ptr = 2;
}

void tmp_regs_reset() {
    if (TMP_REG_NUM == 2) {
        tmp_regs[0].map_ptr = 0;
        tmp_regs[0].used = 0;
        tmp_regs[1].map_ptr = 0;
        tmp_regs[1].used = 0;
    } else {
        panic("Other TMP_REG_NUM!\n");
    }
}

uint32_t check_tmp_reg(uint32_t tmp_idx) {
    for (int i = 0; i < TMP_REG_NUM; i++) {
        if (tmp_regs[i].map_ptr == tmp_idx) {
            tmp_regs[i].used = 1;
            return tmp_regs[i].idx;
        }
    }
    return 0;
}

uint32_t spill_out_and_remap(DecodeExecState *s, uint32_t tmp_idx) {
    uint32_t addr;

    uint32_t ptr = TMP_REG_MAX;
    for (int i = 0; i < TMP_REG_NUM; i++) {
        if (tmp_regs[i].used == 0) {
            ptr = i;
            break;
        }
    }
    if (ptr == TMP_REG_MAX) {
        panic("no clean tmp_regs!\nalready used:%u %u, req: %u\n", tmp_regs[0].map_ptr, tmp_regs[1].map_ptr, tmp_idx);
    }

    addr = SCRATCHPAD_BASE_ADDR + 4 * (tmp_regs[ptr].map_ptr);
    load_imm_no_opt(spill_tmp_reg.idx, addr);
    rv64_sw(tmp_regs[ptr].idx, spill_tmp_reg.idx, 0);

    addr = SCRATCHPAD_BASE_ADDR + 4 * (tmp_idx);
    load_imm_no_opt(spill_tmp_reg.idx, addr);
    rv64_lw(tmp_regs[ptr].idx, spill_tmp_reg.idx, 0);

    tmp_regs[ptr].map_ptr = tmp_idx;
    tmp_regs[ptr].used = 1;

    return tmp_regs[ptr].idx;
}

void cal_suffix_inst() {
    suffix_inst = 0;
    for (int i = 0; i < TMP_REG_NUM; i++) {
        if (tmp_regs[i].map_ptr != 0) {
            suffix_inst += 3;
        }
    }
}

void spill_out_all() {  // can be 0/3/6 inst
    uint32_t addr;
    for (int i = 0; i < TMP_REG_NUM; i++) {
        if (tmp_regs[i].map_ptr != 0) {
            addr = SCRATCHPAD_BASE_ADDR + 4 * (tmp_regs[i].map_ptr);
            load_imm_no_opt(spill_tmp_reg.idx, addr);
            rv64_sw(tmp_regs[i].idx, spill_tmp_reg.idx, 0);
        }
    }
}

void spill_clean(uint32_t tmp_idx) {
    for (int i = 0; i < TMP_REG_NUM; i++) {
        if (tmp_regs[i].map_ptr == tmp_idx) {
            tmp_regs[i].used = 0;
            return;
        }
    }
}

void spill_cleanall() {
    for (int i = 0; i < TMP_REG_NUM; i++) {
      tmp_regs[i].used = 0;
    }
}