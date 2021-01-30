#include <utils.h>
#include <cpu/exec.h>
#include <rtl/rtl.h>
#include <cpu/difftest.h>

uint32_t pio_read(ioaddr_t addr, int len);
void pio_write(ioaddr_t addr, uint32_t data, int len);

void set_nemu_state(int state, vaddr_t pc, int halt_ret) {
  nemu_state.state = state;
  nemu_state.halt_pc = pc;
  nemu_state.halt_ret = halt_ret;
}

static inline void invalid_instr() {
  uint32_t temp[2];
  vaddr_t pc = cpu.pc;
  temp[0] = instr_fetch(&pc, 4);
  temp[1] = instr_fetch(&pc, 4);

  uint8_t *p = (void *)temp;
  printf("invalid opcode(PC = " FMT_WORD "):\n"
      "\t%02x %02x %02x %02x %02x %02x %02x %02x ...\n"
      "\t%08x %08x...\n",
      cpu.pc, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], temp[0], temp[1]);

  printf("There are two cases which will trigger this unexpected exception:\n"
      "1. The instruction at PC = " FMT_WORD " is not implemented.\n"
      "2. Something is implemented incorrectly.\n", cpu.pc);
  printf("Find this PC(" FMT_WORD ") in the disassembling result to distinguish which case it is.\n\n", cpu.pc);
  printf("\33[1;31mIf it is the first case, see\n%s\nfor more details.\n\nIf it is the second case, remember:\n"
      "* The machine is always right!\n"
      "* Every line of untested code is always wrong!\33[0m\n\n", isa_logo);

  set_nemu_state(NEMU_ABORT, cpu.pc, -1);
}

def_rtl(hostcall, uint32_t id, rtlreg_t *dest, const rtlreg_t *src, uint32_t imm) {
  switch (id) {
    case HOSTCALL_EXIT:
      difftest_skip_ref();
      set_nemu_state(NEMU_END, cpu.pc, *src);
      break;
    case HOSTCALL_INV: invalid_instr(); break;
    //case HOSTCALL_PIO: {
    //  if (imm) *dest = pio_read(*src, id_dest->width);
    //  else pio_write(*dest, *src, id_dest->width);
    //  break;
    //}
    default: isa_hostcall(id, dest, src, imm); break;
  }
}
