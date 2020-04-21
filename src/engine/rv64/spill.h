#ifndef __SPILL_H__
#define __SPILL_H__

#include <common.h>
#include <cpu/decode.h>

uint32_t spmidx2rvidx(uint32_t);
uint32_t spill_out_and_remap(DecodeExecState*, uint32_t);
void spill_flush_all();
void cal_suffix_inst();
void spill_writeback_all();
void spill_set_spmidx(uint32_t tmpidx, uint32_t new_spmidx);
int rvidx2tmpidx(uint32_t rvidx);
void spill_set_dirty_rvidx(uint32_t rvidx);
uint32_t varidx2rvidx(uint32_t varidx);
void spill_set_dirty(uint32_t tmpidx);
void spill_writeback(uint32_t i);
uint32_t rtlreg2rvidx_pair(DecodeExecState *s, const rtlreg_t *src1, int load_src1, const rtlreg_t *src2, int load_src2);

#endif
