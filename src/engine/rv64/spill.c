#include "spill.h"
#include "tran.h"
#include "rv64-backend/rv_ins_def.h"
#include "rtl/rtl.h"

#include <string.h>

static Tmp_reg tmp_regs[TMP_REG_NUM];
uint32_t suffix_inst = 0;

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

    spm(sw, tmp_regs[ptr].idx, 4 * tmp_regs[ptr].map_ptr);
    spm(lw, tmp_regs[ptr].idx, 4 * tmp_idx);

    tmp_regs[ptr].map_ptr = tmp_idx;
    tmp_regs[ptr].used = 1;

    return tmp_regs[ptr].idx;
}

void cal_suffix_inst() {
    suffix_inst = 0;
    for (int i = 0; i < TMP_REG_NUM; i++) {
        if (tmp_regs[i].map_ptr != 0) {
            suffix_inst += 1;
        }
    }
}

void spill_writeback(uint32_t i) {
  if (tmp_regs[i].map_ptr != 0) {
    spm(sw, tmp_regs[i].idx, 4 * tmp_regs[i].map_ptr);
  }
}

void spill_writeback_all() {  // can be 0/1/2 inst
  for (int i = 0; i < TMP_REG_NUM; i++) {
    spill_writeback(i);
  }
}

void spill_flush(uint32_t tmp_idx) {
    for (int i = 0; i < TMP_REG_NUM; i++) {
        if (tmp_regs[i].map_ptr == tmp_idx) {
            tmp_regs[i].used = 0;
            return;
        }
    }
}

void spill_flush_all() {
    for (int i = 0; i < TMP_REG_NUM; i++) {
      tmp_regs[i].used = 0;
    }
}
