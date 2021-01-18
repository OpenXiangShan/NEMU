#include <isa.h>
#include <memory/paddr.h>
#include <unistd.h>
#include <rtl/rtl.h>
#include <monitor/monitor.h>
#include "user.h"

void set_nemu_state(int state, vaddr_t pc, int halt_ret);

static inline void user_sys_exit(int status) {
  set_nemu_state(NEMU_END, cpu.pc, status);
}

static inline word_t user_sys_write(int fd, word_t buf, word_t len) {
  return write(fd, guest_to_host(buf - PMEM_BASE), len);
}

static inline word_t user_sys_brk(word_t new_brk) {
  if (new_brk == 0) return user_state.brk;
  if (new_brk < PMEM_BASE + PMEM_SIZE / 2) {
    user_state.brk = new_brk;
    return new_brk;
  }
  panic("new brk = 0x%x is more than PMEM_SIZE / 2", new_brk);
}

word_t host_syscall(word_t id, word_t arg1, word_t arg2, word_t arg3) {
  int ret = 0;
  switch (id) {
    case 1: user_sys_exit(arg1); break;
    case 4: ret = user_sys_write(arg1, arg2, arg3); break;
    case 45: ret = user_sys_brk(arg1); break;
    default: panic("Unsupported syscall ID = %d", id);
  }
  return ret;
}
