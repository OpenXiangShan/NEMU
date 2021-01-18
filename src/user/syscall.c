#include <isa.h>
#include <memory/paddr.h>
#include <unistd.h>
#include <rtl/rtl.h>
#include <monitor/monitor.h>

void set_nemu_state(int state, vaddr_t pc, int halt_ret);

word_t host_syscall(word_t id, word_t arg1, word_t arg2, word_t arg3) {
  int ret = 0;
  switch (id) {
    case 1: // exit
      set_nemu_state(NEMU_END, cpu.pc, arg1);
      break;
    case 4: // write
      ret = write(arg1, guest_to_host(arg2 - PMEM_BASE), arg3);
      break;
    default: panic("Unsupported syscall ID = %d", id);
  }
  return ret;
}
