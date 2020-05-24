#include "spill.h"
#include "tran.h"
#include "rv64-backend/rv_ins_def.h"
#include "rtl/rtl.h"

#define TMP_REG_NUM 2

typedef struct {
  uint32_t rvidx;
  uint32_t spmidx;
  bool dirty;
} Tmp_reg;
static Tmp_reg tmp_regs[TMP_REG_NUM];

void spill_init() {
  assert(TMP_REG_NUM == 2);
  tmp_regs[0].rvidx = tmp_reg1;
  tmp_regs[1].rvidx = tmp_reg2;
}

void spill_flush(int tmpidx) {
  tmp_regs[tmpidx].spmidx = 0;
  tmp_regs[tmpidx].dirty = 0;
}

void spill_reset() {
  spill_flush(0);
  spill_flush(1);
}

int spmidx2tmpidx(uint32_t spmidx) {
  if (tmp_regs[0].spmidx == spmidx) return 0;
  if (tmp_regs[1].spmidx == spmidx) return 1;
  return -1;
}

int rvidx2tmpidx(uint32_t rvidx) {
  if (tmp_regs[0].rvidx == rvidx) return 0;
  if (tmp_regs[1].rvidx == rvidx) return 1;
  return -1;
}

uint32_t spmidx2rvidx(uint32_t spmidx) {
  int tmpidx = spmidx2tmpidx(spmidx);
  if (tmpidx == -1) return 0;
  return tmp_regs[tmpidx].rvidx;
}

uint32_t varidx2rvidx(uint32_t varidx) {
  if (varidx & ~SPMIDX_MASK) return varidx;
  int tmpidx = spmidx2tmpidx(varidx);
  assert(tmpidx != -1);
  return tmp_regs[tmpidx].rvidx;
}

void spill_writeback(uint32_t i) {
  if (tmp_regs[i].spmidx != 0 && tmp_regs[i].dirty) {
    spm(sw, tmp_regs[i].rvidx, 4 * (tmp_regs[i].spmidx & ~SPMIDX_MASK));
    tmp_regs[i].dirty = false;
  }
}

void spill_writeback_all() {
  spill_writeback(0);
  spill_writeback(1);
}

// replace tmp_regs[tmpidx] with spmidx
void spill_replace(uint32_t tmpidx, uint32_t spmidx, int load_val) {
  spill_writeback(tmpidx);
  if (load_val) spm(lw, tmp_regs[tmpidx].rvidx, 4 * (spmidx & ~SPMIDX_MASK));

  tmp_regs[tmpidx].spmidx = spmidx;
  tmp_regs[tmpidx].dirty = false;
}

// find a clean tmpreg and map it to spmidx
uint32_t spill_alloc(uint32_t spmidx, int load_val) {
  uint32_t tmpidx = (tmp_regs[1].dirty ? 0 : 1);
  spill_replace(tmpidx, spmidx, load_val);
  return tmpidx;
}

uint32_t rtlreg2varidx(DecodeExecState *s, const rtlreg_t* dest);

static uint32_t rtlreg2rvidx_internal(DecodeExecState *s, const rtlreg_t *r, int is_dest) {
  uint32_t varidx = rtlreg2varidx(s, r);
  if (varidx & SPMIDX_MASK) {
    uint32_t tmpidx = spmidx2tmpidx(varidx);
    if (tmpidx == -1) tmpidx = spill_alloc(varidx, !is_dest);
    varidx = tmp_regs[tmpidx].rvidx;
    tmp_regs[tmpidx].dirty = is_dest;
  }
  return varidx;
}

uint32_t src2rvidx(DecodeExecState *s, const rtlreg_t *src) {
  return rtlreg2rvidx_internal(s, src, false);
}

uint32_t dest2rvidx(DecodeExecState *s, const rtlreg_t *dest) {
  return rtlreg2rvidx_internal(s, dest, true);
}

uint32_t rtlreg2rvidx_pair(DecodeExecState *s,
    const rtlreg_t *src1, int load_src1, const rtlreg_t *src2, int load_src2) {
  uint32_t src1_varidx = rtlreg2varidx(s, src1);
  uint32_t src2_varidx = rtlreg2varidx(s, src2);

  if ((src1_varidx & SPMIDX_MASK) && (src2_varidx & SPMIDX_MASK)) {
    // check whether they are already mapped
    uint32_t src1_tmpidx = spmidx2tmpidx(src1_varidx);
    uint32_t src2_tmpidx = spmidx2tmpidx(src2_varidx);

    if (src1_tmpidx == -1 && src2_tmpidx != -1) {
      src1_tmpidx = !src2_tmpidx;
      spill_replace(src1_tmpidx, src1_varidx, load_src1);
    } else if (src1_tmpidx != -1 && src2_tmpidx == -1) {
      src2_tmpidx = !src1_tmpidx;
      spill_replace(src2_tmpidx, src2_varidx, load_src2);
    } else if (src1_tmpidx == -1 && src2_tmpidx == -1) {
      src1_tmpidx = 0;
      src2_tmpidx = 1;
      spill_replace(src1_tmpidx, src1_varidx, load_src1);
      spill_replace(src2_tmpidx, src2_varidx, load_src2);
    }

    src1_varidx = tmp_regs[src1_tmpidx].rvidx;
    src2_varidx = tmp_regs[src2_tmpidx].rvidx;
  } else if (src1_varidx & SPMIDX_MASK) {
    uint32_t src1_tmpidx = spmidx2tmpidx(src1_varidx);
    if (src1_tmpidx == -1) src1_tmpidx = spill_alloc(src1_varidx, load_src1);
    src1_varidx = tmp_regs[src1_tmpidx].rvidx;
  } else if (src2_varidx & SPMIDX_MASK) {
    uint32_t src2_tmpidx = spmidx2tmpidx(src2_varidx);
    if (src2_tmpidx == -1) src2_tmpidx = spill_alloc(src2_varidx, load_src2);
    src2_varidx = tmp_regs[src2_tmpidx].rvidx;
  }

  return (src1_varidx << 16) | src2_varidx;
}

void spill_set_dirty(uint32_t tmpidx) {
  tmp_regs[tmpidx].dirty = true;
}

void spill_set_dirty_rvidx(uint32_t rvidx) {
  int tmpidx = rvidx2tmpidx(rvidx);
  if (tmpidx != -1) spill_set_dirty(tmpidx);
}

// this will be called after every translation of an instruction
// to flush RTL tmp registers, since their life-cycle is only
// valid during the translation of a single instruction
static void spill_flush_local_internal(DecodeExecState *s, const rtlreg_t *dest) {
  uint32_t varidx = rtlreg2varidx(s, dest);
  if (tmp_regs[0].spmidx == varidx && tmp_regs[0].dirty) spill_flush(0);
  if (tmp_regs[1].spmidx == varidx && tmp_regs[1].dirty) spill_flush(1);
}
void spill_flush_local() {
  DecodeExecState state; // only used in rtlreg2varidx()
  DecodeExecState *s = &state;
  spill_flush_local_internal(s, s0);
  spill_flush_local_internal(s, s1);
}

void load_spill_reg(const rtlreg_t* dest) {
  uint32_t spmidx = rtlreg2varidx(NULL, dest);
  assert(spmidx & SPMIDX_MASK);
  uint32_t rvidx = spmidx & ~SPMIDX_MASK;
  spm(lw, rvidx, 4 * (spmidx & ~SPMIDX_MASK));
}
