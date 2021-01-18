#include <isa.h>
#include <unistd.h>
#include <monitor/monitor.h>
#include "user.h"
#include <sys/utsname.h>

void set_nemu_state(int state, vaddr_t pc, int halt_ret);

static inline void user_sys_exit(int status) {
  set_nemu_state(NEMU_END, cpu.pc, status);
}

static inline word_t user_sys_write(int fd, word_t buf, word_t len) {
  return write(fd, user_to_host(buf), len);
}

static inline word_t user_sys_brk(word_t new_brk) {
  if (new_brk == 0) return user_state.brk;
  if (new_brk < PMEM_BASE + PMEM_SIZE / 2) {
    user_state.brk = new_brk;
    return new_brk;
  }
  panic("new brk = 0x%x is more than PMEM_SIZE / 2", new_brk);
}

static inline word_t user_readlink(word_t pathname, word_t buf, size_t bufsiz) {
  return readlink(user_to_host(pathname), user_to_host(buf), bufsiz);
}

static inline word_t user_uname(word_t buf) {
  return uname(user_to_host(buf));
}

static inline word_t user_set_thread_area(word_t u_info) {
  struct user_desc {
    uint32_t entry_number;
    vaddr_t base_addr;
    uint32_t limit;
    uint32_t seg_32bit:1;
    uint32_t contents:2;
    uint32_t read_exec_only:1;
    uint32_t limit_in_pages:1;
    uint32_t seg_not_present:1;
    uint32_t useable:1;
  } *info = user_to_host(u_info);
  assert(info->entry_number == -1);
  assert(info->seg_32bit);
  assert(!info->contents);
  assert(!info->read_exec_only);
  assert(info->limit_in_pages);
  assert(!info->seg_not_present);
  assert(info->useable);
  extern uint32_t GDT[];
  static int flag = 0;
  Assert(flag == 0, "call more than once!");
  flag ++;
  GDT[2] = info->base_addr;
  info->entry_number = 2;
  return 0;
}

word_t host_syscall(word_t id, word_t arg1, word_t arg2, word_t arg3) {
  int ret = 0;
  switch (id) {
    case 1: user_sys_exit(arg1); break;
    case 4: ret = user_sys_write(arg1, arg2, arg3); break;
    case 45: ret = user_sys_brk(arg1); break;
    case 85: ret = user_readlink(arg1, arg2, arg3); break;
    case 122: ret = user_uname(arg1); break;
    case 243: ret = user_set_thread_area(arg1); break;
    default: panic("Unsupported syscall ID = %d", id);
  }
  return ret;
}
