#ifdef __ISA_riscv64__

#include <cpu/decode.h>
#include <rtl/rtl.h>
#include <isa.h>
#include <isa/riscv64.h>
#include "../tran.h"

uint32_t rtlreg2varidx(DecodeExecState *s, const rtlreg_t* dest) {
  rtlreg_t* gpr_start = (rtlreg_t *)cpu.gpr;
  rtlreg_t* gpr_end = (void *)gpr_start + sizeof(cpu.gpr);

  if (dest >= gpr_start && dest < gpr_end) {
    int rvidx = dest - gpr_start;
    switch (rvidx) {
      case tmp0:     return 1 | SPMIDX_MASK;   // fixed to tmp0
      case spm_base: return 2 | SPMIDX_MASK;   // used to store sratchpad addr
      case tmp_reg1: return 3 | SPMIDX_MASK;   // tmp_reg 1
      case tmp_reg2: return 4 | SPMIDX_MASK;   // tmp_reg 2
      default: return rvidx;
    }
  }
  if (dest == rz) return 0;

  // other temps
  if (dest == s0)      return 6 | SPMIDX_MASK;
  if (dest == s1)      return 7 | SPMIDX_MASK;

  panic("bad ptr = %p", dest);
  return 0;
}

static uint32_t codebuf_read_spilled_reg[16] = {};
static int nr_instr = 0;
void load_spill_reg(const rtlreg_t* dest);
void clear_trans_buffer();
vaddr_t rv64_exec_trans_buffer(void *buf, int nr_instr, int npc_type);
extern int trans_buffer_index;
extern uint32_t trans_buffer[];

void guest_init() {
  clear_trans_buffer();
  load_spill_reg(&cpu.gpr[tmp0]._64);
  load_spill_reg(&cpu.gpr[tmp_reg1]._64);
  load_spill_reg(&cpu.gpr[tmp_reg2]._64);
  load_spill_reg(&cpu.gpr[spm_base]._64);
  assert(trans_buffer_index < 16);
  nr_instr = trans_buffer_index;
  memcpy(codebuf_read_spilled_reg, trans_buffer, nr_instr * sizeof(codebuf_read_spilled_reg[0]));
}

void guest_getregs(CPU_state *riscv64) {
  riscv64_CPU_state r;
  backend_getregs(&r);
  int i;
  for (i = 0; i < 32; i ++) {
    switch (i) {
      case tmp0: case spm_base: case tmp_reg1: case tmp_reg2: continue;
    }
    riscv64->gpr[i]._64 = r.gpr[i]._64;
  }

  rv64_exec_trans_buffer(codebuf_read_spilled_reg, nr_instr, NEXT_PC_SEQ);
  riscv64_CPU_state r2;
  backend_getregs(&r2);

  riscv64->gpr[tmp0]._64 = r2.gpr[rtlreg2varidx(NULL, &cpu.gpr[tmp0]._64) & ~SPMIDX_MASK]._64;
  riscv64->gpr[spm_base]._64 = r2.gpr[rtlreg2varidx(NULL, &cpu.gpr[spm_base]._64) & ~SPMIDX_MASK]._64;
  riscv64->gpr[tmp_reg1]._64 = r2.gpr[rtlreg2varidx(NULL, &cpu.gpr[tmp_reg1]._64) & ~SPMIDX_MASK]._64;
  riscv64->gpr[tmp_reg2]._64 = r2.gpr[rtlreg2varidx(NULL, &cpu.gpr[tmp_reg2]._64) & ~SPMIDX_MASK]._64;

  backend_setregs(&r);
}

void guest_setregs(const CPU_state *riscv64) {
  panic("not used now");
  riscv64_CPU_state r;
  backend_getregs(&r);
  int i;
  for (i = 0; i < 32; i ++) {
    switch (i) {
      case tmp0: case spm_base: case tmp_reg1: case tmp_reg2: continue;
    }
    r.gpr[i]._64 = riscv64->gpr[i]._64;
  }
  backend_setregs(&r);
}

#endif
