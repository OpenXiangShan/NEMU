#include <isa.h>
#include <unistd.h>
#include <monitor/monitor.h>
#include "user.h"
#include <sys/utsname.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

void set_nemu_state(int state, vaddr_t pc, int halt_ret);

static inline void user_sys_exit(int status) {
  set_nemu_state(NEMU_END, cpu.pc, status);
}

static inline word_t user_sys_brk(word_t new_brk) {
  if (new_brk == 0) return user_state.brk;
  if (new_brk >= user_state.brk_page) {
    uint32_t size = ROUNDUP(new_brk - user_state.brk_page + 1, PAGE_SIZE);
    void *ret = mmap(user_to_host(user_state.brk_page), size, PROT_READ | PROT_WRITE,
      MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
    assert(ret == user_to_host(user_state.brk_page));
    user_state.brk_page += size;
  }
  user_state.brk = new_brk;
  return new_brk;
}

static inline word_t user_sys_fstat64(int fd, word_t statbuf) {
  struct stat buf;
  int ret = fstat(fd, &buf);
  if (ret != 0) return ret;

  struct target_stat64 {
    unsigned short st_dev;
    unsigned char __pad0[10];

    uint32_t __st_ino;

    unsigned int st_mode;
    unsigned int st_nlink;

    uint32_t st_uid;
    uint32_t st_gid;

    unsigned short st_rdev;
    unsigned char __pad3[10];

    long long st_size;
    uint32_t st_blksize;

    uint32_t st_blocks; /* Number 512-byte blocks allocated. */
    uint32_t __pad4;  /* future possible st_blocks high bits */

    uint32_t target_st_atime;
    uint32_t target_st_atime_nsec;

    uint32_t target_st_mtime;
    uint32_t target_st_mtime_nsec;

    uint32_t target_st_ctime;
    uint32_t target_st_ctime_nsec;

    unsigned long long st_ino;
  } __attribute__((packed)) *userbuf = user_to_host(statbuf);

  userbuf->st_dev = buf.st_dev;
  userbuf->__st_ino = buf.st_ino;
  userbuf->st_mode = buf.st_mode;
  userbuf->st_nlink = buf.st_nlink;
  userbuf->st_uid = buf.st_uid;
  userbuf->st_gid = buf.st_gid;
  userbuf->st_rdev = buf.st_rdev;
  userbuf->st_size = buf.st_size;
  userbuf->st_blksize = buf.st_blksize;
  userbuf->st_blocks = buf.st_blocks;
  userbuf->target_st_atime = buf.st_atime;
  userbuf->target_st_atime_nsec = buf.st_atim.tv_nsec;
  userbuf->target_st_mtime = buf.st_mtime;
  userbuf->target_st_mtime_nsec = buf.st_mtim.tv_nsec;
  userbuf->target_st_ctime = buf.st_ctime;
  userbuf->target_st_ctime_nsec = buf.st_ctim.tv_nsec;
  userbuf->st_ino = buf.st_ino;
  return ret;
}

static inline word_t user_sys_set_thread_area(word_t u_info) {
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

uintptr_t host_syscall(uintptr_t id, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3) {
  int ret = 0;
  switch (id) {
    case 252: // exit_group() is treated as exit()
    case 1: user_sys_exit(arg1); break;
    case 4: ret = write(arg1, (void *)arg2, arg3); break;
    case 33: ret = access((void *)arg1, arg2); break;
    case 45: ret = user_sys_brk(arg1); break;
    case 54: ret = ioctl(arg1, arg2, arg3); break;
    case 85: ret = readlink((void *)arg1, (void *)arg2, arg3); break;
    case 122: ret = uname((void *)arg1); break;
    case 197: ret = user_sys_fstat64(arg1, arg2); break;
    case 243: ret = user_sys_set_thread_area(arg1); break;
    default: panic("Unsupported syscall ID = %ld", id);
  }
  return ret;
}
