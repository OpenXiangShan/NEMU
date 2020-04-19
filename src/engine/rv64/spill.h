#ifndef __SPILL_H__
#define __SPILL_H__

#include <common.h>
#include <cpu/decode.h>

// Temp GPR Registers Setting (can set any registers but $0, $1, $28)
// default is $26, $27, which has low perf loss
#define TMP_REG_1     26
#define TMP_REG_2     27

#define TMP_REG_NUM 2
#define TMPIDX_CNT 9
#define TMP_REG_MAX 233

void tmp_regs_init();
void tmp_regs_reset();
uint32_t spmidx2rvidx(uint32_t);
uint32_t spill_out_and_remap(DecodeExecState*, uint32_t);
void spill_flush(uint32_t);
void spill_flush_all();
void cal_suffix_inst();
void spill_writeback_all();
void spill_set_spmidx(uint32_t tmpidx, uint32_t new_spmidx);
int rvidx2tmpidx(uint32_t rvidx);
void spill_set_dirty_rvidx(uint32_t rvidx);
uint32_t varidx2rvidx(uint32_t varidx);
void spill_set_dirty(uint32_t tmpidx);
void spill_writeback(uint32_t i);
uint32_t rtlreg2rvidx_pair(DecodeExecState *s, const rtlreg_t *src1, const rtlreg_t *src2);

#endif
