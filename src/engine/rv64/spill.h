#ifndef __SPILL_H__
#define __SPILL_H__

#include <common.h>
#include <cpu/decode.h>

#define TMP_REG_NUM 2
#define SCRATCHPAD_BASE_ADDR 0x80000000

typedef struct {
    uint32_t idx;
    uint32_t map_ptr;
    bool dirty;
} Tmp_reg;

extern Tmp_reg tmp_regs[TMP_REG_NUM];
extern Tmp_reg spill_tmp_reg;
extern uint32_t tmp_regs_ptr;

void tmp_regs_init();
uint32_t check_tmp_reg(uint32_t);
uint32_t spill_out_and_remap(DecodeExecState*, uint32_t);

#endif