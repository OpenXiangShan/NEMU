#include <isa.h>
#include <unistd.h>
#include <monitor/monitor.h>
#include "user.h"
#include <sys/utsname.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>

static inline int user_fd(int fd) {
  if (fd >= 0 && fd <= 2) return user_state.std_fd[fd];
  return fd;
}

static inline sword_t get_syscall_ret(intptr_t ret) {
  return (ret == -1) ? -errno : ret;
}

static inline void translate_stat(struct stat *hostbuf, word_t user) {
  struct target_stat64 {
    uint64_t st_dev;
    unsigned char __pad0[4];

    uint32_t __st_ino;

    unsigned int st_mode;
    unsigned int st_nlink;

    uint32_t st_uid;
    uint32_t st_gid;

    uint64_t st_rdev;
    unsigned char __pad3[4];

    long long st_size;
    uint32_t st_blksize;

    uint64_t st_blocks; /* Number 512-byte blocks allocated. */

    uint32_t target_st_atime;
    uint32_t target_st_atime_nsec;

    uint32_t target_st_mtime;
    uint32_t target_st_mtime_nsec;

    uint32_t target_st_ctime;
    uint32_t target_st_ctime_nsec;

    unsigned long long st_ino;
  } __attribute__((packed)) *userbuf = user_to_host(user);

  userbuf->st_dev = hostbuf->st_dev;
  userbuf->__st_ino = hostbuf->st_ino;
  userbuf->st_mode = hostbuf->st_mode;
  userbuf->st_nlink = hostbuf->st_nlink;
  userbuf->st_uid = hostbuf->st_uid;
  userbuf->st_gid = hostbuf->st_gid;
  userbuf->st_rdev = hostbuf->st_rdev;
  userbuf->st_size = hostbuf->st_size;
  userbuf->st_blksize = hostbuf->st_blksize;
  userbuf->st_blocks = hostbuf->st_blocks;
  userbuf->target_st_atime = hostbuf->st_atime;
  userbuf->target_st_atime_nsec = hostbuf->st_atim.tv_nsec;
  userbuf->target_st_mtime = hostbuf->st_mtime;
  userbuf->target_st_mtime_nsec = hostbuf->st_mtim.tv_nsec;
  userbuf->target_st_ctime = hostbuf->st_ctime;
  userbuf->target_st_ctime_nsec = hostbuf->st_ctim.tv_nsec;
  userbuf->st_ino = hostbuf->st_ino;
}

static inline void user_sys_exit(int status) {
  void set_nemu_state(int state, vaddr_t pc, int halt_ret);
  set_nemu_state(NEMU_END, cpu.pc, status);
}

static inline word_t user_sys_brk(word_t new_brk) {
  if (new_brk == 0) return user_state.brk;
  if (new_brk >= user_state.brk_page) {
    uint32_t size = ROUNDUP(new_brk - user_state.brk_page + 1, PAGE_SIZE);
    user_mmap(user_to_host(user_state.brk_page), size, PROT_READ | PROT_WRITE,
      MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
    user_state.brk_page += size;
  }
  user_state.brk = new_brk;
  return new_brk;
}

static inline word_t user_sys_stat64(const char *pathname, word_t statbuf) {
  struct stat buf;
  int ret = get_syscall_ret(stat(pathname, &buf));
  if (ret == 0) translate_stat(&buf, statbuf);
  return ret;
}

static inline word_t user_sys_fstat64(int fd, word_t statbuf) {
  struct stat buf;
  int ret = get_syscall_ret(fstat(fd, &buf));
  if (ret == 0) translate_stat(&buf, statbuf);
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

uintptr_t host_syscall(uintptr_t id, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
    uintptr_t arg4, uintptr_t arg5, uintptr_t arg6) {
  uintptr_t ret = 0;
  switch (id) {
    case 252: // exit_group() is treated as exit()
    case 1: user_sys_exit(arg1); break;
    case 3: ret = read(user_fd(arg1), (void *)arg2, arg3); break;
    case 4: ret = write(user_fd(arg1), (void *)arg2, arg3); break;
    case 6: ret = close(user_fd(arg1)); break;
    case 33: ret = access((void *)arg1, arg2); break;
    case 45: ret = user_sys_brk(arg1); break;
    case 54: ret = ioctl(user_fd(arg1), arg2, arg3); break;
    case 85: ret = readlink((void *)arg1, (void *)arg2, arg3); break;
    case 91: ret = user_munmap((void *)arg1, arg2); break;
    case 122: ret = uname((void *)arg1); break;
    case 174: return 0; // sigaction
    case 192: ret = (uintptr_t)user_mmap((void *)arg1, arg2,
                  arg3, arg4, user_fd(arg5), arg6 << 12); break;
    case 195: return user_sys_stat64((void *)arg1, arg2);
    case 197: return user_sys_fstat64(user_fd(arg1), arg2);
    case 243: ret = user_sys_set_thread_area(arg1); break;
    case 295: ret = openat(user_fd(arg1), (void *)arg2, arg3, arg4); break;
    default: panic("Unsupported syscall ID = %ld", id);
  }
  ret = get_syscall_ret(ret);
  return ret;
}
